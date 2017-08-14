#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

static struct cdev mycdev;
static int mycdev_major;
static int mycdev_minor;
static struct class *mycdev_class;
static char mycdev_buf[4096];

static ssize_t mycdev_read(struct file *filp, char __user *buf, size_t count,
			   loff_t *f_pos)
{
	printk(KERN_DEBUG "read count = %ld\n", count);
	if (*f_pos + count > sizeof(mycdev_buf))
		count = sizeof(mycdev_buf) - *f_pos;

	if (copy_to_user(buf, mycdev_buf + *f_pos, count))
		return -EFAULT;

	*f_pos += count;
	return count;
}

static ssize_t mycdev_write(struct file *filp, const char __user *buf,
			    size_t count, loff_t *f_pos)
{
	printk(KERN_DEBUG "write count = %ld\n", count);
	if (*f_pos + count > sizeof(mycdev_buf))
		count = sizeof(mycdev_buf) - *f_pos;

	if (copy_from_user(mycdev_buf + *f_pos, buf, count))
		return -EFAULT;

	*f_pos += count;
	return count;
}

struct file_operations mycdev_fops = {
	.owner = THIS_MODULE,
	.read = mycdev_read,
	.write = mycdev_write
};

static int mycdev_setup(dev_t dev)
{
	struct device *device;
	int retval;

	cdev_init(&mycdev, &mycdev_fops);
	mycdev.owner = THIS_MODULE;
	if ((retval = cdev_add(&mycdev, dev, 1))) {
		unregister_chrdev_region(dev, 1);
		return retval;
	}
	mycdev_class = class_create(THIS_MODULE, "mycdev");
	if (IS_ERR(mycdev_class)) {
		printk(KERN_NOTICE "mycdev: can't create mycdev class\n");
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	device = device_create(mycdev_class, NULL, dev, NULL, "mycdev");
	if (IS_ERR(device)) {
		printk(KERN_NOTICE "mycdev: can't create mycdev device\n");
		unregister_chrdev_region(dev, 1);
		class_destroy(mycdev_class);
		return -1;
	}

	return 0;
}

static int __init mycdev_init(void)
{
	int retval;
	dev_t dev;

	retval = alloc_chrdev_region(&dev, mycdev_minor, 1, "mycdev");
	if (retval < 0) {
		printk("mycdev: can't get major %d\n", mycdev_major);
		return retval;
	}
	mycdev_major = MAJOR(dev);

	return mycdev_setup(dev);
}

static void __exit mycdev_exit(void)
{
	dev_t dev = MKDEV(mycdev_major, mycdev_minor);

	device_destroy(mycdev_class, dev);
	class_destroy(mycdev_class);
	unregister_chrdev_region(dev, 1);
}

MODULE_LICENSE("GPL");
module_init(mycdev_init);
module_exit(mycdev_exit);
