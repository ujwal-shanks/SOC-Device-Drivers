#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ujwal");
MODULE_DESCRIPTION("A simple Hello Linux kernel module");
MODULE_VERSION("0.1");

static int __init hello_world_lkm_init(void) {
    printk(KERN_INFO "Hello Linux, my love >3\n");
    return 0;
}

static void __exit hello_world_lkm_exit(void) {
    printk(KERN_INFO "Goodbye Linux, my love >3\n");
}

module_init(hello_world_lkm_init);
module_exit(hello_world_lkm_exit);
