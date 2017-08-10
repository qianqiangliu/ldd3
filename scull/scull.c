#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "scull.h"

static int scull_major = SCULL_MAJOR;
static int scull_minor = 0;
static int scull_nr_devs = SCULL_NR_DEVS;
static int scull_quantum = SCULL_QUANTUM;
static int scull_qset = SCULL_QSET;

module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_nr_devs, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);
module_param(scull_qset, int, S_IRUGO);

struct scull_dev *scull_devices;	/* allocated in scull_init_module */

#if 0
loff_t scull_llseek(struct file *fp, loff_t loff, int a)
{
	return 0;
}
#endif

/*
 * Follow the list
 */
static struct scull_qset *scull_follow(struct scull_dev *dev, int n)
{
	struct scull_qset *qs = dev->data;

	/* Allocate first qset explicitly if need be */
	if (! qs) {
		qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;  /* Never mind */
		memset(qs, 0, sizeof(struct scull_qset));
	}

	/* Then follow the list */
	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;  /* Never mind */
			memset(qs->next, 0, sizeof(struct scull_qset));
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

static ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (*f_pos > dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/* 在量子集中寻找链表项，qset索引以及偏移量 */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	/* 沿该链表前行，直到正确的位置（在其他地方定义） */
	dptr = scull_follow(dev, item);

	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		goto out;	/* don't fill holes */

	/* 读取该量子的数据直到结尾 */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;
out:
	up(&dev->sem);
	return retval;
}

static ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum;
	int qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (*f_pos > dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/* 在量子集中寻找链表项，qset索引以及偏移量 */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	/* 沿该链表前行，直到正确的位置（在其他地方定义） */
	dptr = scull_follow(dev, item);

	if (dptr == NULL)
		goto out;
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto out;
	}
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	/* 读取该量子的数据直到结尾 */
	if (count > quantum - q_pos)
		count = quantum - q_pos;
	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

	if (dev->size < *f_pos)
		dev->size = *f_pos;
out:
	up(&dev->sem);
	return retval;
}

static int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next, *dptr;
	int qset = dev->qset;
	int i;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;
	return 0;
}

static int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);

	return 0;
}

static int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
	.read = scull_read,
	.write = scull_write,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;
	int devno;

	devno = MKDEV(scull_major, scull_minor + index);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);

	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

static void scull_exit(void)
{
	int i;
	dev_t dev = MKDEV(scull_major, scull_minor);

	if (scull_devices) {
		for (i = 0; i < scull_nr_devs; i++) {
			scull_trim(scull_devices + i);
			cdev_del(&scull_devices[i].cdev);
		}
		kfree(scull_devices);
	}

	unregister_chrdev_region(dev, scull_nr_devs);
}

static int __init scull_init(void)
{
	dev_t dev;
	int res, i;

	if (scull_major) {
		dev = MKDEV(scull_major, scull_minor);
		res = register_chrdev_region(dev, scull_nr_devs, "scull");
	} else {
		res = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs,
					  "scull");
		scull_major = MAJOR(dev);
	}
	if (res < 0) {
		printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
		return res;
	}

	/*
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
	if (!scull_devices) {
		res = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

	/* Initialize each device. */
	for (i = 0; i < scull_nr_devs; i++) {
		scull_devices[i].quantum = scull_quantum;
		scull_devices[i].qset = scull_qset;
		sema_init(&scull_devices[i].sem, 1);
		scull_setup_cdev(&scull_devices[i], i);
	}

	return 0;
fail:
	scull_exit();
	return res;
}

MODULE_LICENSE("GPL");
module_init(scull_init);
module_exit(scull_exit);
