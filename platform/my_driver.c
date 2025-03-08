#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/init.h>

/* Probe 函数：当设备与驱动匹配时调用 */
static int my_driver_probe(struct platform_device *pdev)
{
	pr_info("my_driver: Probe called for device %s\n", pdev->name);
	return 0;
}

/* Remove 函数：当设备移除时调用 */
static int my_driver_remove(struct platform_device *pdev)
{
	pr_info("my_driver: Remove called for device %s\n", pdev->name);
	return 0;
}

/* Suspend 函数：设备进入低功耗状态时调用 */
static int my_driver_suspend(struct device *dev)
{
	pr_info("my_driver: Suspending device\n");
	return 0;
}

/* Resume 函数：设备从低功耗状态恢复时调用 */
static int my_driver_resume(struct device *dev)
{
	pr_info("my_driver: Resuming device\n");
	return 0;
}

/* Hibernation: Freeze */
static int my_driver_freeze(struct device *dev)
{
	pr_info("my_driver: Freeze called\n");
	return 0;
}

/* Hibernation: Thaw */
static int my_driver_thaw(struct device *dev)
{
	pr_info("my_driver: Thaw called\n");
	return 0;
}

/* Hibernation: Poweroff */
static int my_driver_poweroff(struct device *dev)
{
	pr_info("my_driver: Poweroff called\n");
	return 0;
}

/* Hibernation: Restore */
static int my_driver_restore(struct device *dev)
{
	pr_info("my_driver: Restore called\n");
	return 0;
}

/* 电源管理操作集 */
static const struct dev_pm_ops my_driver_pm_ops = {
	.suspend = my_driver_suspend,
	.resume = my_driver_resume,
	.freeze = my_driver_freeze,
	.thaw = my_driver_thaw,
	.poweroff = my_driver_poweroff,
	.restore = my_driver_restore,
};

/* Platform Driver 结构体 */
static struct platform_driver my_platform_driver = {
	.probe = my_driver_probe,
	.remove = my_driver_remove,
	.driver = {
		.name = "my_platform_device",  // 驱动名称，必须与设备名称匹配
		.owner = THIS_MODULE,
		.pm = &my_driver_pm_ops,      // 电源管理操作集
	},
};

/* 模块初始化函数 */
static int __init my_driver_init(void)
{
	pr_info("my_driver: Initializing platform driver\n");

	/* 注册 Platform Driver */
	return platform_driver_register(&my_platform_driver);
}

/* 模块退出函数 */
static void __exit my_driver_exit(void)
{
	pr_info("my_driver: Exiting platform driver\n");

	/* 注销 Platform Driver */
	platform_driver_unregister(&my_platform_driver);
}

module_init(my_driver_init);
module_exit(my_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simplified platform driver example");
MODULE_VERSION("1.0");
