#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

struct hello_pdata {
	struct work_struct remove_work;
	struct delayed_work adc_work;
};

static struct hello_pdata *data;

static void remove_work_handler(struct work_struct *work)
{
	struct hello_pdata *data = container_of(work, struct hello_pdata, remove_work);

	pr_info("1: hello_pdata: %p\n", data);
}

static void adc_work_handler(struct work_struct *work)
{
	struct hello_pdata *data = container_of(work, struct hello_pdata, adc_work.work);

	pr_info("2: hello_pdata: %p\n", data);

	queue_delayed_work(system_wq, &data->adc_work, msecs_to_jiffies(1000));
}

static int __init hello_init(void)
{
	printk(KERN_DEBUG "Hello, World!\n");

	data = kmalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	INIT_WORK(&data->remove_work, remove_work_handler);
	INIT_DELAYED_WORK(&data->adc_work, adc_work_handler);

	schedule_work(&data->remove_work);
	mod_delayed_work(system_wq, &data->adc_work, msecs_to_jiffies(1000));

	return 0;
}

static void __exit hello_exit(void)
{
	cancel_delayed_work_sync(&data->adc_work);
	kfree(data);
	printk(KERN_DEBUG "Goodbye, cruel world!\n");
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(hello_init);
module_exit(hello_exit);
