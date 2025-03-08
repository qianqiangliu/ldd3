#include <linux/platform_device.h>
#include <linux/module.h>

/* 定义 Platform Device */
static struct platform_device my_platform_device = {
    .name = "my_platform_device",  // 设备名称，必须与驱动名称匹配
    .id = -1,  // 设备 ID，-1 表示只有一个设备
};

/* 注册 Platform Device */
static int __init my_device_init(void)
{
    pr_info("my_device: Registering platform device\n");
    return platform_device_register(&my_platform_device);
}

/* 注销 Platform Device */
static void __exit my_device_exit(void)
{
    pr_info("my_device: Unregistering platform device\n");
    platform_device_unregister(&my_platform_device);
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simplified platform device example");
MODULE_VERSION("1.0");
