#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched/signal.h> // for_each_process and task_struct

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ujwal");
MODULE_DESCRIPTION("LKM to list all processes PID, state and name");

static int __init list_process_init(void) {
    struct task_struct *task;

    printk(KERN_INFO "Process list module loaded\n");

    for_each_process(task) {
        printk(KERN_INFO "PID: %d | State: %ld | Name: %s\n",
               task->pid, task->state, task->comm);
    }

    return 0;
}

static void __exit list_process_exit(void) {
    printk(KERN_INFO "Process list module unloaded\n");
}

module_init(list_process_init);
module_exit(list_process_exit);
