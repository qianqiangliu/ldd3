#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>

static int hello_suspend(struct device *dev)
{
	printk(KERN_DEBUG "%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int hello_resume(struct device *dev)
{
	printk(KERN_DEBUG "%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int hello_probe(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int hello_remove(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s:%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static SIMPLE_DEV_PM_OPS(hello_pm_ops, hello_suspend, hello_resume);

static struct platform_driver hello_drv = {
	.probe = hello_probe,
	.remove = hello_remove,
	.driver = {
		.name		= "hello",
		.pm		= &hello_pm_ops,
	},
};

static struct platform_device *hello_platform_device;

static int __init hello_init(void)
{
	int err;

	printk(KERN_DEBUG "Hello, World!\n");
	err = platform_driver_register(&hello_drv);
	if (err)
		return err;

	hello_platform_device = platform_device_alloc("hello", -1);
	if (!hello_platform_device) {
		err = -ENOMEM;
		goto err_driver_unregister;
	}

	err = platform_device_add(hello_platform_device);
	if (err)
		goto err_free_device;

	return 0;

err_free_device:
	platform_device_put(hello_platform_device);
err_driver_unregister:
	platform_driver_unregister(&hello_drv);

	return err;
}

static void __exit hello_exit(void)
{
	printk(KERN_DEBUG "Goodbye, cruel world!\n");
	platform_device_unregister(hello_platform_device);
	platform_driver_unregister(&hello_drv);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("Dual BSD/GPL");
