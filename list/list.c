#include <linux/module.h>
#include <linux/sched.h>
#include <linux/list.h>

static int __init list_init(void)
{
	struct task_struct *task, *pos;

	task = &init_task;

	list_for_each_entry(pos, &task->tasks, tasks) {
		printk(KERN_DEBUG "%d\t%s\n", pos->pid, pos->comm);
	}

	return 0;
}

static void __exit list_exit(void)
{
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(list_init);
module_exit(list_exit);
