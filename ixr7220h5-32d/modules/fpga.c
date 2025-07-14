// SPDX-License-Identifier: GPL-2.0-only
/*
* FPGA driver
* 
* Copyright (C) 2024 Nokia Corporation.
* Copyright (C) 2024 Delta Networks, Inc.
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
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include "fpga.h"
#include "fpga_attr.h"
#include "fpga_gpio.h"
#include "fpga_i2c.h"
#include "fpga_reg.h"

#define FPGA_PCA9548
#define FPGA_GPIO
#define FPGA_ATTR

#if 0
struct file_operations fpga_fileops = {
    .owner = THIS_MODULE,
    .read = fpga_read,
    .write = fpga_write,
    .mmap = fpga_mmap,
    /*.ioctl = fpga_ioctl,*/
    //	.open = fpga_open,
    .release = fpga_close,
};
#endif

static int sys_fpga_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    struct fpga_dev *fpga=NULL;
    int error = -1;
    int ret=0;

    dev_info(&dev->dev, "probe");

    fpga = kzalloc(sizeof(struct fpga_dev), GFP_KERNEL);
    if (!fpga)
    {
        dev_warn(&dev->dev, "Couldn't allocate memory for fpga!\n");
        ret = -ENOMEM;
        goto err_out_kfree;
    }
    fpga->dev = dev;
    fpga->buffer = kmalloc(BUF_SIZE * sizeof(char), GFP_KERNEL);
    if (!fpga->buffer)
    {
        dev_warn(&dev->dev, "Couldn't allocate memory for buffer!\n");
        ret = -ENOMEM;
        goto err_out_kfree;
    }

    /* enabling the device */
    ret = pci_enable_device(dev);
    if (ret)
    {
        dev_warn(&dev->dev, "Failed to enable PCI device.\n");
        goto err_out_kfree;
    }

    ret = pci_request_regions(dev, "sys-fpga");
    if (ret)
    {
        dev_warn(&dev->dev, "Failed to request PCI region.\n");
        goto err_out_disable;
    }

    if (!(pci_resource_flags(dev, 0) & IORESOURCE_MEM))
    {
        dev_warn(&dev->dev, "Incorrect BAR configuration.\n");
        ret = -ENODEV;
        goto err_out_disable;
    }
    
#ifdef FPGA_GPIO
    /* init gpiodev */
    ret = gpiodev_init(dev, fpga);
    if (ret)
    {
        dev_warn(&dev->dev, "Couldn't create gpiodev!\n");
        goto err_out_disable;
    }
#endif

    ret = i2c_adapter_init(dev, fpga);
    if (ret)
    {
        dev_warn(&dev->dev, "Couldn't create i2c_adapter!\n");
        goto err_out_release_region;
    }
#ifdef FPGA_ATTR
    /* init fpga attr */
    ret = fpga_attr_init(dev, fpga);
    if (ret)
    {
        dev_warn(&dev->dev, "Couldn't init fpga attr!\n");
        goto err_out_release_region;
    }
#endif
    return 0;

err_out_release_region:
    if(fpga->pci_base && fpga->pci_size)
        release_region(fpga->pci_base, fpga->pci_size);
err_out_disable:
    pci_disable_device(dev);
err_out_kfree:
    if( fpga && fpga->buffer)
        kfree((fpga->buffer));
    if(fpga)
        kfree((fpga));
    return error;
}

static void sys_fpga_remove(struct pci_dev *dev)
{
    /* release fpga-i2c */
    int i = 0;
    struct fpga_dev *fpga = pci_get_drvdata(dev);
    dev_info(&dev->dev, "fpga = 0x%lx\n", (unsigned long)fpga);

    for (i = 0; i < num_i2c_adapter; i++)
    {
        i2c_del_adapter(&(fpga->i2c + i)->adapter);
        dev_info(&dev->dev, "remove - FPGA-I2C-%d\n", i);
    }
#ifdef FPGA_GPIO
    /* remove gpio */
    gpiodev_exit(dev);
#endif
    /* release pci */
    pci_disable_device(dev);
    pci_release_regions(dev);
    /*release attribute */
#ifdef FPGA_ATTR    
    fpga_attr_exit();
#endif    
if (fpga && fpga->i2c)
        kfree(fpga->i2c);
    if( fpga && fpga->buffer)
        kfree((fpga->buffer));
    if(fpga)
        kfree((fpga));
    printk(KERN_INFO "Goodbye\n");
}

enum chiptype
{
    LATTICE
};

static const struct of_device_id sys_fpga_of_match[] = {
    {.compatible = "sys-fpga,fpga-i2c"},
    {/* sentinel */}};

static const struct pci_device_id sys_fpga_ids[] = {
    {PCI_DEVICE(0x1204, 0x9c1d)},
    {0},
};

MODULE_DEVICE_TABLE(pci, sys_fpga_ids);

static struct pci_driver sys_fpga_driver = {
    .name = "sys-fpga",
    .id_table = sys_fpga_ids,
    .probe = sys_fpga_probe,
    .remove = sys_fpga_remove,
};

static int sys_fpga_init(void)
{
    printk(KERN_INFO "sys-fpga-init\n");
    return pci_register_driver(&sys_fpga_driver);
}

static void sys_fpga_exit(void)
{
    pci_unregister_driver(&sys_fpga_driver);
    printk(KERN_INFO "sys-fpga-exit\n");
}

MODULE_AUTHOR("amos.lin@deltaww.com");
MODULE_DESCRIPTION("Sys-FPGA Driver");
MODULE_LICENSE("GPL");

module_init(sys_fpga_init);
module_exit(sys_fpga_exit);
