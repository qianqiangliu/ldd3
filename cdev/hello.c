#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>

struct hello_device {
	dev_t dev;
	struct cdev cdev;
};

static struct hello_device *hd;

static int hello_open(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "in %s\n", __func__);
	return 0;
}

static int hello_release(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "in %s\n", __func__);
	return 0;
}

static ssize_t hello_read(struct file *filp, char __user *buf, size_t count,
			  loff_t *f_pos)
{
	printk(KERN_DEBUG "in %s\n", __func__);
	return 0;
}

static ssize_t hello_write(struct file *filp, const char __user *buf, size_t count,
			   loff_t *f_pos)
{
	printk(KERN_DEBUG "in %s\n", __func__);
	return 0;
}

static struct file_operations hello_fops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.release = hello_release,
	.read = hello_read,
	.write = hello_write,
};

static int __init hello_init(void)
{
	int result;

	hd = kmalloc(sizeof(struct hello_device), GFP_KERNEL);
	if (!hd)
		return -ENOMEM;

	result = alloc_chrdev_region(&hd->dev, 0, 1, "hello");
	if (result < 0) {
		printk(KERN_ERR "hello: can't get major number\n");
		kfree(hd);
		return result;
	}

	cdev_init(&hd->cdev, &hello_fops);
	hd->cdev.owner = THIS_MODULE;
	result = cdev_add(&hd->cdev, hd->dev, 1);
	if (result) {
		printk(KERN_ERR "Error %d adding hello\n", result);
		unregister_chrdev_region(hd->dev, 1);
		kfree(hd);
		return result;
	}

	printk(KERN_DEBUG "Hello, World!\n");

	return 0;
}

static void __exit hello_exit(void)
{
	cdev_del(&hd->cdev);
	unregister_chrdev_region(hd->dev, 1);
	kfree(hd);

	printk(KERN_DEBUG "Goodbye, cruel world!\n");
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(hello_init);
module_exit(hello_exit);
