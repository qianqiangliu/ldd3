#include <linux/module.h>
#include <linux/jiffies.h>

static int __init jiffies_init(void)
{
	printk(KERN_DEBUG "jiffies / HZ = %ld\n", jiffies / HZ);
	return 0;
}

static void __exit jiffies_exit(void)
{
	printk(KERN_DEBUG "jiffies / HZ = %ld\n", jiffies / HZ);
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(jiffies_init);
module_exit(jiffies_exit);
