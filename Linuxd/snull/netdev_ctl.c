#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/kernel.h> 
#include <linux/slab.h> 
#include <linux/errno.h>  
#include <linux/types.h>  
#include <linux/interrupt.h> 
#include <linux/in.h>
#include <linux/netdevice.h>   
#include <linux/etherdevice.h> 
#include <linux/ip.h>          
#include <linux/tcp.h>        
#include <linux/skbuff.h>
#include <linux/in6.h>
#include <asm/checksum.h>

#include "netdev_ctl.h"

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

static int lockup = 0;
static int timeout = SNULL_TIMEOUT;
static int use_napi = 0;
int pool_size = 8;
module_param(lockup, int, 0);
module_param(timeout, int, 0);
module_param(use_napi, int, 0);
module_param(pool_size, int, 0);

struct snull_packet {
	struct snull_packet *next;
	struct net_device *dev;
	int	datalen;
	u8 data[ETH_DATA_LEN];
};

struct snull_priv {
	struct net_device_stats stats;
	int status;
	struct snull_packet *ppool;
	struct snull_packet *rx_queue;
	int rx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
};

struct net_device *snull_devs[2];

static void snull_tx_timeout(struct net_device *dev);
static void (*snull_interrupt)(int, void *, struct pt_regs *);

void snull_setup_pool(struct net_device *dev)
{
	int i;
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof(struct snull_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}

void snull_teardown_pool(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
    
	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		kfree (pkt);
	}
}    

struct snull_packet *snull_get_tx_buffer(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	unsigned long flags;
	struct snull_packet *pkt;
    
	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->ppool;
	priv->ppool = pkt->next;
	if (priv->ppool == NULL) {
		printk (KERN_INFO "Pool empty\n");
		netif_stop_queue(dev);
	}
	spin_unlock_irqrestore(&priv->lock, flags);
	
	return pkt;
}

void snull_release_buf(struct snull_packet *pkt)
{
	unsigned long flags;
	struct snull_priv *priv = netdev_priv(pkt->dev);
	
	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
	
	if (netif_queue_stopped(pkt->dev) && (pkt->next == NULL))
		netif_wake_queue(pkt->dev);
}

void snull_enqueue_buf(struct net_device *dev, struct snull_packet *pkt)
{
	unsigned long flags;
	struct snull_priv *priv = netdev_priv(dev);

	spin_lock_irqsave(&priv->lock, flags);
	pkt->next = priv->rx_queue;
	priv->rx_queue = pkt;
	spin_unlock_irqrestore(&priv->lock, flags);
}

struct snull_packet *snull_dequeue_buf(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	pkt = priv->rx_queue;
	if (pkt != NULL)
		priv->rx_queue = pkt->next;
	spin_unlock_irqrestore(&priv->lock, flags);
	
	return pkt;
}

static void snull_rx_ints(struct net_device *dev, int enable)
{
	struct snull_priv *priv = netdev_priv(dev);
	
	priv->rx_int_enabled = enable;
}

int snull_open(struct net_device *dev)
{
	memcpy(dev->dev_addr, "\0SNUL0", ETH_ALEN);
	if (dev == snull_devs[1])
		dev->dev_addr[ETH_ALEN-1]++; 
	
	netif_start_queue(dev);
	
	return 0;
}

int snull_release(struct net_device *dev)
{
	netif_stop_queue(dev);
	
	return 0;
}

int snull_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) 
		return -EBUSY;

	if (map->base_addr != dev->base_addr) 
		return -EOPNOTSUPP;

	if (map->irq != dev->irq) 
		dev->irq = map->irq;

	return 0;
}

void snull_rx(struct net_device *dev, struct snull_packet *pkt)
{
	struct sk_buff *skb;
	struct snull_priv *priv = netdev_priv(dev);

	skb = dev_alloc_skb(pkt->datalen + 2);
	if (skb == NULL) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "snull rx: low on mem - packet dropped\n");

		priv->stats.rx_dropped++;
		return;
	}
	
	skb_reserve(skb, 2); 
	memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);
	skb->dev = dev;
	skb->protocol = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; 
	priv->stats.rx_packets++;
	priv->stats.rx_bytes += pkt->datalen;
	netif_rx(skb);
	
	return;
}
    
