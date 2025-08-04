// Driver for Nokia-7220-IXR-H6-64 Router
/*
 * Copyright (C) 2025 Accton Technology Corporation.
 * Copyright (C) 2025 Nokia Corporation. 
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * see <http://www.gnu.org/licenses/>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/platform_data/i2c-ocores.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/mfd/core.h>
#include <linux/pci.h>

#define PORT_NUM  (64 + 2)  /*64 OSFPs + 2 SFP28s*/

/*
 * PCIE BAR0 address
 */
#define BAR0_NUM                       0
#define BAR1_NUM                       1
#define BAR2_NUM                       2
#define REGION_LEN                     0xFF
#define FPGA_PCI_VENDOR_ID             0x10ee
#define FPGA_PCI_DEVICE_ID             0x7021

/* CPLD 1 */
#define CPLD1_PCIE_START_OFFSET        0x2000

/* CPLD 2 */
#define CPLD2_PCIE_START_OFFSET        0x3000

static uint param_i2c_khz = 400;
module_param(param_i2c_khz, uint, S_IRUGO);
MODULE_PARM_DESC(param_i2c_khz, "Target clock speed of i2c bus, in KHz.");

static struct pci_dev *g_pcidev = NULL;

static const uint adapt_offset[PORT_NUM]= {
    0x2100,// CPLD1 I2C Master OSFP Port0
    0x2120,// CPLD1 I2C Master OSFP Port1
    0x2140,// CPLD1 I2C Master OSFP Port2
    0x2160,// CPLD1 I2C Master OSFP Port3
    0x2180,// CPLD1 I2C Master OSFP Port4
    0x21A0,// CPLD1 I2C Master OSFP Port5
    0x21C0,// CPLD1 I2C Master OSFP Port6
    0x21E0,// CPLD1 I2C Master OSFP Port7
    0x2200,// CPLD1 I2C Master OSFP Port8
    0x2220,// CPLD1 I2C Master OSFP Port9
    0x2240,// CPLD1 I2C Master OSFP Port10
    0x2260,// CPLD1 I2C Master OSFP Port11
    0x2280,// CPLD1 I2C Master OSFP Port12
    0x22A0,// CPLD1 I2C Master OSFP Port13
    0x22C0,// CPLD1 I2C Master OSFP Port14
    0x22E0,// CPLD1 I2C Master OSFP Port15
    0x3100,// CPLD2 I2C Master OSFP Port16
    0x3120,// CPLD2 I2C Master OSFP Port17
    0x3140,// CPLD2 I2C Master OSFP Port18
    0x3160,// CPLD2 I2C Master OSFP Port19
    0x3180,// CPLD2 I2C Master OSFP Port20
    0x31A0,// CPLD2 I2C Master OSFP Port21
    0x31C0,// CPLD2 I2C Master OSFP Port22
    0x31E0,// CPLD2 I2C Master OSFP Port23
    0x3200,// CPLD2 I2C Master OSFP Port24
    0x3220,// CPLD2 I2C Master OSFP Port25
    0x3240,// CPLD2 I2C Master OSFP Port26
    0x3260,// CPLD2 I2C Master OSFP Port27
    0x3280,// CPLD2 I2C Master OSFP Port28
    0x32A0,// CPLD2 I2C Master OSFP Port29
    0x32C0,// CPLD2 I2C Master OSFP Port30
    0x32E0,// CPLD2 I2C Master OSFP Port31
    0x2300,// CPLD1 I2C Master OSFP Port32
    0x2320,// CPLD1 I2C Master OSFP Port33
    0x2340,// CPLD1 I2C Master OSFP Port34
    0x2360,// CPLD1 I2C Master OSFP Port35
    0x2380,// CPLD1 I2C Master OSFP Port36
    0x23A0,// CPLD1 I2C Master OSFP Port37
    0x23C0,// CPLD1 I2C Master OSFP Port38
    0x23E0,// CPLD1 I2C Master OSFP Port39
    0x2400,// CPLD1 I2C Master OSFP Port40
    0x2420,// CPLD1 I2C Master OSFP Port41
    0x2440,// CPLD1 I2C Master OSFP Port42
    0x2460,// CPLD1 I2C Master OSFP Port43
    0x2480,// CPLD1 I2C Master OSFP Port44
    0x24A0,// CPLD1 I2C Master OSFP Port45
    0x24C0,// CPLD1 I2C Master OSFP Port46
    0x24E0,// CPLD1 I2C Master OSFP Port47
    0x3300,// CPLD2 I2C Master OSFP Port48
    0x3320,// CPLD2 I2C Master OSFP Port49
    0x3340,// CPLD2 I2C Master OSFP Port50
    0x3360,// CPLD2 I2C Master OSFP Port51
    0x3380,// CPLD2 I2C Master OSFP Port52
    0x33A0,// CPLD2 I2C Master OSFP Port53
    0x33C0,// CPLD2 I2C Master OSFP Port54
    0x33E0,// CPLD2 I2C Master OSFP Port55
    0x3400,// CPLD2 I2C Master OSFP Port56
    0x3420,// CPLD2 I2C Master OSFP Port57
    0x3440,// CPLD2 I2C Master OSFP Port58
    0x3460,// CPLD2 I2C Master OSFP Port59
    0x3480,// CPLD2 I2C Master OSFP Port60
    0x34A0,// CPLD2 I2C Master OSFP Port61
    0x34C0,// CPLD2 I2C Master OSFP Port62
    0x34E0,// CPLD2 I2C Master OSFP Port63
    0x2500,// CPLD2 I2C Master SFP-28 Port64
    0x2520,// CPLD2 I2C Master SFP-28 Port65
};

