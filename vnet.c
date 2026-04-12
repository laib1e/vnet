#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laible");
MODULE_DESCRIPTION("Virtual network interface, testing challange");
MODULE_VERSION("1.0");

struct net_device *vnet_dev;

static int vnet_open(struct net_device *dev) 
{
    return 0;
}

static int vnet_close(struct net_device *dev) 
{
    return 0;
}

static netdev_tx_t start_ximit(struct sk_buff *skb, struct net_device *dev) 
{
    kfree_skb(skb);
    return NETDEV_TX_OK;
}

static const struct net_device_ops vnet_ops = 
{
    .ndo_open = vnet_open,
    .ndo_stop = vnet_close,
    .ndo_start_xmit = start_ximit
};

static int __init vnet_module_init(void) 
{
    printk(KERN_INFO "vnet: module loaded\n");
    vnet_dev = alloc_netdev(0, "vnet%d", NET_NAME_ENUM, ether_setup);
    if (vnet_dev == NULL) 
    {
        printk(KERN_ERR "vnet: alloc_netdev return NULL");
        return -ENOMEM;
    }

    vnet_dev->netdev_ops = &vnet_ops;
    int res = register_netdev(vnet_dev);
    if (res > 0) 
    {
        printk(KERN_ERR "vnet: register_netdev return error %d\n", res);
        return -1;
    }

    return 0;
}

static void __exit vnet_module_exit(void) 
{
    printk(KERN_INFO "vnet: module unloaded\n");
    unregister_netdev(vnet_dev);
    free_netdev(vnet_dev);
}

module_init(vnet_module_init);
module_exit(vnet_module_exit);