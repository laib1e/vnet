#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <linux/etherdevice.h>
#include <linux/proc_fs.h>
#include <linux/inet.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Laible");
MODULE_DESCRIPTION("Virtual network interface");
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
    struct ethhdr  *eth;
    struct iphdr   *iph;
    struct icmphdr *icmph;

    eth = (struct ethhdr *)skb->data;
    if (ntohs(eth->h_proto) != ETH_P_IP) 
        goto drop;

    iph = (struct iphdr*)(skb->data + sizeof(struct ethhdr));
    if (iph->protocol != IPPROTO_ICMP)
        goto drop;

    icmph = (struct icmphdr *)(skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr));
    if (icmph->type != ICMP_ECHO || iph->daddr != vnet_ip_be32)
        goto drop;

    __be32 temp_addr;
    unsigned char temp_eth_addr[ETH_ALEN];

    skb->ip_summed = CHECKSUM_UNNECESSARY;
    ether_addr_copy(temp_eth_addr, eth->h_source);
    ether_addr_copy(eth->h_source, eth->h_dest);
    ether_addr_copy(eth->h_dest, temp_eth_addr);

    temp_addr = iph->saddr;
    iph->saddr = iph->daddr;
    iph->daddr = temp_addr;

    icmph->type = ICMP_ECHOREPLY;
    icmph->checksum = 0;
    icmph->checksum = ip_compute_csum(icmph, ntohs(iph->tot_len) - sizeof(struct iphdr));

    iph->check = 0;
    iph->check = ip_fast_csum((unsigned char*)iph, iph->ihl);

    // See the solution to bug #3
    skb->dev = dev; 
    skb_reset_mac_header(skb);
    skb_pull(skb, sizeof(struct ethhdr));
    skb_reset_network_header(skb);
    // See the solution to bug #3
    
    skb->pkt_type = PACKET_HOST;
    skb->protocol = htons(ETH_P_IP);
    netif_rx(skb);
    return NETDEV_TX_OK;

    drop:
        printk(KERN_INFO "vnet: drop packet");
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

    if (count >= sizeof(vnet_ip_char))
        goto error;

    if (copy_from_user(vnet_ip_char, buf, count) != 0) 
        goto error;

    if (count > 0 && vnet_ip_char[count - 1] == '\n') 
        vnet_ip_char[count - 1] = '\0';

    if (in4_pton(vnet_ip_char, -1, (u8 *)&vnet_ip_be32, '\0', NULL) == 0)
        goto error;

    return count;

    error:
        printk(KERN_ERR "vnet: vnet_proc_write");
        vnet_ip_char[0] = '\0';
        return -EINVAL;
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

    vnet_dev->flags |= IFF_NOARP; // See the solution to bug #1
    vnet_dev->netdev_ops = &vnet_ops;
    res_register_netdev = register_netdev(vnet_dev);
    if (res_register_netdev < 0) 
    {
        printk(KERN_ERR "vnet: register_netdev return error %d\n", res_register_netdev);
        free_netdev(vnet_dev);
        return res_register_netdev;
    }

    vnet_proc_dir = proc_create("vnet", 0660, NULL, &vnet_proc_ops);
    if (vnet_proc_dir == NULL) 
    {
        printk(KERN_ERR "vnet: proc_create return NULL");
        unregister_netdev(vnet_dev);
        free_netdev(vnet_dev);
        return -ENOMEM;
    }

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