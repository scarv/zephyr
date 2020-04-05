#define LOG_MODULE_NAME eth_liteeth
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <stdbool.h>
#include <net/ethernet.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <sys/printk.h>

/* flags */
#define ETH_SCARVSOC_EV_TX		0x1
#define ETH_SCARVSOC_EV_TXED    0x0
#define ETH_SCARVSOC_EV_RX		0x0
#define ETH_SCARVSOC_EV_RXED    0x1

/* sram - transmit */
#define ETH_SCARVSOC_TX_BASE		DT_INST_0_SCARVSOC_ETH0_TRANSMIT_BASE_ADDRESS
#define ETH_SCARVSOC_TX_DEST_ADDR   ((ETH_SCARVSOC_TX_BASE) + 0x00)
#define ETH_SCARVSOC_TX_SRC_ADDR    ((ETH_SCARVSOC_TX_BASE) + 0x06)
#define ETH_SCARVSOC_TX_TYPE        ((ETH_SCARVSOC_TX_BASE) + 0x12)
#define ETH_SCARVSOC_TX_DATA        ((ETH_SCARVSOC_TX_BASE) + 0x14)
#define ETH_SCARVSOC_TX_LENGTH      ((ETH_SCARVSOC_TX_BASE) + 0x07F4)
#define ETH_SCARVSOC_TX_GIE         ((ETH_SCARVSOC_TX_BASE) + 0x07F8)
#define ETH_SCARVSOC_TX_CONTROL     ((ETH_SCARVSOC_TX_BASE) + 0x07FC)

/* sram - receive */
#define ETH_SCARVSOC_RX_BASE		DT_INST_0_SCARVSOC_ETH0_RECEIVE_BASE_ADDRESS
#define ETH_SCARVSOC_RX_DEST_ADDR   ((ETH_SCARVSOC_RX_BASE) + 0x00)
#define ETH_SCARVSOC_RX_SRC_ADDR    ((ETH_SCARVSOC_RX_BASE) + 0x06)
#define ETH_SCARVSOC_RX_TYPE        ((ETH_SCARVSOC_RX_BASE) + 0x12)
#define ETH_SCARVSOC_RX_DATA        ((ETH_SCARVSOC_RX_BASE) + 0x14)
#define ETH_SCARVSOC_RX_CRC         ((ETH_SCARVSOC_RX_BASE) + 0x07F4)
#define ETH_SCARVSOC_RX_CONTROL     ((ETH_SCARVSOC_RX_BASE) + 0x07F8)

/* label */
#define ETH_SCARVSOC_LABEL		    DT_INST_0_SCARVSOC_ETH0_LABEL

struct eth_liteeth_dev_data {
	struct net_if *iface;
	u8_t mac_addr[6];
};

static int eth_initialize(struct device *dev)
{
	//const struct eth_liteeth_config *config = dev->config->config_info;

	//config->config_func();

	return 0;
}

static int eth_tx(struct device *dev, struct net_pkt *pkt)
{
	int key;
	u16_t len;
	//struct eth_liteeth_dev_data *context = dev->driver_data;

	key = irq_lock();

	/* get data from packet and send it */
	len = net_pkt_get_len(pkt);
	net_pkt_read(pkt, ETH_SCARVSOC_TX_DATA, len);

	sys_write8(len >> 8, SCARVSOC_TX_LENGTH + 8);
	sys_write8(len & 0xFF, SCARVSOC_TX_LENGTH + 12);

	/* wait for the device to be ready to transmit */
	while (sys_read8(SCARVSOC_TX_CONTROL) == ETH_SCARVSOC_EV_TX) {
		;
	}

	/* start transmitting */
	sys_write8(ETH_SCARVSOC_EV_TC, SCARVSOC_TX_CONTROL);

	irq_unlock(key);

	return 0;
}

static void eth_rx(struct device *port)
{
	struct net_pkt *pkt;
	struct eth_liteeth_dev_data *context = port->driver_data;

	unsigned int key, r;
	u16_t len = 0;

	key = irq_lock();

	/* get frame's length */
	for (int i = 0; i < 2; i++) {
		len <<= 8;
		len |= sys_read8(SCARVSOC_RX_TYPE + i * 0x4);
	}

	/* obtain rx buffer */
	pkt = net_pkt_rx_alloc_with_buffer(context->iface, len, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("Failed to obtain RX buffer");
		goto out;
	}

	/* copy data to buffer */
	if (net_pkt_write(pkt, (void *)ETH_SCARVSOC_RX_DATA, len) != 0) {
		LOG_ERR("Failed to append RX buffer to context buffer");
		net_pkt_unref(pkt);
		goto out;
	}

	/* receive data */
	r = net_recv_data(context->iface, pkt);
	if (r < 0) {
		LOG_ERR("Failed to enqueue frame into RX queue: %d", r);
		net_pkt_unref(pkt);
	}

out:
	irq_unlock(key);
}

#ifdef CONFIG_ETH_SCARVSOC_RANDOM_MAC
static void generate_mac(u8_t *mac_addr)
{
	u32_t entropy;

	entropy = sys_rand32_get();

	mac_addr[3] = entropy >> 8;
	mac_addr[4] = entropy >> 16;
	/* Locally administered, unicast */
	mac_addr[5] = ((entropy >> 0) & 0xfc) | 0x02;
}
#endif

#ifdef CONFIG_ETH_SCARVSOC

static struct eth_liteeth_dev_data eth_data = {
	.mac_addr =  DT_INST_0_SCARVSOC_ETH0_LOCAL_MAC_ADDRESS
};

static void eth_iface_init(struct net_if *iface)
{
	struct device *port = net_if_get_device(iface);
	struct eth_liteeth_dev_data *context = port->driver_data;
	static bool init_done;

	/* initialize only once */
	if (init_done) {
		return;
	}

	/* set interface */
	context->iface = iface;

	/* initialize ethernet L2 */
	ethernet_init(iface);

#ifdef CONFIG_ETH_SCARVSOC_RANDOM_MAC
	/* generate random MAC address */
	generate_mac(context->mac_addr);
#endif

	/* set MAC address */
	net_if_set_link_addr(iface, context->mac_addr, sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	/* clear pending events */
	sys_write8(SCARVSOC_EV_TXED, SCARVSOC_TX_CONTROL);
	sys_write8(SCARVSOC_EV_RXED, SCARVSOC_RX_CONTROL);

	init_done = true;
}

static enum ethernet_hw_caps eth_caps(struct device *dev)
{
	ARG_UNUSED(dev);
	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T |
	       ETHERNET_LINK_1000BASE_T;
}

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_caps,
	.send = eth_tx
};

NET_DEVICE_INIT(eth0, ETH_SCARVSOC_LABEL, eth_initialize, &eth_data, &eth_config,
		CONFIG_ETH_INIT_PRIORITY, &eth_api, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);

#endif /* CONFIG_ETH_SCARVSOC */
