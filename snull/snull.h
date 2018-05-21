#ifndef _SNULL_H_
#define _SNULL_H_

#define PDEBUG(fmt, args...) printk(KERN_DEBUG "snull: " fmt, ## args)
#define PDEBUGG(fmt, args...)

/* These are the flags in the statusword */
#define SNULL_RX_INTR 0x0001
#define SNULL_TX_INTR 0x0002

/* Default timeout period */
#define SNULL_TIMEOUT 5	/* In jiffies */

#endif /* _SNULL_H_ */
