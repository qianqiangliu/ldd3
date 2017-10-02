#ifndef _SCULL_H_
#define _SCULL_H_

#ifdef __KERNEL__
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#endif /* __KERNEL__ */

#define SCULL_MAJOR		0
#define SCULL_NR_DEVS		4
#define SCULL_QUANTUM		4096
#define SCULL_QSET		1024

/* ioctl commands */
#define SCULL_IOC_MAGIC		'l'

#define SCULL_IOCRESET		_IO(SCULL_IOC_MAGIC, 0)

#define SCULL_IOCSQUANTUM	_IOW(SCULL_IOC_MAGIC, 1, int)
#define SCULL_IOCSQSET		_IOW(SCULL_IOC_MAGIC, 2, int)
#define SCULL_IOCTQUANTUM	_IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET		_IO(SCULL_IOC_MAGIC, 4)
#define SCULL_IOCGQUANTUM	_IOR(SCULL_IOC_MAGIC, 5, int)
#define SCULL_IOCGQSET		_IOR(SCULL_IOC_MAGIC, 6, int)
#define SCULL_IOCQQUANTUM	_IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET		_IO(SCULL_IOC_MAGIC, 8)
#define SCULL_IOCXQUANTUM	_IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET		_IOWR(SCULL_IOC_MAGIC, 10, int)
#define SCULL_IOCHQUANTUM	_IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET		_IO(SCULL_IOC_MAGIC, 12)

#define SCULL_IOC_MAXNR		14

#ifdef __KERNEL__
struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;	/* 指向第一个量子集的指针 */
	int quantum;			/* 当前量子的大小 */
	int qset;			/* 当前数组的大小 */
	unsigned long size;		/* 保存在其中的数据总量 */
	unsigned int access_key;	/* 由sculluid和scullpriv使用 */
	struct semaphore sem;		/* 互斥信号量 */
	struct cdev cdev;		/* 字符设备结构 */
};
#endif /* __KERNEL__ */

#endif /* _SCULL_H_ */
