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

#define MODULE_NAME     "cpuctl"
#define PCI_VENDOR_ID_NOKIA 0x1064
#define PCI_DEVICE_ID_NOKIA_IOCTL 0x001a
#define PCI_DEVICE_ID_NOKIA_CPUCTL 0x001d
#define PCI_DEVICE_ID_NOKIA_CPUCTL_HORNET 0x0025
#define PCI_DEVICE_ID_NOKIA_CPUCTL_VERMILION 0x0030
#define PCI_DEVICE_ID_NOKIA_IOCTL_VERMILION 0x0033

#define CPUCTL_NUM_MEM_REGIONS 1
#define CPUCTL_MINORS_MAX 1

typedef struct
{
	struct list_head list;
	struct pci_dev *pcidev;
	struct i2c_adapter adapter;
	struct i2c_mux_core *ctlmuxcore;
	const struct attribute_group *sysfs;
	u8 phys_chan;
	s8 current_modsel;
	struct ctlvariant *ctlv;
	int minor;
	int enabled;
	void __iomem *base;
	u8 reset_list[36];
	spinlock_t lock;
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
	u16 ctl_type;
	u16 devid;
	u16 nchans;
	struct chan_map * pchanmap;
	u32 bus400;
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

struct opi {
	struct list_head        list;
	CTLDEV                 *pdev;
	pid_t                   pid;
	wait_queue_head_t       wait_queue;
	int                     minor;
};

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

#define NUM_JER_ASICS 2
#define CTL_CNTR_STA    0x00800000
#define CTL_DMA_INT1    0x00800008
#define CTL_DMA_INT2    0x00800018
#define CTL_ETH_RST     0x00800070
#define CTL_CARD_TYPE   0x008000E0
#define CTL_SCRATCH_PAD 0x00800500
#define CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE   (0x00807D80)
#define CTL_BDB_CNTR    0x02700000
#define CTL_BDB_SLOT    0x02700004
// #define CTL_MISC_IO     0x02700008
// #define CTL_MISC_IO_DIR 0x0270000C
#define CTL_MISC_IO3_DAT     0x02700050
#define CTL_MISC_IO3_ENA     0x02700054
#define CTL_MISC_IO4_DAT     0x02700040
#define CTL_MISC_IO4_ENA     0x02700044
#define CTL_BDB_SIGDET  0x02700010
#define CTL_BDB_ERRDET  0x02700014

#define CTL_A32_MISCIO2_DATA            0x02700048
#define FPGA_A32_CODE_VER               0x00800070
#define IO_A32_PORT_MOD_ABS_BASE        0x00807D00
#define IO_A32_PORT_MOD_RST_BASE        0x00807D40
#define IO_A32_PORT_MOD_LPMODE_BASE     0x00807D60

#define MISCIO3_IO_VERM_JER0_SYS_RST_BIT        (1 << 0)
#define MISCIO3_IO_VERM_JER1_SYS_RST_BIT        (1 << 1)
#define MISCIO3_IO_VERM_JER0_SYS_PCI_BIT        (1 << 2)
#define MISCIO3_IO_VERM_JER1_SYS_PCI_BIT        (1 << 3)

#define MISCIO4_IO_VERM_IMM_PLL_RST_N_BIT       (1 << 24)
#define MISCIO4_IO_VERM_IMM_PLL2_RST_N_BIT      (1 << 25)

#define CTL_MAX_I2C_CHANS   32

#endif /* CPUCTL_MOD_H */
