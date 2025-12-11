// Driver for Nokia-7220-IXR-H6-128 Router
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

#define PORT_NUM  (128 + 2)  /*128 OSFPs + 2 SFP28s*/

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
    0x2100,// MESS_TOP_L_CPLD I2C Master OSFP Port1
    0x2120,// MESS_TOP_L_CPLD I2C Master OSFP Port2
    0x2100,// PORTCPLD0 I2C Master OSFP Port3
    0x2120,// PORTCPLD0 I2C Master OSFP Port4
    0x2140,// PORTCPLD0 I2C Master OSFP Port5
    0x2160,// PORTCPLD0 I2C Master OSFP Port6
    0x2100,// MESS_BOT_L_CPLD I2C Master OSFP Port7
    0x2120,// MESS_BOT_L_CPLD I2C Master OSFP Port8
    0x2140,// MESS_TOP_L_CPLD I2C Master OSFP Port9
    0x2160,// MESS_TOP_L_CPLD I2C Master OSFP Port10
    0x2180,// PORTCPLD0 I2C Master OSFP Port11
    0x21A0,// PORTCPLD0 I2C Master OSFP Port12
    0x21C0,// PORTCPLD0 I2C Master OSFP Port13
    0x21E0,// PORTCPLD0 I2C Master OSFP Port14
    0x2140,// MESS_BOT_L_CPLD I2C Master OSFP Port15
    0x2160,// MESS_BOT_L_CPLD I2C Master OSFP Port16
    0x2180,// MESS_TOP_L_CPLD I2C Master OSFP Port17
    0x21A0,// MESS_TOP_L_CPLD I2C Master OSFP Port18
    0x2200,// PORTCPLD0 I2C Master OSFP Port19
    0x2220,// PORTCPLD0 I2C Master OSFP Port20
    0x2240,// PORTCPLD0 I2C Master OSFP Port21
    0x2260,// PORTCPLD0 I2C Master OSFP Port22
    0x2180,// MESS_BOT_L_CPLD I2C Master OSFP Port23
    0x21A0,// MESS_BOT_L_CPLD I2C Master OSFP Port24
    0x21C0,// MESS_TOP_L_CPLD I2C Master OSFP Port25
    0x21E0,// MESS_TOP_L_CPLD I2C Master OSFP Port26
    0x2280,// PORTCPLD0 I2C Master OSFP Port27
    0x22A0,// PORTCPLD0 I2C Master OSFP Port28
    0x22C0,// PORTCPLD0 I2C Master OSFP Port29
    0x22E0,// PORTCPLD0 I2C Master OSFP Port30
    0x21C0,// MESS_BOT_L_CPLD I2C Master OSFP Port31
    0x21E0,// MESS_BOT_L_CPLD I2C Master OSFP Port32
////////////////////////////////////////////////////// 
    0x2200,// MESS_TOP_L_CPLD I2C Master OSFP Port33
    0x2220,// MESS_TOP_L_CPLD I2C Master OSFP Port34
    0x2300,// PORTCPLD0 I2C Master OSFP Port35
    0x2320,// PORTCPLD0 I2C Master OSFP Port36
    0x2340,// PORTCPLD0 I2C Master OSFP Port37
    0x2360,// PORTCPLD0 I2C Master OSFP Port38
    0x2200,// MESS_BOT_L_CPLD I2C Master OSFP Port39
    0x2220,// MESS_BOT_L_CPLD I2C Master OSFP Port40
    0x2240,// MESS_TOP_L_CPLD I2C Master OSFP Port41
    0x2260,// MESS_TOP_L_CPLD I2C Master OSFP Port42
    0x2380,// PORTCPLD0 I2C Master OSFP Port43
    0x23A0,// PORTCPLD0 I2C Master OSFP Port44
    0x23C0,// PORTCPLD0 I2C Master OSFP Port45
    0x23E0,// PORTCPLD0 I2C Master OSFP Port46
    0x2240,// MESS_BOT_L_CPLD I2C Master OSFP Port47
    0x2260,// MESS_BOT_L_CPLD I2C Master OSFP Port48
    0x2280,// MESS_TOP_L_CPLD I2C Master OSFP Port49
    0x22A0,// MESS_TOP_L_CPLD I2C Master OSFP Port50
    0x2400,// PORTCPLD0 I2C Master OSFP Port51
    0x2420,// PORTCPLD0 I2C Master OSFP Port52
    0x2440,// PORTCPLD0 I2C Master OSFP Port53
    0x2460,// PORTCPLD0 I2C Master OSFP Port54
    0x2280,// MESS_BOT_L_CPLD I2C Master OSFP Port55
    0x22A0,// MESS_BOT_L_CPLD I2C Master OSFP Port56
    0x22C0,// MESS_TOP_L_CPLD I2C Master OSFP Port57
    0x22E0,// MESS_TOP_L_CPLD I2C Master OSFP Port58
    0x2480,// PORTCPLD0 I2C Master OSFP Port59
    0x24A0,// PORTCPLD0 I2C Master OSFP Port60
    0x24C0,// PORTCPLD0 I2C Master OSFP Port61
    0x24E0,// PORTCPLD0 I2C Master OSFP Port62
    0x22C0,// MESS_BOT_L_CPLD I2C Master OSFP Port63
    0x22E0,// MESS_BOT_L_CPLD I2C Master OSFP Port64
