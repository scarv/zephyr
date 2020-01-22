/**
 * @brief UART driver for the SCAR-V CPU
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <drivers/uart.h>

#define RXDATA_EMPTY   (1 << 31)   /* Receive FIFO Empty */
#define RXDATA_MASK    0xFF        /* Receive Data Mask */

#define TXDATA_FULL    (1 << 31)   /* Transmit FIFO Full */

struct uart_scarvsoc_regs_t {
	u32_t rx;
	u32_t tx;
	u32_t stat;
};

struct uart_scarvsoc_device_config {
	uintptr_t   port;
	u32_t       sys_clk_freq;
	u32_t       baud_rate;
};

struct uart_scarvsoc_data {
};

#define DEV_CFG(dev)						\
	((const struct uart_scarvsoc_device_config * const)	\
	 (dev)->config->config_info)
#define DEV_UART(dev)						\
	((struct uart_scarvsoc_regs_t *)(DEV_CFG(dev))->port)
#define DEV_DATA(dev)						\
	((struct uart_scarvsoc_data * const)(dev)->driver_data)

/**
 * @brief Output a character in polled mode.
 *
 * Writes data to tx register if transmitter is not full.
 *
 * @param dev UART device struct
 * @param c Character to send
 */
static void uart_scarvsoc_poll_out(struct device *dev,
					 unsigned char c)
{
	volatile struct uart_scarvsoc_regs_t *uart = DEV_UART(dev);

	/* Wait while TX FIFO is full */
	while (uart->tx & TXDATA_FULL) {
	}

	uart->tx = (int)c;
}

/**
 * @brief Poll the device for input.
 *
 * @param dev UART device struct
 * @param c Pointer to character
 *
 * @return 0 if a character arrived, -1 if the input buffer if empty.
 */
static int uart_scarvsoc_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uart_scarvsoc_regs_t *uart = DEV_UART(dev);
	u32_t val = uart->rx;

	if (val & RXDATA_EMPTY) {
		return -1;
	}

	*c = (unsigned char)(val & RXDATA_MASK);

	return 0;
}

static int uart_scarvsoc_init(struct device *dev)
{
	return 0;
}

static const struct uart_driver_api uart_scarvsoc_driver_api = {
	.poll_in          = uart_scarvsoc_poll_in,
	.poll_out         = uart_scarvsoc_poll_out,
	.err_check        = NULL,
};

#ifdef CONFIG_UART_SCARVSOC_PORT_0

static struct uart_scarvsoc_data uart_scarvsoc_data_0;

static const struct uart_scarvsoc_device_config uart_scarvsoc_dev_cfg_0 = {
	.port         = DT_INST_0_SCARVSOC_UART0_BASE_ADDRESS,
	.sys_clk_freq = DT_INST_0_SCARVSOC_UART0_CLOCK_FREQUENCY,
	.baud_rate    = DT_INST_0_SCARVSOC_UART0_CURRENT_SPEED,
};

DEVICE_AND_API_INIT(uart_scarvsoc_0, DT_INST_0_SCARVSOC_UART0_LABEL,
		    uart_scarvsoc_init,
		    &uart_scarvsoc_data_0, &uart_scarvsoc_dev_cfg_0,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_scarvsoc_driver_api);

#endif /* CONFIG_UART_SCARVSOC_PORT_0 */

#ifdef CONFIG_UART_SCARVSOC_PORT_1

static struct uart_scarvsoc_data uart_scarvsoc_data_1;

static const struct uart_scarvsoc_device_config uart_scarvsoc_dev_cfg_1 = {
	.port         = DT_INST_1_SCARVSOC_UART0_BASE_ADDRESS,
	.sys_clk_freq = DT_INST_1_SCARVSOC_UART0_CLOCK_FREQUENCY,
	.baud_rate    = DT_INST_1_SCARVSOC_UART0_CURRENT_SPEED,
};

DEVICE_AND_API_INIT(uart_scarvsoc_1, DT_INST_1_SCARVSOC_UART0_LABEL,
		    uart_scarvsoc_init,
		    &uart_scarvsoc_data_1, &uart_scarvsoc_dev_cfg_1,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&uart_scarvsoc_driver_api);

#endif /* CONFIG_UART_SCARVSOC_PORT_1 */
