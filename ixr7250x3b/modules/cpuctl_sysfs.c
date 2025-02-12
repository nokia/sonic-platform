// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia cpctl/ioctl i2c bus adapter/multiplexer
 *
 *  Copyright (C) 2024 Nokia
 *
 */

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include "cpuctl.h"

static ssize_t jer_reset_seq_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	u32 val = 0;
	return sprintf(buf, "%d\n", val);
}

static ssize_t jer_reset_seq_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
	int i;
	u32 val;
	CTLDEV *pdev = dev_get_drvdata(dev);

	dev_info(dev, "resetting asics\n");

	/* put jer in reset */
	dev_dbg(dev, "%s put into reset\n", __FUNCTION__);
	spin_lock(&pdev->lock);
	val = ctl_reg_read(pdev, CTL_MISC_IO3_DAT);
	val &= ~(MISCIO3_IO_VERM_JER0_SYS_RST_BIT | MISCIO3_IO_VERM_JER1_SYS_RST_BIT |
			 MISCIO3_IO_VERM_JER0_SYS_PCI_BIT | MISCIO3_IO_VERM_JER1_SYS_PCI_BIT);
	ctl_reg_write(pdev, CTL_MISC_IO3_DAT, val);
	spin_unlock(&pdev->lock);
	msleep(100);

	/* take plls out of reset */
	spin_lock(&pdev->lock);
	val = ctl_reg_read(pdev, CTL_MISC_IO4_DAT);
	val |= MISCIO4_IO_VERM_IMM_PLL_RST_N_BIT | MISCIO4_IO_VERM_IMM_PLL2_RST_N_BIT;
	ctl_reg_write(pdev, CTL_MISC_IO4_DAT, val);
	spin_unlock(&pdev->lock);
	dev_dbg(dev, "%s wrote io4_dat 0x%08x\n", __FUNCTION__, val);
	msleep(100);

	/* take jer out of reset */
	dev_dbg(dev, "%s take out of reset\n", __FUNCTION__);
	for (i = 0; i < NUM_JER_ASICS; i++)
	{
		/* sysrst */
		spin_lock(&pdev->lock);
		val = ctl_reg_read(pdev, CTL_MISC_IO3_DAT);
		val |= (MISCIO3_IO_VERM_JER0_SYS_RST_BIT << i);
		ctl_reg_write(pdev, CTL_MISC_IO3_DAT, val);
		spin_unlock(&pdev->lock);

		msleep(100);

		/* pci reset*/
		spin_lock(&pdev->lock);
		val = ctl_reg_read(pdev, CTL_MISC_IO3_DAT);
		val |= (MISCIO3_IO_VERM_JER0_SYS_PCI_BIT << i);
		ctl_reg_write(pdev, CTL_MISC_IO3_DAT, val);
		spin_unlock(&pdev->lock);

		msleep(10);
	}
	dev_dbg(dev, "%s wrote io3_dat 0x%08x\n", __FUNCTION__, val);

	return count;
}

static ssize_t uint_show(char *buf, u32 val)
{
	return sprintf(buf, "0x%08x\n", val);
}

static ssize_t uint_store(const char *buf, size_t count, u32 *pval)
{
	if (kstrtouint(buf, 0, pval) < 0)
		return -EINVAL;
	return count;
}

static ssize_t bus_speed_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	return uint_show(buf, pdev->ctlv->bus400);
}

static ssize_t bus_speed_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	return uint_store(buf, count, &pdev->ctlv->bus400);
}

static DEVICE_ATTR_RW(jer_reset_seq);
static DEVICE_ATTR_RW(bus_speed);

static struct attribute *cp_vermilion_ctl_attrs[] = {
	&dev_attr_bus_speed.attr,
	NULL,
};

static const struct attribute_group cp_vermilion_ctl_group = {
	.attrs = cp_vermilion_ctl_attrs,
};

static struct attribute *io_vermilion_ctl_attrs[] = {
	&dev_attr_jer_reset_seq.attr,
	&dev_attr_bus_speed.attr,
	NULL,
};

static const struct attribute_group io_vermilion_ctl_group = {
	.attrs = io_vermilion_ctl_attrs,
};

int ctl_sysfs_init(CTLDEV *pdev)
{
	int rc;
	switch (pdev->ctlv->ctl_type)
	{
	case ctl_cp_vermilion:
		pdev->sysfs = &cp_vermilion_ctl_group;
		break;
	case ctl_io_vermilion:
		pdev->sysfs = &io_vermilion_ctl_group;
		break;
	default:
		pdev->sysfs = NULL;
		break;
	}

	rc = sysfs_create_group(&pdev->pcidev->dev.kobj, pdev->sysfs);
	return rc;
}

void ctl_sysfs_remove(CTLDEV *pdev)
{
	if (pdev->sysfs)
		sysfs_remove_group(&pdev->pcidev->dev.kobj, pdev->sysfs);
}
