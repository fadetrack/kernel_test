#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>

struct kprobe kp;

int handler_pre(struct kprobe *p, struct pt_regs *regs) {
    printk(KERN_INFO "pt_regs: %p, pid: %d, jiffies: %ld\n",
                     regs, current->tgid, jiffies);
    return 0;
}

static __init int init_schk(void)
{
    kp.addr = (kprobe_opcode_t *)0xffffffff8003e1b4;
    kp.pre_handler = handler_pre;
    register_kprobe(&kp);

    return 0;
}
module_init(init_schk);

static __exit void cleanup_schk(void)
{
    unregister_kprobe(&kp);
}
module_exit(cleanup_schk);

MODULE_LICENSE("GPL");
