// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia cpctl/ioctl i2c bus adapter/multiplexer
 *
 *  Copyright (C) 2024 Nokia
 *
 */

#ifndef CPUCTL_MOD_H
#define CPUCTL_MOD_H

#include <linux/pci.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/spi/spi.h>

#define MODULE_NAME     "cpuctl"
#define PCI_VENDOR_ID_NOKIA 0x1064
#define PCI_DEVICE_ID_NOKIA_IOCTL 0x001a
#define PCI_DEVICE_ID_NOKIA_CPUCTL 0x001d
#define PCI_DEVICE_ID_NOKIA_CPUCTL_HORNET 0x0025
#define PCI_DEVICE_ID_NOKIA_CPUCTL_VERMILION 0x0030
#define PCI_DEVICE_ID_NOKIA_IOCTL_VERMILION 0x0033

#define CPUCTL_NUM_MEM_REGIONS 1
#define CPUCTL_MINORS_MAX 1
#define N_SPI_MINORS 4
#define CTL_MAX_I2C_CHANS   64
#define CTL_THROTTLE_MIN 5
#define CTL_THROTTLE_MAX 30
typedef struct
{
	struct list_head list;
	struct pci_dev *pcidev;
	struct i2c_adapter adapter;
	struct i2c_mux_core *ctlmuxcore;
	const struct attribute_group *sysfs;
	struct _chan_stats {
		u64 last_xfer;
		u32 backoff_cnt;
		u32 throttle_cnt;
		u8 throttle_min;
	}chan_stats[CTL_MAX_I2C_CHANS];
	u8 phys_chan;
	u8 virt_chan;
	s8 current_modsel;
	u8 modsel_active;
	struct ctlvariant *ctlv;
	int minor;
	int enabled;
	void __iomem *base;
	spinlock_t lock;
	struct {
		spinlock_t lock;
		struct spi_controller spis[N_SPI_MINORS];
	}spi;
} CTLDEV;

struct ctlmux {
	u32 last_chan;
	struct i2c_client *client;
	CTLDEV *pdev;
};

struct chan_map {
	u8 phys_chan;
	s8 modsel;
};

struct ctlvariant {
	const char *name;
	u8 num_asics;
	u8 num_asic_if;
	u8 spi_bus;
	u16 ctl_type;
	u16 devid;
	u16 nchans;
	struct chan_map * pchanmap;
	u32 bus400;
	u32 miscio1_oe;    // output enable
	u32 miscio3_oe;    // output enable
	u32 miscio4_oe;    // output enable
};

enum ctl_type {
	ctl_cp,
	ctl_io,
	ctl_cp_hornet,
	ctl_cp_vermilion,
	ctl_io_vermilion,
};

/* module_param */
extern uint board;
enum brd_type {
	brd_x3b=0,
	brd_x1b,
	brd_x4,
	brd_max
};

/* module_param */
extern uint debug;
#define CTL_DEBUG_I2C 0x0001
#define CTL_DEBUG_SPI 0x0002

static inline int ctlv_is_cp(enum ctl_type t) {
	switch (t) {
		case ctl_cp:
		case ctl_cp_hornet:
		case ctl_cp_vermilion:
			return 1;
		default:
			return 0;
	}
	return 0;
}

int ctl_i2c_probe(CTLDEV *pdev);
void ctl_i2c_remove(CTLDEV *pdev);

int ctl_sysfs_init(CTLDEV *pdev);
void ctl_sysfs_remove(CTLDEV *pdev);

static inline u32 ctl_reg_read(CTLDEV *p, unsigned offset)
{
	volatile void __iomem *addr = (p->base + offset);
	return swab32(readl(addr));
}
static inline void ctl_reg_write(CTLDEV *p, unsigned offset, u32 value)
{
	volatile void __iomem *addr = (p->base + offset);
	writel(swab32(value), addr);
}
static inline u32 ctl_reg16_read(CTLDEV *p, unsigned offset)
{
	volatile void __iomem *addr = (p->base + offset);
	return swab16(readw(addr));
}
static inline void ctl_reg16_write(CTLDEV *p, unsigned offset, u16 value)
{
	volatile void __iomem *addr = (p->base + offset);
	writew(swab16(value), addr);
}
static inline u64 ctl_reg64_read(CTLDEV *p, unsigned offset)
{
	volatile void __iomem *addr = (p->base + offset);
	return swab64(readq(addr));
}
static inline void ctl_reg64_write(CTLDEV *p, unsigned offset, u32 value)
{
	volatile void __iomem *addr = (p->base + offset);
	writeq(swab64(value), addr);
}
static inline u8 ctl_reg8_read(CTLDEV *p, unsigned offset)
{
	volatile void __iomem *addr = (p->base + offset);
	return readb(addr);
}
static inline void ctl_reg8_write(CTLDEV *p, unsigned offset, u8 value)
{
	volatile void __iomem *addr = (p->base + offset);
	writeb(value, addr);
}

