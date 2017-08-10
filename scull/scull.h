#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/semaphore.h>
#include <linux/cdev.h>

#define SCULL_MAJOR	0
#define SCULL_NR_DEVS	4
#define SCULL_QUANTUM	4096
#define SCULL_QSET	1024

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

#endif /* _SCULL_H_ */
