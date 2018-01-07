#include <linux/module.h>
#include <linux/netdevice.h>

static int __init net_init(void)
{
	struct net_device *net;

	net = dev_get_by_name(&init_net, "eth0");
	if (!net) {
		printk(KERN_ERR "can not get eth0 dev\n");
		return -1;
	}
	printk("name: %s, ifindex: %d\n", net->name, net->ifindex);

	net = dev_get_by_index(&init_net, 1);
	if (!net) {
		printk(KERN_ERR "can not get 1 ifindex dev\n");
		return -1;
	}
	printk("name: %s, ifindex: %d\n", net->name, net->ifindex);

	net = first_net_device(&init_net);
	while (net) {
		if (netif_running(net))
			printk("net is up\n");
		else
			printk("net is down\n");
		printk("name: %s, ifindex: %d\n", net->name, net->ifindex);
		net = next_net_device(net);
	}
	return 0;
}

static void __exit net_exit(void)
{
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(net_init);
module_exit(net_exit);
