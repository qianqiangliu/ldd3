#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/gfp.h>

#include "scullp.h"

static int scullp_major = SCULLP_MAJOR;
static int scullp_minor = 0;
static int scullp_nr_devs = SCULLP_NR_DEVS;
static int scullp_qset = SCULLP_QSET;
static int scullp_order = SCULLP_ORDER;

module_param(scullp_major, int, S_IRUGO);
module_param(scullp_minor, int, S_IRUGO);
module_param(scullp_nr_devs, int, S_IRUGO);
module_param(scullp_qset, int, S_IRUGO);
module_param(scullp_order, int, S_IRUGO);

static struct scullp_dev *scullp_devices;	/* allocated in scullp_init_module */

static int scullp_trim(struct scullp_dev *dev)
{
	struct scullp_qset *next, *dptr;
	int qset = dev->qset;
	int i;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++) {
				if (dptr->data[i])
					free_pages((unsigned long)(dptr->data[i]), dev->order);
			}
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->qset = scullp_qset;
	dev->data = NULL;
	dev->order = scullp_order;
	return 0;
}

static int scullp_open(struct inode *inode, struct file *filp)
{
	struct scullp_dev *dev;

	printk(KERN_DEBUG "in scullp_open\n");

	dev = container_of(inode->i_cdev, struct scullp_dev, cdev);
	filp->private_data = dev;

	/* now trim to 0 the length of the device if open was write-only */
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		scullp_trim(dev);
		up(&dev->sem);
	}
	return 0;
}

static int scullp_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * Follow the list
 */
static struct scullp_qset *scullp_follow(struct scullp_dev *dev, int n)
{
	struct scullp_qset *qs = dev->data;

