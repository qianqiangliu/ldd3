#include <linux/module.h>
#include <linux/errno.h>
#include <linux/netdevice.h>

MODULE_LICENSE("GPL");

static struct net_device *sg_dev;

static int virnet_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;
	dev_kfree_skb(skb);
	return 0;
}

static int virnet_open(struct net_device *dev)
{
	printk(KERN_DEBUG "netdev_open\n");
	memcpy(dev->dev_addr, "\x00\x01\x02\x03\x04\x05", ETH_ALEN);
	return 0;
}

static int virnet_release(struct net_device *dev)
{
	printk(KERN_DEBUG "netdev_release\n");
	return 0;
}

static struct net_device_ops sg_ops = {
	.ndo_start_xmit = virnet_send_packet,
	.ndo_open = virnet_open,
	.ndo_stop = virnet_release
};

static int virnet_init(void)
{
    int result, ret = -ENOMEM;

    sg_dev = alloc_netdev(0, "mon", ether_setup);
    if (sg_dev == NULL)
        goto out;

	sg_dev->netdev_ops = &sg_ops;

    ret = -ENODEV;
    if ((result = register_netdev(sg_dev))) {
        printk("error %i registering device \"%s\"\n", result, sg_dev->name);
        free_netdev(sg_dev);
    } else {
        ret = 0;
    }

out:
    return ret;
}

static void virnet_exit(void)
{
	if (sg_dev) {
		unregister_netdev(sg_dev);
		free_netdev(sg_dev);
	}
}

module_init(virnet_init);
module_exit(virnet_exit);
