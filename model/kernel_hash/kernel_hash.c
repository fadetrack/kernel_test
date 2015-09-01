#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include "debug.h"
MODULE_LICENSE("Dual BSD/GPL");

#define SHA1_LENGTH     20

static int hello_init(void)
{
	/*
     * http://lxr.oss.org.cn/source/fs/ecryptfs/crypto.c?v=2.6.30
     */
	struct scatterlist sg;
	struct hash_desc desc;

	//way 1		
	/*
	char *plaintext = NULL;
	printk(KERN_INFO "valid=%s\n", virt_addr_valid(plaintext) ? "true" : "false");
	plaintext = "c";
	size_t len = strlen(plaintext);
	*/

	//way 2
	/*
	char *plaintext = kmalloc(sizeof(char), GFP_KERNEL);
	printk(KERN_INFO "valid=%s\n", virt_addr_valid(plaintext) ? "true" : "false");
	*plaintext = 'c'; 	
	printk(KERN_INFO "valid=%s\n", virt_addr_valid(plaintext) ? "true" : "false");
	size_t len = 1;
	*/

	// way 3.
	/*	
	char plaintext[1] = {'c'};
	size_t len = 1;
	*/

	// way 4.
	char *plaintext = (char *)__get_free_page(GFP_KERNEL);
	memcpy(plaintext, "c", 1);
	size_t len = 1;

	int rc = 0;
	int i; 
	char hashtext[SHA1_LENGTH];

    memset(hashtext, 0x00, SHA1_LENGTH);
    printk(KERN_INFO "sha1: %s\n", __FUNCTION__);
	printk(KERN_INFO "valid=%s PAGE=%lu, plaintext=%lu, %d , %s\n", virt_addr_valid(plaintext) ? "true" : "false", PAGE_OFFSET, (unsigned long)plaintext, (unsigned long)plaintext - PAGE_OFFSET, ((unsigned long)plaintext > __START_KERNEL_map) ? "true" : "false");

	sg_init_one(&sg, plaintext, len);
	desc.tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	desc.flags = 0;
	
	rc = crypto_hash_init(&desc);
	if (rc) {
		printk(KERN_ERR "%s: Error initializing crypto hash; rc = [%d]\n", __func__, rc);
     	goto out;
    }
	rc = crypto_hash_update(&desc, &sg, len);
	if (rc) {
    	printk(KERN_ERR "%s: Error updating crypto hash; rc = [%d]\n", __func__, rc);
        goto out;
    }
	rc = crypto_hash_final(&desc, hashtext);
	if (rc) {
    	printk(KERN_ERR "%s: Error finalizing crypto hash; rc = [%d]\n", __func__, rc);
        goto out;
    }
	crypto_free_hash(desc.tfm);
    
	for (i = 0; i < 20; i++) {
        printk(KERN_INFO "%02x-%d\n", hashtext[i]&0xff, i);
    }

out:
    printk(KERN_INFO "end\n");
    return rc;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "Goodbye, cruel world\n");
}

module_init(hello_init);
module_exit(hello_exit);
