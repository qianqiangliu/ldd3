#ifndef _SCULLP_H_
#define _SCULLP_H_

#ifdef __KERNEL__
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#endif /* __KERNEL__ */

#define SCULLP_MAJOR		0
#define SCULLP_NR_DEVS		4
#define SCULLP_QSET		1024
#define SCULLP_ORDER		0

/* ioctl commands */
#define SCULLP_IOC_MAGIC		'l'

#define SCULLP_IOCRESET		_IO(SCULLP_IOC_MAGIC, 0)

#define SCULLP_IOCSORDER	_IOW(SCULLP_IOC_MAGIC, 1, int)
#define SCULLP_IOCSQSET		_IOW(SCULLP_IOC_MAGIC, 2, int)
#define SCULLP_IOCTORDER	_IO(SCULLP_IOC_MAGIC, 3)
#define SCULLP_IOCTQSET		_IO(SCULLP_IOC_MAGIC, 4)
#define SCULLP_IOCGORDER	_IOR(SCULLP_IOC_MAGIC, 5, int)
#define SCULLP_IOCGQSET		_IOR(SCULLP_IOC_MAGIC, 6, int)
#define SCULLP_IOCQORDER	_IO(SCULLP_IOC_MAGIC, 7)
#define SCULLP_IOCQQSET		_IO(SCULLP_IOC_MAGIC, 8)
#define SCULLP_IOCXORDER	_IOWR(SCULLP_IOC_MAGIC, 9, int)
#define SCULLP_IOCXQSET		_IOWR(SCULLP_IOC_MAGIC, 10, int)
#define SCULLP_IOCHORDER	_IO(SCULLP_IOC_MAGIC, 11)
#define SCULLP_IOCHQSET		_IO(SCULLP_IOC_MAGIC, 12)

#define SCULLP_IOC_MAXNR		14

#ifdef __KERNEL__
struct scullp_qset {
	void **data;
	struct scullp_qset *next;
};

struct scullp_dev {
	struct scullp_qset *data;	/* 指向第一个量子集的指针 */
	int qset;			/* 当前数组的大小 */
	int order;
	unsigned long size;		/* 保存在其中的数据总量 */
	unsigned int access_key;	/* 由scullpuid和scullppriv使用 */
	struct semaphore sem;		/* 互斥信号量 */
	struct cdev cdev;		/* 字符设备结构 */
};
#endif /* __KERNEL__ */

#endif /* _SCULLP_H_ */
