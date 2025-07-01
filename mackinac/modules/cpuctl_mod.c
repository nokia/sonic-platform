// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia cpctl/ioctl i2c bus adapter/multiplexer
 *
 *  Copyright (C) 2024 Nokia
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/dmapool.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include "cpuctl.h"

static int ctl_probe(struct pci_dev *pcidev, const struct pci_device_id *id);
static void ctl_remove(struct pci_dev *pcidev);

uint board = brd_x3b;
module_param_named(board, board, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(board,
	" enum: x3b=0 (default), x1b=1, x4=2\n"
);
uint debug = 0;
module_param_named(debug, debug, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug,
	" bitmask\n"
	"  0x0001 i2c\n"
	"  0x0002 spi\n"
);

static CTLDEV *ctl_dev_alloc(void)
{
	CTLDEV *pdev;
	pdev = kmalloc(sizeof(CTLDEV), GFP_KERNEL);
	if (pdev)
	{
		memset(pdev, 0, sizeof(CTLDEV));
		INIT_LIST_HEAD(&pdev->list);
		spin_lock_init(&pdev->lock);
		spin_lock_init(&pdev->spi.lock);
	}
	return pdev;
}

static void ctl_dev_free(CTLDEV *pdev)
{
	list_del(&pdev->list);
	kfree(pdev);
}

static struct chan_map ctl_cp_vermilion_chanmap[] = {
	{0, -1},{1, -1},{2, -1},{3, -1},
	{4, -1},{5, -1},{6, -1},{7, -1},
	{8, -1},{9, -1},{19, -1}
};

static struct chan_map ctl_io_vermilion_chanmap[] = {
	{0, -1},
	{1, -1},
	{2, -1},
	{3, -1},
	{5, -1},
	{6, -1},
	{7, -1},
	{8, -1},
	{9, 0},{9, 1},{9, 2},{9, 3},{9, 4},{9, 5},
	{10, 6},{10, 7},{10, 8},{10, 9},{10, 10},{10, 11},
	{11, 12},{11, 13},{11, 14},{11, 15},{11, 16},{11, 17},
	{12, 18},{12, 19},{12, 20},{12, 21},{12, 22},{12, 23},
	{13, 24},{13, 25},{13, 26},{13, 27},{13, 28},{13, 29},
	{14, 30},{14, 31},{14, 32},{14, 33},{14, 34},{14, 35},
};

static struct ctlvariant ctls[] = {
	[ctl_cp] = {
		.ctl_type = ctl_cp,
		.pchanmap = NULL,
		.nchans = 0, /* tbd */
		.devid = PCI_DEVICE_ID_NOKIA_CPUCTL,
		.name = "ctl_cp",
		.miscio3_oe = 0x00000000,
	},
	[ctl_io] = {
		.ctl_type = ctl_io,
		.pchanmap = NULL,
		.nchans = 0, /* tbd */
		.devid = PCI_DEVICE_ID_NOKIA_IOCTL,
		.name = "ctl_io",
		.miscio3_oe = 0x00000000,
	},
	[ctl_cp_hornet] = {
		.ctl_type = ctl_cp_hornet,
		.pchanmap = NULL,
		.nchans = 0, /* tbd */
		.devid = PCI_DEVICE_ID_NOKIA_CPUCTL_HORNET,
		.name = "ctl_cp_hornet",
		.miscio3_oe = 0x00000000,
	},
	[ctl_cp_vermilion] = {
		.ctl_type = ctl_cp_vermilion,
		.pchanmap = ctl_cp_vermilion_chanmap,
		.nchans = sizeof(ctl_cp_vermilion_chanmap) / sizeof(struct chan_map),
		.bus400 = 0x040a,
		.spi_bus = 0,
		.devid = PCI_DEVICE_ID_NOKIA_CPUCTL_VERMILION,
		.name = "ctl_cp_vermilion",
		.miscio1_oe = 0x08080200,
		.miscio3_oe = 0x00000000,
		.miscio4_oe = 0x00000000,
	},
	[ctl_io_vermilion] = {
		.ctl_type = ctl_io_vermilion,
		.pchanmap = ctl_io_vermilion_chanmap,
		.nchans = sizeof(ctl_io_vermilion_chanmap)/sizeof(struct chan_map),
		.bus400 = 0x7eef,
		.spi_bus = 1,
		.devid = PCI_DEVICE_ID_NOKIA_IOCTL_VERMILION,
		.name = "ctl_io_vermilion",
		.miscio3_oe = 0x0000000f,
		.miscio4_oe = 0xffff0000,
	},
};

static struct pci_device_id ctl_ids[] = {
	{PCI_DEVICE_DATA(NOKIA, CPUCTL, &ctls[ctl_cp])},
	{PCI_DEVICE_DATA(NOKIA, IOCTL, &ctls[ctl_io])},
	{PCI_DEVICE_DATA(NOKIA, CPUCTL_HORNET, &ctls[ctl_cp_hornet])},
	{PCI_DEVICE_DATA(NOKIA, CPUCTL_VERMILION, &ctls[ctl_cp_vermilion])},
	{PCI_DEVICE_DATA(NOKIA, IOCTL_VERMILION, &ctls[ctl_io_vermilion])},
	{0},
};

static struct pci_driver ctl_pci_driver = {
	.name = MODULE_NAME,
	.id_table = ctl_ids,
	.probe = ctl_probe,
	.remove = ctl_remove,
};

static int ctl_probe(struct pci_dev *pcidev, const struct pci_device_id *id)
{
	int rc;
	CTLDEV *pdev;
	struct ctlvariant *ctlv;

	ctlv = (struct ctlvariant *)id->driver_data;

	dev_info(&pcidev->dev, "probe for %s (%04x:%04x) at 0x%llx\n", ctlv->name, id->vendor, id->device,
			 pci_resource_start(pcidev, 0));

	rc = pci_enable_device(pcidev);
	if (rc < 0)
	{
		dev_err(&pcidev->dev, "pci_enable_device failed\n");
		return rc;
	}

	pdev = ctl_dev_alloc();
	if (pdev == NULL)
	{
		dev_err(&pcidev->dev, "ctl_dev_alloc failed\n");
		return -ENOMEM;
	}
	pdev->base = pcim_iomap(pcidev, 0, 0);
	if (!pdev->base)
	{
		dev_err(&pcidev->dev, "pcim_iomap failed\n");
		ctl_dev_free(pdev);
		return rc;
	}

	pci_set_drvdata(pcidev, pdev);
	pdev->pcidev = pcidev;
	pdev->ctlv = ctlv;
	pdev->enabled = 1;
	pcidev->dev.init_name = ctlv->name;

	dev_dbg(&pcidev->dev, "control/status 0x%016llx cardtype 0x%02x board=%d\n",
			ctl_reg64_read(pdev, CTL_CNTR_STA),
			ctl_reg_read(pdev, CTL_CARD_TYPE),
			board);

	switch (board) {
		case brd_x3b:
		ctlv->num_asics = 2;
		ctlv->num_asic_if = 2;
		break;
		case brd_x1b:
		ctlv->num_asics = 1;
		ctlv->num_asic_if = 1;
		break;
		case brd_x4:
		ctlv->num_asics = 1;
		ctlv->num_asic_if = 2;
		break;
	}

	if (ctlv->miscio1_oe)
	{
		/* init io output enable mask */
		ctl_reg_write(pdev, CTL_MISC_IO1_ENA, ctlv->miscio1_oe);
	}
	if (ctlv->miscio3_oe)
	{
		/* init io output enable mask */
		ctl_reg_write(pdev, CTL_MISC_IO3_ENA, ctlv->miscio3_oe);
	}
	if (ctlv->miscio4_oe)
	{
		/* init io output enable mask */
		ctl_reg_write(pdev, CTL_MISC_IO4_ENA, ctlv->miscio4_oe);
	}

	ctl_clk_reset(pdev);

	/* i2c stuff */
	rc = ctl_i2c_probe(pdev);
	if (rc)
	{
		pcim_iounmap(pcidev, pdev->base);
		ctl_dev_free(pdev);
		return rc;
	}

	spi_device_create(pdev);

	/* tbd: set instance name e.g. /sys/bus/pci/drivers/cpuctl/ctl_io_vermilion
	kobject_set_name(&pcidev->dev.kobj, ctlv->name); */

	ctl_sysfs_init(pdev);

	dev_dbg(&pcidev->dev, "probe done\n");

	return rc;
}

static void ctl_remove(struct pci_dev *pcidev)
{
	CTLDEV *pdev = (CTLDEV *)pci_get_drvdata(pcidev);

	dev_dbg(&pcidev->dev, "%s\n", __FUNCTION__);
	if (!pdev)
		return;

	ctl_sysfs_remove(pdev);
	ctl_i2c_remove(pdev);

	pci_disable_device(pcidev);
	pcim_iounmap(pcidev, pdev->base);
	spi_device_remove(pdev);
	ctl_dev_free(pdev);
}

static int __init cpuctl_init(void)
{
	int rc;

	printk(KERN_INFO MODULE_NAME " %s\n", __FUNCTION__);

	rc = pci_register_driver(&ctl_pci_driver);
	if (rc < 0)
	{
		printk(KERN_ERR MODULE_NAME " pci_register_driver failed %d\n", rc);
		return rc;
	}

	return rc;
}

static void __exit cpuctl_exit(void)
{
	// spi_driver_exit();
	pci_unregister_driver(&ctl_pci_driver);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("jon.goldberg@nokia.com");
MODULE_DESCRIPTION("ctl driver");
MODULE_DEVICE_TABLE(pci, ctl_ids);

module_init(cpuctl_init);
module_exit(cpuctl_exit);