static struct ocores_i2c_platform_data h6_64 = {
    .reg_shift = 2,
    .clock_khz = 25000,
    .bus_khz = 400,
    .devices = NULL,  //trcv_nvm,
    .num_devices = 1  //ARRAY_SIZE(trcv_nvm)
};

static void ftdi_release_platform_dev(struct device *dev)
{
    dev->parent = NULL;
}

static struct platform_device myi2c[PORT_NUM] = {{0}};
static int __init h6_ocore_i2c_init(void)
{
    int i, err = 0;
    static const char *devname = "ocores-i2c";
    static struct resource ocores_resources[PORT_NUM] = {0};
    struct pci_dev *pcidev;
    int status = 0;
    unsigned long bar_base;
    struct resource *res;
    struct platform_device *p = NULL;

    pcidev = pci_get_device(FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID, NULL);
     if (!pcidev) {
        pr_err("Cannot found PCI device(%x:%x)\n",
                     FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID);
        return -ENODEV;
    }

    g_pcidev = pcidev;

    err = pci_enable_device(pcidev);
    if (err != 0) {
        pr_err("Cannot enable PCI device(%x:%x)\n",
                     FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID);
        status = -ENODEV;
        goto exit_pci_put;
    }
    /* enable PCI bus-mastering */
    pci_set_master(pcidev);

    status = pci_enable_msi(pcidev);
    if (status < 0) {
        pr_err("Failed to allocate IRQ vectors: %d\n", status);
        goto exit_pci_disable;
    }

    h6_64.bus_khz = clamp_val(param_i2c_khz, 50, 400);
    for(i = 0; i < PORT_NUM; i++) {
        p = &myi2c[i];
        p->name                   = devname;
        p->id                     = i;
        p->dev.platform_data      = &h6_64;
        p->dev.release = ftdi_release_platform_dev;
        res = &ocores_resources[i];
        switch (i)
        {
            // Nokia TH6 has three BAR address
            case 0 ... 15:
                bar_base = pci_resource_start(pcidev, BAR1_NUM);
                break;
            case 16 ... 31:
                bar_base = pci_resource_start(pcidev, BAR2_NUM);
                break;
            case 32 ... 47:
                bar_base = pci_resource_start(pcidev, BAR1_NUM);
                break;
            case 48 ... 63:
                bar_base = pci_resource_start(pcidev, BAR2_NUM);
                break;
            case 64 ... 65:
                bar_base = pci_resource_start(pcidev, BAR1_NUM);
                break;
            default:
                break;
        }
        res->start = bar_base + adapt_offset[i];
        res->end = res->start + 0x20 - 1;
        res->name = NULL;
        res->flags =IORESOURCE_MEM;
        res->desc = IORES_DESC_NONE;
        p->num_resources          = 1;
        p->resource               = res;
        err = platform_device_register(p);
        if (err)
            goto unload;
    }

    return 0;

unload:
    {
        int j;
        pr_err("[ERROR]rc:%d, unload %u register devices\n", err, i);
        for(j = 0; j < i; j++) {
            platform_device_unregister(&myi2c[j]);
        }
    }
    pci_disable_msi(pcidev);

exit_pci_disable:
    pci_disable_device(pcidev);

exit_pci_put:
    pci_dev_put(pcidev);
    g_pcidev = NULL;

    return status ? status : err;
}

static void __exit h6_ocore_i2c_exit(void)
{
    int i;
    for(i = PORT_NUM ; i > 0; i--) {
        platform_device_unregister(&myi2c[i-1]);
    }
    if (g_pcidev) {
        pci_disable_msi(g_pcidev);
        pci_disable_device(g_pcidev);
        pci_dev_put(g_pcidev);
        g_pcidev = NULL;
    }
}

module_init(h6_ocore_i2c_init);
module_exit(h6_ocore_i2c_exit);

MODULE_AUTHOR("Roy Lee <roy_lee@accton.com.tw>");
MODULE_DESCRIPTION("h6 ocore_i2c platform device driver");
MODULE_LICENSE("GPL");
