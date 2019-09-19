#include <linux/module.h>
#include <linux/device.h>

struct class cp;

static ssize_t table_show(struct class *cls, struct class_attribute *attr,
		char *buf)
{
	printk("table_show\n");
	return 0;
}

static ssize_t table_store(struct class *cls, struct class_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}


struct class_attribute class_attrs[] = {
	__ATTR_RW(table),
	__ATTR_NULL
};


static int __init class_init(void)
{
	int ret;

	cp.name = "class_node";
	cp.owner = THIS_MODULE;
	cp.class_attrs = class_attrs;
	ret = class_register(&cp);
	if (ret) {
		printk("Fail to create class test\n");
	}

	return ret;
}

static void __exit class_exit(void)
{
	class_unregister(&cp);
	printk(KERN_DEBUG "class_exit\n");
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(class_init);
module_exit(class_exit);