//////////////////////////////////////////////////////   
    0x2100,// MESS_TOP_R_CPLD I2C Master OSFP Port65
    0x2120,// MESS_TOP_R_CPLD I2C Master OSFP Port66
    0x2100,// PORTCPLD1 I2C Master OSFP Port67
    0x2120,// PORTCPLD1 I2C Master OSFP Port68
    0x2140,// PORTCPLD1 I2C Master OSFP Port69
    0x2160,// PORTCPLD1 I2C Master OSFP Port70
    0x2100,// MESS_BOT_R_CPLD I2C Master OSFP Port71
    0x2120,// MESS_BOT_R_CPLD I2C Master OSFP Port72
    0x2140,// MESS_TOP_R_CPLD I2C Master OSFP Port73
    0x2160,// MESS_TOP_R_CPLD I2C Master OSFP Port74
    0x2180,// PORTCPLD1 I2C Master OSFP Port75
    0x21A0,// PORTCPLD1 I2C Master OSFP Port76
    0x21C0,// PORTCPLD1 I2C Master OSFP Port77
    0x21E0,// PORTCPLD1 I2C Master OSFP Port78
    0x2140,// MESS_BOT_R_CPLD I2C Master OSFP Port79
    0x2160,// MESS_BOT_R_CPLD I2C Master OSFP Port80
    0x2180,// MESS_TOP_R_CPLD I2C Master OSFP Port81
    0x21A0,// MESS_TOP_R_CPLD I2C Master OSFP Port82
    0x2200,// PORTCPLD1 I2C Master OSFP Port83
    0x2220,// PORTCPLD1 I2C Master OSFP Port84
    0x2240,// PORTCPLD1 I2C Master OSFP Port85
    0x2260,// PORTCPLD1 I2C Master OSFP Port86
    0x2180,// MESS_BOT_R_CPLD I2C Master OSFP Port87
    0x21A0,// MESS_BOT_R_CPLD I2C Master OSFP Port88
    0x21C0,// MESS_TOP_R_CPLD I2C Master OSFP Port89
    0x21E0,// MESS_TOP_R_CPLD I2C Master OSFP Port90
    0x2280,// PORTCPLD1 I2C Master OSFP Port91
    0x22A0,// PORTCPLD1 I2C Master OSFP Port92
    0x22C0,// PORTCPLD1 I2C Master OSFP Port93
    0x22E0,// PORTCPLD1 I2C Master OSFP Port94
    0x21C0,// MESS_BOT_R_CPLD I2C Master OSFP Port95
    0x21E0,// MESS_BOT_R_CPLD I2C Master OSFP Port96
//////////////////////////////////////////////////////     
    0x2200,// MESS_TOP_R_CPLD I2C Master OSFP Port97
    0x2220,// MESS_TOP_R_CPLD I2C Master OSFP Port98
    0x2300,// PORTCPLD1 I2C Master OSFP Port99
    0x2320,// PORTCPLD1 I2C Master OSFP Port100
    0x2340,// PORTCPLD1 I2C Master OSFP Port101
    0x2360,// PORTCPLD1 I2C Master OSFP Port102
    0x2200,// MESS_BOT_R_CPLD I2C Master OSFP Port103
    0x2220,// MESS_BOT_R_CPLD I2C Master OSFP Port104
    0x2240,// MESS_TOP_R_CPLD I2C Master OSFP Port105
    0x2260,// MESS_TOP_R_CPLD I2C Master OSFP Port106
    0x2380,// PORTCPLD1 I2C Master OSFP Port107
    0x23A0,// PORTCPLD1 I2C Master OSFP Port108
    0x23C0,// PORTCPLD1 I2C Master OSFP Port109
    0x23E0,// PORTCPLD1 I2C Master OSFP Port110
    0x2240,// MESS_BOT_R_CPLD I2C Master OSFP Port111
    0x2260,// MESS_BOT_R_CPLD I2C Master OSFP Port112
    0x2280,// MESS_TOP_R_CPLD I2C Master OSFP Port113
    0x22A0,// MESS_TOP_R_CPLD I2C Master OSFP Port114
    0x2400,// PORTCPLD1 I2C Master OSFP Port115
    0x2420,// PORTCPLD1 I2C Master OSFP Port116
    0x2440,// PORTCPLD1 I2C Master OSFP Port117
    0x2460,// PORTCPLD1 I2C Master OSFP Port118
    0x2280,// MESS_BOT_R_CPLD I2C Master OSFP Port119
    0x22A0,// MESS_BOT_R_CPLD I2C Master OSFP Port120
    0x22C0,// MESS_TOP_R_CPLD I2C Master OSFP Port121
    0x22E0,// MESS_TOP_R_CPLD I2C Master OSFP Port122
    0x2480,// PORTCPLD1 I2C Master OSFP Port123
    0x24A0,// PORTCPLD1 I2C Master OSFP Port124
    0x24C0,// PORTCPLD1 I2C Master OSFP Port125
    0x24E0,// PORTCPLD1 I2C Master OSFP Port126
    0x22C0,// MESS_BOT_R_CPLD I2C Master OSFP Port127
    0x22E0,// MESS_BOT_R_CPLD I2C Master OSFP Port128   
    0x2500,// MESS_BOT_R_CPLD I2C Master SFP Port1
    0x2520,// MESS_BOT_R_CPLD I2C Master SFP Port2
};

static struct ocores_i2c_platform_data i2c_data = {
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

    i2c_data.bus_khz = clamp_val(param_i2c_khz, 50, 400);
    for(i = 0; i < PORT_NUM; i++) {
        p = &myi2c[i];
        p->name                   = devname;
        p->id                     = i;
        p->dev.platform_data      = &i2c_data;
        p->dev.release = ftdi_release_platform_dev;
        res = &ocores_resources[i];
        switch (i)
        {
            case 0 ... 129:
                bar_base = pci_resource_start(pcidev, BAR0_NUM);
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
