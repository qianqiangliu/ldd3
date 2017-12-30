#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/version.h>

#include "scullc.h"

static int scullc_major = SCULLC_MAJOR;
static int scullc_minor = 0;
static int scullc_nr_devs = SCULLC_NR_DEVS;
static int scullc_quantum = SCULLC_QUANTUM;
static int scullc_qset = SCULLC_QSET;
static struct kmem_cache *scullc_cache;

module_param(scullc_major, int, S_IRUGO);
module_param(scullc_minor, int, S_IRUGO);
module_param(scullc_nr_devs, int, S_IRUGO);
module_param(scullc_quantum, int, S_IRUGO);
module_param(scullc_qset, int, S_IRUGO);

static struct scullc_dev *scullc_devices;	/* allocated in scullc_init_module */

static int scullc_trim(struct scullc_dev *dev)
{
	struct scullc_qset *next, *dptr;
	int qset = dev->qset;
	int i;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			for (i = 0; i < qset; i++) {
				if (dptr->data[i])
					kmem_cache_free(scullc_cache, dptr->data[i]);
			}
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = scullc_quantum;
	dev->qset = scullc_qset;
	dev->data = NULL;
	return 0;
}

static int scullc_open(struct inode *inode, struct file *filp)
{
	struct scullc_dev *dev;

	printk(KERN_DEBUG "in scullc_open\n");

	dev = container_of(inode->i_cdev, struct scullc_dev, cdev);
	filp->private_data = dev;

	/* now trim to 0 the length of the device if open was write-only */
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		scullc_trim(dev);
		up(&dev->sem);
	}
	return 0;
}

static int scullc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * Follow the list
 */
static struct scullc_qset *scullc_follow(struct scullc_dev *dev, int n)
{
	struct scullc_qset *qs = dev->data;