	/* Allocate first qset explicitly if need be */
	if (!qs) {
		qs = dev->data = kmalloc(sizeof(struct scullp_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;  /* Never mind */
		memset(qs, 0, sizeof(struct scullp_qset));
	}

	/* Then follow the list */
	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct scullp_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;  /* Never mind */
			memset(qs->next, 0, sizeof(struct scullp_qset));
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

static ssize_t scullp_read(struct file *filp, char __user *buf, size_t count,
			  loff_t *f_pos)
{
	struct scullp_dev *dev = filp->private_data;
	struct scullp_qset *dptr;	/* the first listitem */
	int quantum = PAGE_SIZE << dev->order, qset = dev->qset;
	int itemsize = quantum * qset; /* how many bytes in the listitem */
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (*f_pos >= dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/* find listitem, qset index, and offset in the quantum */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	/* follow the list up to the right position (defined elsewhere) */
	dptr = scullp_follow(dev, item);

	if (dptr == NULL || !dptr->data || ! dptr->data[s_pos])
		goto out; /* don't fill holes */

	/* read only up to the end of this quantum */
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

static ssize_t scullp_write(struct file *filp, const char __user *buf, size_t count,
			   loff_t *f_pos)
{
	struct scullp_dev *dev = filp->private_data;
	struct scullp_qset *dptr;
	int quantum = PAGE_SIZE << dev->order, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* find listitem, qset index and offset in the quantum */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	/* follow the list up to the right position */
	dptr = scullp_follow(dev, item);
	if (dptr == NULL)
		goto out;
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = (void *)__get_free_pages(GFP_KERNEL, dev->order);
		if (!dptr->data[s_pos])
			goto out;
		memset(dptr->data[s_pos], 0, quantum);
	}
	/* write only up to the end of this quantum */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

        /* update the size */
	if (dev->size < *f_pos)
		dev->size = *f_pos;

out:
	up(&dev->sem);
	return retval;
}

static loff_t scullp_llseek(struct file *filp, loff_t off, int whence)
{
	struct scullp_dev *dev = filp->private_data;
	loff_t newpos;

	switch (whence) {
	case SEEK_SET:
		newpos = off;
		break;

	case SEEK_CUR:
		newpos = filp->f_pos + off;
		break;

	case SEEK_END:
		newpos = dev->size + off;
		break;

	default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0)
	    return -EINVAL;
	filp->f_pos = newpos;
	return newpos;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
int scullp_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg)
#else
long scullp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	int err = 0, tmp;
	int retval = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	struct scullp_dev *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
#endif

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != SCULLP_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULLP_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
	case SCULLP_IOCRESET:
		scullp_order = SCULLP_ORDER;
		scullp_qset = SCULLP_QSET;
		break;

	case SCULLP_IOCSORDER: /* Set: arg points to the value */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(scullp_order, (int __user *)arg);
		break;

	case SCULLP_IOCTORDER: /* Tell: arg is the value */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		scullp_order = arg;
		break;

	case SCULLP_IOCGORDER: /* Get: arg is pointer to result */
		retval = __put_user(scullp_order, (int __user *)arg);
		break;

	case SCULLP_IOCQORDER: /* Query: return it (it's positive) */
		return scullp_order;

	case SCULLP_IOCXORDER: /* eXchange: use arg as pointer */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullp_order;
		retval = __get_user(scullp_order, (int __user *)arg);
		if (retval == 0)
			retval = __put_user(tmp, (int __user *)arg);
		break;

	case SCULLP_IOCHORDER: /* sHift: like Tell + Query */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullp_order;
		scullp_order = arg;
		return tmp;

	case SCULLP_IOCSQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(scullp_qset, (int __user *)arg);
		break;

	case SCULLP_IOCTQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		scullp_qset = arg;
		break;

	case SCULLP_IOCGQSET:
		retval = __put_user(scullp_qset, (int __user *)arg);
		break;

	case SCULLP_IOCQQSET:
		return scullp_qset;

	case SCULLP_IOCXQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullp_qset;
		retval = __get_user(scullp_qset, (int __user *)arg);
		if (retval == 0)
			retval = put_user(tmp, (int __user *)arg);
		break;

	case SCULLP_IOCHQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullp_qset;
		scullp_qset = arg;
		return tmp;

	default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	up(&dev->sem);
#endif

	return retval;
}

static struct file_operations scullp_fops = {
	.owner = THIS_MODULE,
	.open = scullp_open,
	.release = scullp_release,
	.read = scullp_read,
	.write = scullp_write,
	.llseek = scullp_llseek,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = scullp_ioctl
#else
	.unlocked_ioctl = scullp_ioctl
#endif
};

static void scullp_setup_cdev(struct scullp_dev *dev, int index)
{
	int err;
	dev_t devno;

	devno = MKDEV(scullp_major, scullp_minor + index);

	cdev_init(&dev->cdev, &scullp_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scullp%d\n", err, index);
}

static void scullp_exit(void)
{
	int i;
	dev_t dev = MKDEV(scullp_major, scullp_minor);

	if (scullp_devices) {
		for (i = 0; i < scullp_nr_devs; i++) {
			scullp_trim(scullp_devices + i);
			cdev_del(&scullp_devices[i].cdev);
		}
		kfree(scullp_devices);
	}
	unregister_chrdev_region(dev, scullp_nr_devs);
}

static int __init scullp_init(void)
{
	dev_t dev;
	int result, i;

	if (scullp_major) {
		dev = MKDEV(scullp_major, scullp_minor);
		result = register_chrdev_region(dev, scullp_nr_devs, "scullp");
	} else {
		result = alloc_chrdev_region(&dev, scullp_minor, scullp_nr_devs,
					     "scullp");
		scullp_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "scullp: can't get major %d\n", scullp_major);
		return result;
	}

	/*
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	scullp_devices = kmalloc(scullp_nr_devs * sizeof(struct scullp_dev), GFP_KERNEL);
	if (!scullp_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(scullp_devices, 0, scullp_nr_devs * sizeof(struct scullp_dev));

	/* Initialize each device. */
	for (i = 0; i < scullp_nr_devs; i++) {
		scullp_devices[i].qset = scullp_qset;
		scullp_devices[i].order = scullp_order;
		sema_init(&scullp_devices[i].sem, 1);
		scullp_setup_cdev(&scullp_devices[i], i);
	}

	return 0;
fail:
	scullp_exit();
	return result;
}

MODULE_LICENSE("GPL");
module_init(scullp_init);
module_exit(scullp_exit);
