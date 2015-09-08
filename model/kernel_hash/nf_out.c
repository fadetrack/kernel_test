#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/version.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/net.h>
#include <linux/tcp.h>
#include <net/inet_hashtables.h>
#include "debug.h"
#include "chunk.h"
#include "sha.h"

struct tcp_chunk *hash_head = NULL;
unsigned long save_num = 0;
rwlock_t hash_rwlock = RW_LOCK_UNLOCKED; /* Static way which get rwlock*/
spinlock_t hash_lock = SPIN_LOCK_UNLOCKED;
int i = 0;                             /* just for test*/

void hand_hash(uint8_t dst[], size_t len) 
{
	struct tcp_chunk *element;

	read_lock(&hash_rwlock);
	HASH_FIND_STR(hash_head, dst, element);  /* dst already in the hash? */
	read_unlock(&hash_rwlock);
    if (element == NULL) {
		element = (struct tcp_chunk*)kmalloc(sizeof(struct tcp_chunk), GFP_KERNEL);
    	element->sha = dst;
    	element->id = i++;
		write_lock(&hash_rwlock);
    	HASH_ADD_KEYPTR(hh, hash_head, element->sha, SHALEN, element);
		write_unlock(&hash_rwlock);
    } else {
		spin_lock(&hash_lock);
		save_num += len;
		spin_unlock(&hash_lock);
		//DEBUG_LOG("\n save len is:%d\n", len);
	}
}

void build_hash(char *src, int start, int end, int length) 
{
	/*
     * Fixup: use slab maybe effectiver than kmalloc.
     */
	uint8_t *dst = kmalloc(sizeof(uint8_t)*(SHALEN+1), GFP_KERNEL);
	memset(dst, '\0', SHALEN+1);
	ecryptfs_calculate_sha1(dst, src + start, (end - start + 1));
	
	hand_hash(dst, length); 
}

void get_partition(char *data, int length)
{
	struct kfifo *fifo = NULL;
	spinlock_t lock = SPIN_LOCK_UNLOCKED;
	int fifo_i, fifo_part, value, fifo_len; 
	int start_pos = 0;
	int end_pos = 0;

	/*
	 * Fixup: alloc kfifo everytime.
     */	
	fifo = kfifo_alloc(KFIFOLEN, GFP_KERNEL, &lock);
	if (unlikely(fifo == NULL)) {
		printk(KERN_ERR "alloc kfifo failed.");
		BUG();
	}	

	if (length > chunk_num) {	
		calculate_partition(data, length, fifo);

		fifo_part = kfifo_len(fifo);
		fifo_len = fifo_part/sizeof(int); 

		DEBUG_LOG(KERN_INFO "length is:%d, fifo_len is:%lu", length, fifo_part/sizeof(int));
		for (fifo_i = 0; fifo_i < fifo_part/sizeof(value); ++fifo_i) {
			kfifo_get(fifo, (unsigned char *)&value, sizeof(value));
		
			//DEBUG_LOG(KERN_INFO "fifo_i is:%d", fifo_i);
			if (fifo_i == 0) {
				start_pos = 0;
				end_pos = value;
				DEBUG_LOG(KERN_INFO "start_pos is:%d,end_pos is:%d", start_pos, end_pos);
				//ecryptfs_calculate_sha1(dst, data + start_pos, (end_pos - start_pos + 1));
				build_hash(data, start_pos, end_pos, (end_pos - start_pos + 1));
			} 
			else {
				start_pos = end_pos + 1;
				end_pos = value;
				DEBUG_LOG(KERN_INFO "start_pos is:%d,end_pos is:%d", start_pos, end_pos);
				//ecryptfs_calculate_sha1(dst, data + start_pos, (end_pos - start_pos + 1));
				build_hash(data, start_pos, end_pos, (end_pos - start_pos + 1));
			}
		}
		
		if (fifo_len > 0)  {
			if (end_pos != length - 1) {
				start_pos = end_pos + 1;
				end_pos = length - 1;
				DEBUG_LOG(KERN_INFO "start_pos is:%d,end_pos is:%d", start_pos, end_pos);
				build_hash(data, start_pos, end_pos, (end_pos - start_pos + 1));
			}
		} else {
			start_pos = 0;
			end_pos = length - 1;
			DEBUG_LOG(KERN_INFO "start_pos is:%d,end_pos is:%d", start_pos, end_pos);
			build_hash(data, start_pos, end_pos, (end_pos - start_pos + 1));
		}			
	}

	kfifo_free(fifo);	
}

static unsigned int nf_out(
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
		const struct nf_hook_ops *ops,
#else
		unsigned int hooknum,
#endif
		struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	char *data = NULL;
	size_t data_len = 0;
	unsigned short dport;
	struct iphdr *iph = (struct iphdr *)skb->data;
	struct tcphdr *tcph = (struct tcphdr *)(skb->data + (iph->ihl << 2));
	
	skb_linearize(skb);
	
	if (iph->protocol != IPPROTO_TCP)
		return NF_ACCEPT;

	dport = tcph->dest;

	if (likely(ntohs(dport) == 8888)) {	
		if (HASH_OVERHEAD(hh, hash_head) >= MEMLIMIT) {
			return NF_ACCEPT;
		}

		data = (char *)((unsigned char *)tcph + (tcph->doff << 2));
		data_len = ntohs(iph->tot_len) - (iph->ihl << 2) - (tcph->doff << 2);
		DEBUG_LOG(KERN_INFO "chunk is %d, data_len is %lu, iph_tot is%d, iph is%d, tcph is%d", chunk_num, data_len, ntohs(iph->tot_len), (iph->ihl << 2), (tcph->doff<<2));
		//for (i = 0; i < data_len; ++i)
			//DEBUG_LOG(KERN_INFO "data is:%02x", data[i]&0xff);

		get_partition(data, data_len);
	}

	return NF_ACCEPT;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 32)
static unsigned int nf_out_p(unsigned int hooknum,
		struct sk_buff **skb
		const struct net_device *in,
		const struct net_device *out,
		int (*okfn) (struct sk_buff *))
{
	return nf_out(hooknum, *skb, in, out, okfn);
}
#endif

struct nf_hook_ops nf_out_ops = {
	.list		= { NULL, NULL},
	.pf		= PF_INET,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 32)
	.hook           = nf_out,
	.hooknum        = NF_INET_POST_ROUTING,
#else
	.hook           = nf_out_p,
	.hooknum        = NF_IP_POST_ROUTING,
#endif
	.priority       = NF_IP_PRI_LAST,
};
