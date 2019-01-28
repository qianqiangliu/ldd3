#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/fs.h>

#include <asm-generic/io.h>
#include <asm-generic/uaccess.h>

#define SHORT_NR_PORTS	3

static unsigned long base = 0x378;
module_param(base, long, 0);

static int major = 0;	/* dynamic by default */
module_param(major, int, 0);

ssize_t short_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned char value;

	value = inb(base);
	rmb();

	if (copy_to_user(buf, &value, 1))
		return -EFAULT;

	return 1;
}

ssize_t short_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	int i;
	unsigned char *ptr;
	unsigned char *kbuf = kmalloc(count, GFP_KERNEL);

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	ptr = kbuf;

	for (i = 0; i < count; i++) {
		outb(ptr[i], base);
		wmb();
	}

	kfree(kbuf);
	return count;
}

int short_open (struct inode *inode, struct file *filp)
{
	return 0;
}

int short_release (struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations short_fops = {
	.owner	 = THIS_MODULE,
	.read	 = short_read,
	.write	 = short_write,
	.open	 = short_open,
	.release = short_release,
};

static int __init short_init(void)
{
	int result;

	if (!request_region(base, SHORT_NR_PORTS, "short")) {
		printk(KERN_INFO "short: can't get I/O port address 0x%lx\n", base);
		return -ENODEV;
	}

	result = register_chrdev(major, "short", &short_fops);
	if (result < 0) {
		printk(KERN_INFO "short: can't get major number\n");
		release_region(base, SHORT_NR_PORTS); /* FIXME - use-mem case? */
		return result;
	}

	if (major == 0)
		major = result;

	printk(KERN_INFO "short major is %d\n", major);

	return 0;
}

static void __exit short_exit(void)
{
	unregister_chrdev(major, "short");
	release_region(base, SHORT_NR_PORTS);
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(short_init);
module_exit(short_exit);