static int snull_poll(struct net_device *dev, int *budget)
{
	int npackets = 0, quota = min(dev->quota, *budget);
	struct sk_buff *skb;
	struct snull_priv *priv = netdev_priv(dev);
	struct snull_packet *pkt;
    
	while (npackets < quota && priv->rx_queue) {
		pkt = snull_dequeue_buf(dev);
		skb = dev_alloc_skb(pkt->datalen + 2);
		if (!skb) {
			if (printk_ratelimit())
				printk(KERN_NOTICE "snull: packet dropped\n");
			
			priv->stats.rx_dropped++;
			snull_release_buf(pkt);
			continue;
		}
		
		skb_reserve(skb, 2); 
		memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);
		skb->dev = dev;
		skb->protocol = eth_type_trans(skb, dev);
		skb->ip_summed = CHECKSUM_UNNECESSARY; 
		netif_receive_skb(skb);
		
		npackets++;
		priv->stats.rx_packets++;
		priv->stats.rx_bytes += pkt->datalen;
		snull_release_buf(pkt);
	}

	*budget -= npackets;
	dev->quota -= npackets;
	
	if (!priv->rx_queue) {
		netif_rx_complete(dev);
		snull_rx_ints(dev, 1);
		return 0;
	}

	return 1;
}
	    
static void snull_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	struct snull_priv *priv;
	struct snull_packet *pkt = NULL;
	struct net_device *dev;

	dev = (struct net_device *)dev_id;
	if (dev == NULL)
		return;

	priv = netdev_priv(dev);
	spin_lock(&priv->lock);
	statusword = priv->status;
	priv->status = 0;
	
	if (statusword & SNULL_RX_INTR) {
		pkt = priv->rx_queue;
		if (pkt) {
			priv->rx_queue = pkt->next;
			snull_rx(dev, pkt);
		}
	}
	
	if (statusword & SNULL_TX_INTR) {
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += priv->tx_packetlen;
		dev_kfree_skb(priv->skb);
	}

	spin_unlock(&priv->lock);
	
	if (pkt) 
		snull_release_buf(pkt); 
	
	return;
}

static void snull_napi_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	int statusword;
	struct snull_priv *priv;
	struct net_device *dev;

	dev = (struct net_device *)dev_id;
	if (dev == NULL) 
		return;

	priv = netdev_priv(dev);
	spin_lock(&priv->lock);
	statusword = priv->status;
	priv->status = 0;
	
	if (statusword & SNULL_RX_INTR) {
		snull_rx_ints(dev, 0);  
		netif_rx_schedule(dev);
	}
	
	if (statusword & SNULL_TX_INTR) {
		priv->stats.tx_packets++;
		priv->stats.tx_bytes += priv->tx_packetlen;
		dev_kfree_skb(priv->skb);
	}
	
	spin_unlock(&priv->lock);
	
	return;
}

static void snull_hw_tx(char *buf, int len, struct net_device *dev)
{
	struct iphdr *ih;
	struct net_device *dest;
	struct snull_priv *priv;
	u32 *saddr, *daddr;
	struct snull_packet *tx_buffer;
    
	if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
		printk("Packet too short (%i octets)\n", len);
		return;
	}

	ih = (struct iphdr *)(buf + sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	((u8 *)saddr)[2] ^= 1; 
	((u8 *)daddr)[2] ^= 1;

	ih->check = 0;       
	ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);

	if (dev == snull_devs[0])
		PDEBUGG("%08x:%05i --> %08x:%05i\n",
				ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source),
				ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest));
	else
		PDEBUGG("%08x:%05i <-- %08x:%05i\n",
				ntohl(ih->daddr),ntohs(((struct tcphdr *)(ih+1))->dest),
				ntohl(ih->saddr),ntohs(((struct tcphdr *)(ih+1))->source));

	dest = snull_devs[dev == snull_devs[0] ? 1 : 0];
	priv = netdev_priv(dest);
	tx_buffer = snull_get_tx_buffer(dev);
	tx_buffer->datalen = len;
	memcpy(tx_buffer->data, buf, len);
	snull_enqueue_buf(dest, tx_buffer);
	
	if (priv->rx_int_enabled) {
		priv->status |= SNULL_RX_INTR;
		snull_interrupt(0, dest, NULL);
	}

	priv = netdev_priv(dev);
	priv->tx_packetlen = len;
	priv->tx_packetdata = buf;
	priv->status |= SNULL_TX_INTR;
	
	if (lockup && ((priv->stats.tx_packets + 1) % lockup) == 0) {
		netif_stop_queue(dev);
		PDEBUG("Simulate lockup at %ld, txp %ld\n", jiffies, (unsigned long)priv->stats.tx_packets);
	} else {
		snull_interrupt(0, dev, NULL);
	}
}

