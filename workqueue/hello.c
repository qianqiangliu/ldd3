#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

static struct workqueue_struct *wq = NULL;
static struct work_struct work;

static struct timer_list my_timer;

static void work_handler(struct work_struct *work)
{
	pr_info("%s:%d\n", __FUNCTION__, __LINE__);
}

void my_timer_callback(struct timer_list *list)
{
	queue_work(wq, &work);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
}

static int __init hello_init(void)
{
	int err;

	wq = create_singlethread_workqueue("workqueue");
	if (!wq)
		return -1;
	INIT_WORK(&work, work_handler);

	timer_setup(&my_timer, my_timer_callback, 0);
	/* setup timer interval to 200 msecs */
	err = mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000));
	if (err) {
		destroy_workqueue(wq);
		return -1;
	}
	return 0;
}

static void __exit hello_exit(void)
{
	destroy_workqueue(wq);
	del_timer(&my_timer);
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(hello_init);
module_exit(hello_exit);