	/* Allocate first qset explicitly if need be */
	if (!qs) {
		qs = dev->data = kmalloc(sizeof(struct scullc_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;  /* Never mind */
		memset(qs, 0, sizeof(struct scullc_qset));
	}

	/* Then follow the list */
	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct scullc_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;  /* Never mind */
			memset(qs->next, 0, sizeof(struct scullc_qset));
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

static ssize_t scullc_read(struct file *filp, char __user *buf, size_t count,
			  loff_t *f_pos)
{
	struct scullc_dev *dev = filp->private_data;
	struct scullc_qset *dptr;	/* the first listitem */
	int quantum = dev->quantum, qset = dev->qset;
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
	dptr = scullc_follow(dev, item);

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

static ssize_t scullc_write(struct file *filp, const char __user *buf, size_t count,
			   loff_t *f_pos)
{
	struct scullc_dev *dev = filp->private_data;
	struct scullc_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
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
	dptr = scullc_follow(dev, item);
	if (dptr == NULL)
		goto out;
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmem_cache_alloc(scullc_cache, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto out;
		memset(dptr->data[s_pos], 0, scullc_quantum);
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

static loff_t scullc_llseek(struct file *filp, loff_t off, int whence)
{
	struct scullc_dev *dev = filp->private_data;
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
int scullc_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg)
#else
long scullc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	int err = 0, tmp;
	int retval = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	struct scullc_dev *dev = filp->private_data;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
#endif

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != SCULLC_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULLC_IOC_MAXNR) return -ENOTTY;

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
	case SCULLC_IOCRESET:
		scullc_quantum = SCULLC_QUANTUM;
		scullc_qset = SCULLC_QSET;
		break;

	case SCULLC_IOCSQUANTUM: /* Set: arg points to the value */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(scullc_quantum, (int __user *)arg);
		break;

	case SCULLC_IOCTQUANTUM: /* Tell: arg is the value */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		scullc_quantum = arg;
		break;

	case SCULLC_IOCGQUANTUM: /* Get: arg is pointer to result */
		retval = __put_user(scullc_quantum, (int __user *)arg);
		break;

	case SCULLC_IOCQQUANTUM: /* Query: return it (it's positive) */
		return scullc_quantum;

	case SCULLC_IOCXQUANTUM: /* eXchange: use arg as pointer */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullc_quantum;
		retval = __get_user(scullc_quantum, (int __user *)arg);
		if (retval == 0)
			retval = __put_user(tmp, (int __user *)arg);
		break;

	case SCULLC_IOCHQUANTUM: /* sHift: like Tell + Query */
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullc_quantum;
		scullc_quantum = arg;
		return tmp;

	case SCULLC_IOCSQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		retval = __get_user(scullc_qset, (int __user *)arg);
		break;

	case SCULLC_IOCTQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		scullc_qset = arg;
		break;

	case SCULLC_IOCGQSET:
		retval = __put_user(scullc_qset, (int __user *)arg);
		break;

	case SCULLC_IOCQQSET:
		return scullc_qset;

	case SCULLC_IOCXQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullc_qset;
		retval = __get_user(scullc_qset, (int __user *)arg);
		if (retval == 0)
			retval = put_user(tmp, (int __user *)arg);
		break;

	case SCULLC_IOCHQSET:
		if (!capable(CAP_SYS_ADMIN))
			return -EPERM;
		tmp = scullc_qset;
		scullc_qset = arg;
		return tmp;

	default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
	up(&dev->sem);
#endif

	return retval;
}

static struct file_operations scullc_fops = {
	.owner = THIS_MODULE,
	.open = scullc_open,
	.release = scullc_release,
	.read = scullc_read,
	.write = scullc_write,
	.llseek = scullc_llseek,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = scullc_ioctl
#else
	.unlocked_ioctl = scullc_ioctl
#endif
};

static void scullc_setup_cdev(struct scullc_dev *dev, int index)
{
	int err;
	dev_t devno;

	devno = MKDEV(scullc_major, scullc_minor + index);

	cdev_init(&dev->cdev, &scullc_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scullc%d\n", err, index);
}

static void scullc_exit(void)
{
	int i;
	dev_t dev = MKDEV(scullc_major, scullc_minor);

	if (scullc_devices) {
		for (i = 0; i < scullc_nr_devs; i++) {
			scullc_trim(scullc_devices + i);
			cdev_del(&scullc_devices[i].cdev);
		}
		kfree(scullc_devices);
	}

	if (scullc_cache)
		kmem_cache_destroy(scullc_cache);

	unregister_chrdev_region(dev, scullc_nr_devs);
}

static int __init scullc_init(void)
{
	dev_t dev;
	int result, i;

	if (scullc_major) {
		dev = MKDEV(scullc_major, scullc_minor);
		result = register_chrdev_region(dev, scullc_nr_devs, "scullc");
	} else {
		result = alloc_chrdev_region(&dev, scullc_minor, scullc_nr_devs,
					     "scullc");
		scullc_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "scullc: can't get major %d\n", scullc_major);
		return result;
	}

	/*
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	scullc_devices = kmalloc(scullc_nr_devs * sizeof(struct scullc_dev), GFP_KERNEL);
	if (!scullc_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(scullc_devices, 0, scullc_nr_devs * sizeof(struct scullc_dev));

	scullc_cache = kmem_cache_create("scullc", scullc_quantum, 0,
					 SLAB_HWCACHE_ALIGN, NULL);
	if (!scullc_cache) {
		result = -ENOMEM;
		goto fail;
	}

	/* Initialize each device. */
	for (i = 0; i < scullc_nr_devs; i++) {
		scullc_devices[i].quantum = scullc_quantum;
		scullc_devices[i].qset = scullc_qset;
		sema_init(&scullc_devices[i].sem, 1);
		scullc_setup_cdev(&scullc_devices[i], i);
	}

	return 0;
fail:
	scullc_exit();
	return result;
}

MODULE_LICENSE("GPL");
module_init(scullc_init);
module_exit(scullc_exit);
