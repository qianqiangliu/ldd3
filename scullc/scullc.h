#ifndef _SCULLC_H_
#define _SCULLC_H_

#ifdef __KERNEL__
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#endif /* __KERNEL__ */

#define SCULLC_MAJOR		0
#define SCULLC_NR_DEVS		4
#define SCULLC_QUANTUM		4096
#define SCULLC_QSET		1024

/* ioctl commands */
#define SCULLC_IOC_MAGIC		'l'

#define SCULLC_IOCRESET		_IO(SCULLC_IOC_MAGIC, 0)

#define SCULLC_IOCSQUANTUM	_IOW(SCULLC_IOC_MAGIC, 1, int)
#define SCULLC_IOCSQSET		_IOW(SCULLC_IOC_MAGIC, 2, int)
#define SCULLC_IOCTQUANTUM	_IO(SCULLC_IOC_MAGIC, 3)
#define SCULLC_IOCTQSET		_IO(SCULLC_IOC_MAGIC, 4)
#define SCULLC_IOCGQUANTUM	_IOR(SCULLC_IOC_MAGIC, 5, int)
#define SCULLC_IOCGQSET		_IOR(SCULLC_IOC_MAGIC, 6, int)
#define SCULLC_IOCQQUANTUM	_IO(SCULLC_IOC_MAGIC, 7)
#define SCULLC_IOCQQSET		_IO(SCULLC_IOC_MAGIC, 8)
#define SCULLC_IOCXQUANTUM	_IOWR(SCULLC_IOC_MAGIC, 9, int)
#define SCULLC_IOCXQSET		_IOWR(SCULLC_IOC_MAGIC, 10, int)
#define SCULLC_IOCHQUANTUM	_IO(SCULLC_IOC_MAGIC, 11)
#define SCULLC_IOCHQSET		_IO(SCULLC_IOC_MAGIC, 12)

#define SCULLC_IOC_MAXNR		14

#ifdef __KERNEL__
struct scullc_qset {
	void **data;
	struct scullc_qset *next;
};

struct scullc_dev {
	struct scullc_qset *data;	/* 指向第一个量子集的指针 */
	int quantum;			/* 当前量子的大小 */
	int qset;			/* 当前数组的大小 */
	unsigned long size;		/* 保存在其中的数据总量 */
	unsigned int access_key;	/* 由scullcuid和scullcpriv使用 */
	struct semaphore sem;		/* 互斥信号量 */
	struct cdev cdev;		/* 字符设备结构 */
};
#endif /* __KERNEL__ */

#endif /* _SCULLC_H_ */
