#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <linux/inet.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laible");
MODULE_DESCRIPTION("Virtual network interface, testing challange");
MODULE_VERSION("1.0");

char vnet_ip_char[16];
__be32 vnet_ip_be32;

static struct net_device *vnet_dev;
static struct proc_dir_entry *vnet_proc_dir;

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
    if (skb == NULL) return NETDEV_TX_OK;
 
    // Код вызывающий ошибку
    // struct ethhdr *eth = (struct ethhdr*)skb->data;
    // struct iphdr  *iph = (struct iphdr*)(skb->data + sizeof(struct ethhdr));
    // if (iph) 
    // {
    //     printk(KERN_INFO "vnet: SRC: %pI4, DST: %pI4", &iph->saddr, &iph->daddr);
    // }

    printk(KERN_INFO "vnet: skb len = %d, data first 34 bytes:", skb->len);
    print_hex_dump(KERN_INFO, "vnet: ", DUMP_PREFIX_OFFSET, 16, 1, skb->data, min((unsigned)skb->len, (unsigned int)34), true);

    kfree_skb(skb);
    return NETDEV_TX_OK;
}

static const struct net_device_ops vnet_ops = 
{
    .ndo_open = vnet_open,
    .ndo_stop = vnet_close,
    .ndo_start_xmit = start_ximit
};

static ssize_t vnet_proc_read(struct file *fd, char __user *buf, size_t count, loff_t *pos) 
{
    int len = 0;
    if (*pos > 0) return 0;
    len = strlen(vnet_ip_char);
    if (copy_to_user(buf, vnet_ip_char, len) != 0) return -EFAULT;
    *pos += len;
    return len;
}

static ssize_t vnet_proc_write(struct file *fd, const char __user *buf, size_t count, loff_t *pos) 
{
    if (*pos > 0) return 0;
    if (copy_from_user(vnet_ip_char, buf, count) != 0) return -EFAULT;
    in4_pton(vnet_ip_char, -1, (u8 *)&vnet_ip_be32, '\0', NULL);
    return count;
}

static const struct proc_ops vnet_proc_ops = 
{
    .proc_read = vnet_proc_read,
    .proc_write = vnet_proc_write
};

static int __init vnet_module_init(void) 
{
    int res_register_netdev = 0;
    printk(KERN_INFO "vnet: module loaded\n");
    vnet_dev = alloc_netdev(0, "vnet%d", NET_NAME_ENUM, ether_setup);
    if (vnet_dev == NULL) 
    {
        printk(KERN_ERR "vnet: alloc_netdev return NULL");
        return -ENOMEM;
    }

    vnet_dev->netdev_ops = &vnet_ops;
    res_register_netdev = register_netdev(vnet_dev);
    if (res_register_netdev < 0) 
    {
        printk(KERN_ERR "vnet: register_netdev return error %d\n", res_register_netdev);
        return -1;
    }

    vnet_proc_dir = proc_create("vnet", 0660, NULL, &vnet_proc_ops);

    return 0;
}

static void __exit vnet_module_exit(void) 
{
    printk(KERN_INFO "vnet: module unloaded\n");
    unregister_netdev(vnet_dev);
    proc_remove(vnet_proc_dir);
    free_netdev(vnet_dev);
}

module_init(vnet_module_init);
module_exit(vnet_module_exit);