int snull_tx(struct sk_buff *skb, struct net_device *dev)
{
	int len;
	char *data, shortpkt[ETH_ZLEN];
	struct snull_priv *priv = netdev_priv(dev);
	
	data = skb->data;
	len = skb->len;
	if (len < ETH_ZLEN) {
		memset(shortpkt, 0, ETH_ZLEN);
		memcpy(shortpkt, skb->data, skb->len);
		len = ETH_ZLEN;
		data = shortpkt;
	}
	
	dev->trans_start = jiffies; 
	priv->skb = skb;
	snull_hw_tx(data, len, dev);

	return 0; 
}

void snull_tx_timeout (struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);

	PDEBUG("Transmit timeout at %ld, latency %ld\n", jiffies, jiffies - dev->trans_start);

	priv->status = SNULL_TX_INTR;
	snull_interrupt(0, dev, NULL);
	priv->stats.tx_errors++;
	netif_wake_queue(dev);
	
	return;
}

int snull_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	PDEBUG("ioctl\n");
	
	return 0;
}

struct net_device_stats *snull_stats(struct net_device *dev)
{
	struct snull_priv *priv = netdev_priv(dev);
	
	return &priv->stats;
}

int snull_rebuild_header(struct sk_buff *skb)
{
	struct ethhdr *eth = (struct ethhdr *) skb->data;
	struct net_device *dev = skb->dev;
    
	memcpy(eth->h_source, dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1]   ^= 0x01;  
	
	return 0;
}

int snull_header(struct sk_buff *skb, struct net_device *dev,
                unsigned short type, void *daddr, void *saddr, unsigned int len)
{
	struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);

	eth->h_proto = htons(type);
	memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(eth->h_dest, daddr ? daddr : dev->dev_addr, dev->addr_len);
	eth->h_dest[ETH_ALEN-1] ^= 0x01;   
	
	return (dev->hard_header_len);
}

int snull_change_mtu(struct net_device *dev, int new_mtu)
{
	unsigned long flags;
	struct snull_priv *priv = netdev_priv(dev);
	spinlock_t *lock = &priv->lock;
    
	if ((new_mtu < 68) || (new_mtu > 1500))
		return -EINVAL;

	spin_lock_irqsave(lock, flags);
	dev->mtu = new_mtu;
	spin_unlock_irqrestore(lock, flags);
	
	return 0; 
}

void snull_creat(struct net_device *dev)
{
	struct snull_priv *priv;

	ether_setup(dev);

	dev->open            = 	snull_open;
	dev->stop            = 	snull_release;
	dev->set_config      = 	snull_config;
	dev->hard_start_xmit = 	snull_tx;
	dev->do_ioctl        = 	snull_ioctl;
	dev->get_stats       = 	snull_stats;
	dev->change_mtu      = 	snull_change_mtu;  
	dev->rebuild_header  = 	snull_rebuild_header;
	dev->hard_header     = 	snull_header;
	dev->tx_timeout      = 	snull_tx_timeout;
	dev->watchdog_timeo  = 	timeout;
	
	if (use_napi) {
		dev->poll        = 	snull_poll;
		dev->weight      = 	2;
	}
	
	dev->flags          |= 	IFF_NOARP;
	dev->features       |= 	NETIF_F_NO_CSUM;
	dev->hard_header_cache= NULL;      

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct snull_priv));
	spin_lock_init(&priv->lock);
	snull_rx_ints(dev, 1);		
	snull_setup_pool(dev);
}

void snull_cleanup(void)
{
	int i;
    
	for (i = 0; i < 2;  i++) {
		if (snull_devs[i]) {
			unregister_netdev(snull_devs[i]);
			snull_teardown_pool(snull_devs[i]);
			free_netdev(snull_devs[i]);
		}
	}
	
	return;
}

int snull_init(void)
{
	int  i, result = 0;

	snull_interrupt = use_napi ? snull_napi_interrupt : snull_regular_interrupt;

	snull_devs[0] = alloc_netdev(sizeof(struct snull_priv), "sn%d", snull_creat);
	snull_devs[1] = alloc_netdev(sizeof(struct snull_priv), "sn%d", snull_creat);
	if (snull_devs[0] == NULL || snull_devs[1] == NULL)
		goto out;

	for (i = 0; i < 2;  i++) {
		if ((result = register_netdev(snull_devs[i]))) {
			printk("snull: error %i registering device \"%s\"\n", result, snull_devs[i]->name);
		} else {
			result = 0;
		}
	}

out:
	if (result) 
		snull_cleanup();
	
	return result;
}

module_init(snull_init);
module_exit(snull_cleanup);