#define MAX_NUM_JER_ASICS 2
#define CTL_CNTR_STA    0x00800000
#define CTL_DMA_INT1    0x00800008
#define CTL_DMA_INT2    0x00800018
#define CTL_ETH_RST     0x00800070
#define CTL_CARD_TYPE   0x008000E0
#define CTL_SCRATCH_PAD 0x00800500
#define CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE   (0x00807D80)
#define CTL_BDB_CNTR    0x02700000
#define CTL_BDB_SLOT    0x02700004
#define CTL_MISC_IO1_DAT 0x02700008
#define CTL_MISC_IO1_ENA 0x0270000C
#define CTL_MISC_IO3_DAT     0x02700050
#define CTL_MISC_IO3_ENA     0x02700054
#define CTL_MISC_IO4_DAT     0x02700040
#define CTL_MISC_IO4_ENA     0x02700044
#define CTL_BDB_SIGDET  0x02700010
#define CTL_BDB_ERRDET  0x02700014
#define A32_SPI_DATA_SR (0x014FFFF8)
#define A32_SPI_CTRL_SR (0x014FFFFC)

#define A32_GEN_CONFIG              0x02700000
#define A32_CP_MISCIO1_DATA            (A32_GEN_CONFIG+0x08)  /* I/O bits */
#define A32_IO_MISCIO1_DATA            (A32_GEN_CONFIG+0x08)  /* I/O bits */
#define A32_IO_MISCIO4_DATA            (A32_GEN_CONFIG+0x40)  /* I/O bits */
#define A32_CP_MISCIO4_DATA            (A32_GEN_CONFIG+0x40)  /* I/O bits */
#define A32_CP_MISCIO4_ENABLE          (A32_GEN_CONFIG+0x44)  /* Bit set, pin is an output*/
#define A32_MISCIO0_ENABLE          (A32_GEN_CONFIG+0x0C)  /* Bit set, pin is an output*/
#define A32_IO_MISCIO5_DATA            (A32_GEN_CONFIG+0x58)
#define A32_IO_MISCIO5_ENA             (A32_GEN_CONFIG+0x5c)
#define A32_MACSEC_SELECT_FIJI                    A32_MISCIO5_IOM_FIJI_DATA
#define A32_CTRLSTAT_MSW         0x00800000  // 32-bits (msw) of 64-bit ctrl status
#define A32_CTRLSTAT_LSW         0x00800004  // 32-bits (lsw) of 64-bit ctrl status

#define CTL_A32_CP_MISCIO2_DATA         0x02700048
#define CTL_A32_IO_MISCIO2_DATA         0x02700048
#define Ctl_A32_LED_STATE_BASE          0x02700140
#define FPGA_A32_CODE_VER               0x00800070
#define IO_A32_PORT_MOD_ABS_BASE        0x00807D00
#define IO_A32_PORT_MOD_RST_BASE        0x00807D40
#define IO_A32_PORT_MOD_LPMODE_BASE     0x00807D60
#define IO_A8_LED_STATE_BASE            0x02700080

#define M_MISCIO2_IO_VERM_JER0_AVS      0xff

#define MISCIO1_CP_VERM_SETS_RST_BIT        (1 << 19)

#define MISCIO3_IO_VERM_JER0_SYS_RST_BIT        (1 << 0)
#define MISCIO3_IO_VERM_JER1_SYS_RST_BIT        (1 << 1)
#define MISCIO3_IO_VERM_JER0_SYS_PCI_BIT        (1 << 2)
#define MISCIO3_IO_VERM_JER1_SYS_PCI_BIT        (1 << 3)

#define MISCIO4_IO_VERM_IMM_RGB_RST_N_BIT       (0xff << 16)
#define MISCIO4_IO_VERM_IMM_PLL_RST_N_BIT       (1 << 24)
#define MISCIO4_IO_VERM_IMM_PLL2_RST_N_BIT      (1 << 25)

#define IS_HW_CARD_TYPE_HORNET_R2 0

extern void ctl_clk_reset(CTLDEV * pdev);

extern int spi_device_create(CTLDEV* pdev);
extern void spi_device_remove(CTLDEV* pdev);

#endif /* CPUCTL_MOD_H */
