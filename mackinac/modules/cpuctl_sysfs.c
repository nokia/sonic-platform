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
#include <linux/hwmon-sysfs.h>
#include "cpuctl.h"

static ssize_t jer_reset_seq_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	u32 val = 0;
	return sprintf(buf, "%d\n", val);
}

static ssize_t jer_reset_seq_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
	int i;
	int num_asics,num_asic_if;
	u32 val,bits;
	CTLDEV *pdev = dev_get_drvdata(dev);

	num_asics = pdev->ctlv->num_asics;
	num_asic_if = pdev->ctlv->num_asic_if;
	if (num_asic_if == 0) {
		dev_warn(dev, "num_asic_if=0 missing configuration for board=%d?\n",board);
		return -EINVAL;
	}

	dev_info(dev, "resetting asics/if (%d/%d)\n",num_asics,num_asic_if);
	
	/* put jer in reset */
	dev_dbg(dev, "%s put into reset\n", __FUNCTION__);
	spin_lock(&pdev->lock);
	val = ctl_reg_read(pdev, CTL_MISC_IO3_DAT);
	val &= ~(MISCIO3_IO_VERM_JER0_SYS_RST_BIT | MISCIO3_IO_VERM_JER1_SYS_RST_BIT |
			 MISCIO3_IO_VERM_JER0_SYS_PCI_BIT | MISCIO3_IO_VERM_JER1_SYS_PCI_BIT);
	ctl_reg_write(pdev, CTL_MISC_IO3_DAT, val);
	spin_unlock(&pdev->lock);
	msleep(100);

	/* take plls/rgb out of reset */
	bits = MISCIO4_IO_VERM_IMM_PLL_RST_N_BIT | MISCIO4_IO_VERM_IMM_PLL2_RST_N_BIT;

	if (board == brd_x1b)
		bits |= MISCIO4_IO_VERM_IMM_RGB_RST_N_BIT;
	val = ctl_reg_read(pdev, CTL_MISC_IO4_DAT);
	val &= ~bits;
	ctl_reg_write(pdev, CTL_MISC_IO4_DAT, val);
	msleep(10);
	val |= bits;
	ctl_reg_write(pdev, CTL_MISC_IO4_DAT, val);
	dev_dbg(dev, "%s wrote io4_dat 0x%08x\n", __FUNCTION__, val);
	msleep(100);

	/* take jer out of reset */
	for (i = 0; i < num_asic_if; i++)
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
		dev_dbg(dev, "%s take out of reset asic%d\n", __FUNCTION__,i);

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

static void flush_delay(CTLDEV *pdev)
{
    u32 val;
	val = ctl_reg_read(pdev, A32_IO_MISCIO4_DATA);
	udelay(556);
}

static ssize_t jer_avs_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 val;
	val = ctl_reg_read(pdev, CTL_A32_IO_MISCIO2_DATA);
	return uint_show(buf, val & 0xffff);
}

static ssize_t chan_stats_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	int i;
	char* p = buf;
	p += sprintf(p, "chan\tthmin\tthcnt\tbackcnt\n");
	for(i=0;i<pdev->ctlv->nchans;i++) {
		p += sprintf(p, "chan%02d\t%d\t%d\t%d\n", i, 
			pdev->chan_stats[i].throttle_min,
			pdev->chan_stats[i].throttle_cnt,
			pdev->chan_stats[i].backoff_cnt
		);
	}
	return (p - buf);
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

static ssize_t fandraw_prs_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u32 val;

	val = ctl_reg_read(pdev, CTL_A32_CP_MISCIO2_DATA);

	return sprintf(buf, "%d\n", val>>(u8)sda->index & 0x1 ? 1:0);
}

static ssize_t port_prs_reg1_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	u32 val;
	CTLDEV *pdev = dev_get_drvdata(dev);

	val = ctl_reg_read(pdev, IO_A32_PORT_MOD_ABS_BASE);

	return uint_show(buf, val);
}

static ssize_t port_prs_reg2_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	u32 val;
	CTLDEV *pdev = dev_get_drvdata(dev);

	val = ctl_reg_read(pdev, IO_A32_PORT_MOD_ABS_BASE + 4);

	return sprintf(buf, "0x%01x\n", val&0xf);
}

static ssize_t code_ver_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 val;
	
	val = ctl_reg_read(pdev, FPGA_A32_CODE_VER);
		
	return sprintf(buf, "%02x.00\n", (val & 0xff000000) >> 24);
}

static ssize_t port_prs_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u32 val;

	val = ctl_reg_read(pdev, IO_A32_PORT_MOD_ABS_BASE + ((u8)(sda->index / 32) * 4));

	return sprintf(buf, "%d\n", val>>(u8)(sda->index % 32) & 0x1 ? 1:0);
}

static ssize_t port_lpmod_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u32 val;
	
	val = ctl_reg_read(pdev, IO_A32_PORT_MOD_LPMODE_BASE + ((u8)(sda->index / 32) * 4));
	
	return sprintf(buf, "%d\n", val>>(u8)(sda->index % 32) & 0x1 ? 1:0);
}

static ssize_t port_lpmod_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u32 reg_val = 0;
	u32 usr_val = 0;
	u32 mask;
	
	int ret = kstrtouint(buf, 10, &usr_val);
	if (ret != 0)
		return ret;
	if (usr_val > 1)
		return -EINVAL;
	
	mask = (~(1 << (u8)(sda->index % 32))) & 0xffffffff;
	usr_val = usr_val << (u8)(sda->index % 32);
	
	spin_lock(&pdev->lock);
	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_LPMODE_BASE + (u8)(sda->index / 32 * 4));
	reg_val = reg_val & mask;
	ctl_reg_write(pdev, IO_A32_PORT_MOD_LPMODE_BASE + (u8)(sda->index / 32 * 4), (reg_val | usr_val));
	spin_unlock(&pdev->lock);
	flush_delay(pdev);
	
	return count;
}

static ssize_t port_rst_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u32 val;

	val = ctl_reg_read(pdev, IO_A32_PORT_MOD_RST_BASE + ((u8)(sda->index / 32) * 4));

	return sprintf(buf, "%d\n", val>>(u8)(sda->index % 32) & 0x1 ? 1:0);
}

static ssize_t port_rst_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u32 reg_val = 0;
	u32 usr_val = 0;
	u32 mask;

	int ret = kstrtouint(buf, 10, &usr_val);
	if (ret != 0) 
		return ret;
	if (usr_val > 1)
		return -EINVAL;

	mask = (~(1 << (u8)(sda->index % 32))) & 0xffffffff;
	usr_val = usr_val << (u8)(sda->index % 32);

	spin_lock(&pdev->lock);
	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_RST_BASE + (u8)(sda->index / 32 * 4));
	reg_val = reg_val & mask;
	ctl_reg_write(pdev, IO_A32_PORT_MOD_RST_BASE + (u8)(sda->index / 32 * 4), (reg_val | usr_val));
	spin_unlock(&pdev->lock);
	flush_delay(pdev);

	return count;
}

static ssize_t port_led_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u8 val;
	u8 offset;

	offset = sda->index % 32 * 4 + sda->index / 32 * 2;
	val = ctl_reg8_read(pdev, IO_A8_LED_STATE_BASE + offset);

	return sprintf(buf, "0x%x\n", val);
}

static ssize_t port_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
	u32 usr_val = 0;
	u8 offset;

	int ret = kstrtouint(buf, 16, &usr_val);
	if (ret != 0) 
		return ret;
	if (usr_val > 0xff)
		return -EINVAL;

	offset = sda->index % 32 * 4 + sda->index / 32 * 2;	
	ctl_reg8_write(pdev, IO_A8_LED_STATE_BASE + offset, usr_val);

	return count;
}

static ssize_t led_sys_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 val;

	val = ctl_reg_read(pdev, Ctl_A32_LED_STATE_BASE);

	return sprintf(buf, "0x%x\n", val);
}

static ssize_t led_sys_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 usr_val = 0;

	int ret = kstrtouint(buf, 16, &usr_val);
	if (ret != 0) 
		return ret;
	if (usr_val > 0xffffffff)
		return -EINVAL;

	ctl_reg_write(pdev, Ctl_A32_LED_STATE_BASE, usr_val);

	return count;
}

static ssize_t led_fan_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 val;

	val = ctl_reg_read(pdev, Ctl_A32_LED_STATE_BASE + 8);

	return sprintf(buf, "0x%x\n", val);
}

static ssize_t led_fan_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 usr_val = 0;

	int ret = kstrtouint(buf, 16, &usr_val);
	if (ret != 0) 
		return ret;
	if (usr_val > 0xffffffff)
		return -EINVAL;

	ctl_reg_write(pdev, Ctl_A32_LED_STATE_BASE + 8, usr_val);

	return count;
}

static ssize_t led_psu_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 val;

	val = ctl_reg_read(pdev, Ctl_A32_LED_STATE_BASE + 12);

	return sprintf(buf, "0x%x\n", val);
}

static ssize_t led_psu_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 usr_val = 0;

	int ret = kstrtouint(buf, 16, &usr_val);
	if (ret != 0) 
		return ret;
	if (usr_val > 0xffffffff)
		return -EINVAL;

	ctl_reg_write(pdev, Ctl_A32_LED_STATE_BASE + 12, usr_val);

	return count;
}

static ssize_t led_mgmt_link_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u8 val;

	val = ctl_reg8_read(pdev, IO_A8_LED_STATE_BASE + 4);

	return sprintf(buf, "0x%x\n", val);
}

static ssize_t led_mgmt_link_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 usr_val = 0;

	int ret = kstrtouint(buf, 16, &usr_val);
	if (ret != 0) 
		return ret;
	if (usr_val > 0xff)
		return -EINVAL;

	ctl_reg8_write(pdev, IO_A8_LED_STATE_BASE + 4, usr_val);

	return count;
}

static ssize_t led_mgmt_actv_show(struct device *dev, struct device_attribute *devattr, char *buf) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u8 val;

	val = ctl_reg8_read(pdev, IO_A8_LED_STATE_BASE);

	return sprintf(buf, "0x%x\n", val);
}

static ssize_t led_mgmt_actv_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	CTLDEV *pdev = dev_get_drvdata(dev);
	u32 usr_val = 0;

	int ret = kstrtouint(buf, 16, &usr_val);
	if (ret != 0) 
		return ret;
	if (usr_val > 0xff)
		return -EINVAL;

	ctl_reg8_write(pdev, IO_A8_LED_STATE_BASE, usr_val);

	return count;
}

static DEVICE_ATTR_RW(jer_reset_seq);
static DEVICE_ATTR_RW(bus_speed);
static DEVICE_ATTR_RO(jer_avs);
static DEVICE_ATTR_RO(chan_stats);

static SENSOR_DEVICE_ATTR(fandraw_1_prs, S_IRUGO, fandraw_prs_show, NULL, 0);
static SENSOR_DEVICE_ATTR(fandraw_2_prs, S_IRUGO, fandraw_prs_show, NULL, 1);
static SENSOR_DEVICE_ATTR(fandraw_3_prs, S_IRUGO, fandraw_prs_show, NULL, 2);
static DEVICE_ATTR_RW(led_sys);
static DEVICE_ATTR_RW(led_fan);
static DEVICE_ATTR_RW(led_psu);
static DEVICE_ATTR_RW(led_mgmt_link);
static DEVICE_ATTR_RW(led_mgmt_actv);

static DEVICE_ATTR_RO(code_ver);
static DEVICE_ATTR_RO(port_prs_reg1);
static DEVICE_ATTR_RO(port_prs_reg2);
static SENSOR_DEVICE_ATTR(port_1_prs, S_IRUGO, port_prs_show, NULL, 0);
static SENSOR_DEVICE_ATTR(port_2_prs, S_IRUGO, port_prs_show, NULL, 1);
static SENSOR_DEVICE_ATTR(port_3_prs, S_IRUGO, port_prs_show, NULL, 2);
static SENSOR_DEVICE_ATTR(port_4_prs, S_IRUGO, port_prs_show, NULL, 3);
static SENSOR_DEVICE_ATTR(port_5_prs, S_IRUGO, port_prs_show, NULL, 4);
static SENSOR_DEVICE_ATTR(port_6_prs, S_IRUGO, port_prs_show, NULL, 5);
static SENSOR_DEVICE_ATTR(port_7_prs, S_IRUGO, port_prs_show, NULL, 6);
static SENSOR_DEVICE_ATTR(port_8_prs, S_IRUGO, port_prs_show, NULL, 7);
static SENSOR_DEVICE_ATTR(port_9_prs, S_IRUGO, port_prs_show, NULL, 8);
static SENSOR_DEVICE_ATTR(port_10_prs, S_IRUGO, port_prs_show, NULL, 9);
static SENSOR_DEVICE_ATTR(port_11_prs, S_IRUGO, port_prs_show, NULL, 10);
static SENSOR_DEVICE_ATTR(port_12_prs, S_IRUGO, port_prs_show, NULL, 11);
static SENSOR_DEVICE_ATTR(port_13_prs, S_IRUGO, port_prs_show, NULL, 12);
static SENSOR_DEVICE_ATTR(port_14_prs, S_IRUGO, port_prs_show, NULL, 13);
static SENSOR_DEVICE_ATTR(port_15_prs, S_IRUGO, port_prs_show, NULL, 14);
static SENSOR_DEVICE_ATTR(port_16_prs, S_IRUGO, port_prs_show, NULL, 15);
static SENSOR_DEVICE_ATTR(port_17_prs, S_IRUGO, port_prs_show, NULL, 16);
static SENSOR_DEVICE_ATTR(port_18_prs, S_IRUGO, port_prs_show, NULL, 17);
static SENSOR_DEVICE_ATTR(port_19_prs, S_IRUGO, port_prs_show, NULL, 18);
static SENSOR_DEVICE_ATTR(port_20_prs, S_IRUGO, port_prs_show, NULL, 19);
static SENSOR_DEVICE_ATTR(port_21_prs, S_IRUGO, port_prs_show, NULL, 20);
static SENSOR_DEVICE_ATTR(port_22_prs, S_IRUGO, port_prs_show, NULL, 21);
static SENSOR_DEVICE_ATTR(port_23_prs, S_IRUGO, port_prs_show, NULL, 22);
static SENSOR_DEVICE_ATTR(port_24_prs, S_IRUGO, port_prs_show, NULL, 23);
static SENSOR_DEVICE_ATTR(port_25_prs, S_IRUGO, port_prs_show, NULL, 24);
static SENSOR_DEVICE_ATTR(port_26_prs, S_IRUGO, port_prs_show, NULL, 25);
static SENSOR_DEVICE_ATTR(port_27_prs, S_IRUGO, port_prs_show, NULL, 26);
static SENSOR_DEVICE_ATTR(port_28_prs, S_IRUGO, port_prs_show, NULL, 27);
static SENSOR_DEVICE_ATTR(port_29_prs, S_IRUGO, port_prs_show, NULL, 28);
static SENSOR_DEVICE_ATTR(port_30_prs, S_IRUGO, port_prs_show, NULL, 29);
static SENSOR_DEVICE_ATTR(port_31_prs, S_IRUGO, port_prs_show, NULL, 30);
static SENSOR_DEVICE_ATTR(port_32_prs, S_IRUGO, port_prs_show, NULL, 31);
static SENSOR_DEVICE_ATTR(port_33_prs, S_IRUGO, port_prs_show, NULL, 32);
static SENSOR_DEVICE_ATTR(port_34_prs, S_IRUGO, port_prs_show, NULL, 33);
static SENSOR_DEVICE_ATTR(port_35_prs, S_IRUGO, port_prs_show, NULL, 34);
static SENSOR_DEVICE_ATTR(port_36_prs, S_IRUGO, port_prs_show, NULL, 35);
static SENSOR_DEVICE_ATTR(port_1_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 0);
static SENSOR_DEVICE_ATTR(port_2_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 1);
static SENSOR_DEVICE_ATTR(port_3_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 2);
static SENSOR_DEVICE_ATTR(port_4_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 3);
static SENSOR_DEVICE_ATTR(port_5_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 4);
static SENSOR_DEVICE_ATTR(port_6_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 5);
static SENSOR_DEVICE_ATTR(port_7_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 6);
static SENSOR_DEVICE_ATTR(port_8_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 7);
static SENSOR_DEVICE_ATTR(port_9_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 8);
static SENSOR_DEVICE_ATTR(port_10_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 9);
static SENSOR_DEVICE_ATTR(port_11_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 10);
static SENSOR_DEVICE_ATTR(port_12_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 11);
static SENSOR_DEVICE_ATTR(port_13_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 12);
static SENSOR_DEVICE_ATTR(port_14_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 13);
static SENSOR_DEVICE_ATTR(port_15_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 14);
static SENSOR_DEVICE_ATTR(port_16_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 15);
static SENSOR_DEVICE_ATTR(port_17_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 16);
static SENSOR_DEVICE_ATTR(port_18_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 17);
static SENSOR_DEVICE_ATTR(port_19_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 18);
static SENSOR_DEVICE_ATTR(port_20_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 19);
static SENSOR_DEVICE_ATTR(port_21_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 20);
static SENSOR_DEVICE_ATTR(port_22_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 21);
static SENSOR_DEVICE_ATTR(port_23_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 22);
static SENSOR_DEVICE_ATTR(port_24_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 23);
static SENSOR_DEVICE_ATTR(port_25_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 24);
static SENSOR_DEVICE_ATTR(port_26_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 25);
static SENSOR_DEVICE_ATTR(port_27_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 26);
static SENSOR_DEVICE_ATTR(port_28_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 27);
static SENSOR_DEVICE_ATTR(port_29_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 28);
static SENSOR_DEVICE_ATTR(port_30_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 29);
static SENSOR_DEVICE_ATTR(port_31_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 30);
static SENSOR_DEVICE_ATTR(port_32_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 31);
static SENSOR_DEVICE_ATTR(port_33_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 32);
static SENSOR_DEVICE_ATTR(port_34_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 33);
static SENSOR_DEVICE_ATTR(port_35_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 34);
static SENSOR_DEVICE_ATTR(port_36_lpmod, S_IRUGO | S_IWUSR, port_lpmod_show, port_lpmod_store, 35);
static SENSOR_DEVICE_ATTR(port_1_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 0);
static SENSOR_DEVICE_ATTR(port_2_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 1);
static SENSOR_DEVICE_ATTR(port_3_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 2);
static SENSOR_DEVICE_ATTR(port_4_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 3);
static SENSOR_DEVICE_ATTR(port_5_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 4);
static SENSOR_DEVICE_ATTR(port_6_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 5);
static SENSOR_DEVICE_ATTR(port_7_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 6);
static SENSOR_DEVICE_ATTR(port_8_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 7);
static SENSOR_DEVICE_ATTR(port_9_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 8);
static SENSOR_DEVICE_ATTR(port_10_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 9);
static SENSOR_DEVICE_ATTR(port_11_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 10);
static SENSOR_DEVICE_ATTR(port_12_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 11);
static SENSOR_DEVICE_ATTR(port_13_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 12);
static SENSOR_DEVICE_ATTR(port_14_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 13);
static SENSOR_DEVICE_ATTR(port_15_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 14);
static SENSOR_DEVICE_ATTR(port_16_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 15);
static SENSOR_DEVICE_ATTR(port_17_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 16);
static SENSOR_DEVICE_ATTR(port_18_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 17);
static SENSOR_DEVICE_ATTR(port_19_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 18);
static SENSOR_DEVICE_ATTR(port_20_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 19);
static SENSOR_DEVICE_ATTR(port_21_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 20);
static SENSOR_DEVICE_ATTR(port_22_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 21);
static SENSOR_DEVICE_ATTR(port_23_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 22);
static SENSOR_DEVICE_ATTR(port_24_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 23);
static SENSOR_DEVICE_ATTR(port_25_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 24);
static SENSOR_DEVICE_ATTR(port_26_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 25);
static SENSOR_DEVICE_ATTR(port_27_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 26);
static SENSOR_DEVICE_ATTR(port_28_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 27);
static SENSOR_DEVICE_ATTR(port_29_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 28);
static SENSOR_DEVICE_ATTR(port_30_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 29);
static SENSOR_DEVICE_ATTR(port_31_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 30);
static SENSOR_DEVICE_ATTR(port_32_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 31);
static SENSOR_DEVICE_ATTR(port_33_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 32);
static SENSOR_DEVICE_ATTR(port_34_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 33);
static SENSOR_DEVICE_ATTR(port_35_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 34);
static SENSOR_DEVICE_ATTR(port_36_rst, S_IRUGO | S_IWUSR, port_rst_show, port_rst_store, 35);

static SENSOR_DEVICE_ATTR(port_1_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 0);
static SENSOR_DEVICE_ATTR(port_2_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 1);
static SENSOR_DEVICE_ATTR(port_3_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 2);
static SENSOR_DEVICE_ATTR(port_4_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 3);
static SENSOR_DEVICE_ATTR(port_5_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 4);
static SENSOR_DEVICE_ATTR(port_6_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 5);
static SENSOR_DEVICE_ATTR(port_7_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 6);
static SENSOR_DEVICE_ATTR(port_8_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 7);
static SENSOR_DEVICE_ATTR(port_9_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 8);
static SENSOR_DEVICE_ATTR(port_10_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 9);
static SENSOR_DEVICE_ATTR(port_11_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 10);
static SENSOR_DEVICE_ATTR(port_12_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 11);
static SENSOR_DEVICE_ATTR(port_13_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 12);
static SENSOR_DEVICE_ATTR(port_14_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 13);
static SENSOR_DEVICE_ATTR(port_15_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 14);
static SENSOR_DEVICE_ATTR(port_16_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 15);
static SENSOR_DEVICE_ATTR(port_17_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 16);
static SENSOR_DEVICE_ATTR(port_18_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 17);
static SENSOR_DEVICE_ATTR(port_19_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 18);
static SENSOR_DEVICE_ATTR(port_20_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 19);
static SENSOR_DEVICE_ATTR(port_21_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 20);
static SENSOR_DEVICE_ATTR(port_22_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 21);
static SENSOR_DEVICE_ATTR(port_23_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 22);
static SENSOR_DEVICE_ATTR(port_24_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 23);
static SENSOR_DEVICE_ATTR(port_25_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 24);
static SENSOR_DEVICE_ATTR(port_26_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 25);
static SENSOR_DEVICE_ATTR(port_27_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 26);
static SENSOR_DEVICE_ATTR(port_28_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 27);
static SENSOR_DEVICE_ATTR(port_29_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 28);
static SENSOR_DEVICE_ATTR(port_30_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 29);
static SENSOR_DEVICE_ATTR(port_31_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 30);
static SENSOR_DEVICE_ATTR(port_32_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 31);
static SENSOR_DEVICE_ATTR(port_33_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 32);
static SENSOR_DEVICE_ATTR(port_34_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 33);
static SENSOR_DEVICE_ATTR(port_35_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 34);
static SENSOR_DEVICE_ATTR(port_36_led, S_IRUGO | S_IWUSR, port_led_show, port_led_store, 35);

static struct attribute *cp_vermilion_ctl_attrs[] = {
	&dev_attr_bus_speed.attr,
	&sensor_dev_attr_fandraw_1_prs.dev_attr.attr,
	&sensor_dev_attr_fandraw_2_prs.dev_attr.attr,
	&sensor_dev_attr_fandraw_3_prs.dev_attr.attr,
	&dev_attr_code_ver.attr,
	&dev_attr_led_sys.attr,
	&dev_attr_led_fan.attr,
	&dev_attr_led_psu.attr,
	&dev_attr_led_mgmt_link.attr,
	&dev_attr_led_mgmt_actv.attr,

	NULL,
};

static const struct attribute_group cp_vermilion_ctl_group = {
	.attrs = cp_vermilion_ctl_attrs,
};

static struct attribute *io_vermilion_ctl_attrs[] = {
	&dev_attr_jer_reset_seq.attr,
	&dev_attr_jer_avs.attr,
	&dev_attr_bus_speed.attr,
	&dev_attr_port_prs_reg1.attr,
	&dev_attr_port_prs_reg2.attr,
	&dev_attr_code_ver.attr,
	&dev_attr_chan_stats.attr,

	&sensor_dev_attr_port_1_prs.dev_attr.attr,
	&sensor_dev_attr_port_2_prs.dev_attr.attr,
	&sensor_dev_attr_port_3_prs.dev_attr.attr,
	&sensor_dev_attr_port_4_prs.dev_attr.attr,
	&sensor_dev_attr_port_5_prs.dev_attr.attr,
	&sensor_dev_attr_port_6_prs.dev_attr.attr,
	&sensor_dev_attr_port_7_prs.dev_attr.attr,
	&sensor_dev_attr_port_8_prs.dev_attr.attr,
	&sensor_dev_attr_port_9_prs.dev_attr.attr,
	&sensor_dev_attr_port_10_prs.dev_attr.attr,
	&sensor_dev_attr_port_11_prs.dev_attr.attr,
	&sensor_dev_attr_port_12_prs.dev_attr.attr,
	&sensor_dev_attr_port_13_prs.dev_attr.attr,
	&sensor_dev_attr_port_14_prs.dev_attr.attr,
	&sensor_dev_attr_port_15_prs.dev_attr.attr,
	&sensor_dev_attr_port_16_prs.dev_attr.attr,
	&sensor_dev_attr_port_17_prs.dev_attr.attr,
	&sensor_dev_attr_port_18_prs.dev_attr.attr,
	&sensor_dev_attr_port_19_prs.dev_attr.attr,
	&sensor_dev_attr_port_20_prs.dev_attr.attr,
	&sensor_dev_attr_port_21_prs.dev_attr.attr,
	&sensor_dev_attr_port_22_prs.dev_attr.attr,
	&sensor_dev_attr_port_23_prs.dev_attr.attr,
	&sensor_dev_attr_port_24_prs.dev_attr.attr,
	&sensor_dev_attr_port_25_prs.dev_attr.attr,
	&sensor_dev_attr_port_26_prs.dev_attr.attr,
	&sensor_dev_attr_port_27_prs.dev_attr.attr,
	&sensor_dev_attr_port_28_prs.dev_attr.attr,
	&sensor_dev_attr_port_29_prs.dev_attr.attr,
	&sensor_dev_attr_port_30_prs.dev_attr.attr,
	&sensor_dev_attr_port_31_prs.dev_attr.attr,
	&sensor_dev_attr_port_32_prs.dev_attr.attr,
	&sensor_dev_attr_port_33_prs.dev_attr.attr,
	&sensor_dev_attr_port_34_prs.dev_attr.attr,
	&sensor_dev_attr_port_35_prs.dev_attr.attr,
	&sensor_dev_attr_port_36_prs.dev_attr.attr,
	
	&sensor_dev_attr_port_1_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_2_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_3_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_4_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_5_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_6_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_7_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_8_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_9_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_10_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_11_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_12_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_13_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_14_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_15_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_16_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_17_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_18_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_19_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_20_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_21_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_22_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_23_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_24_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_25_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_26_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_27_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_28_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_29_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_30_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_31_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_32_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_33_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_34_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_35_lpmod.dev_attr.attr,
	&sensor_dev_attr_port_36_lpmod.dev_attr.attr,

	&sensor_dev_attr_port_1_rst.dev_attr.attr,
	&sensor_dev_attr_port_2_rst.dev_attr.attr,
	&sensor_dev_attr_port_3_rst.dev_attr.attr,
	&sensor_dev_attr_port_4_rst.dev_attr.attr,
	&sensor_dev_attr_port_5_rst.dev_attr.attr,
	&sensor_dev_attr_port_6_rst.dev_attr.attr,
	&sensor_dev_attr_port_7_rst.dev_attr.attr,
	&sensor_dev_attr_port_8_rst.dev_attr.attr,
	&sensor_dev_attr_port_9_rst.dev_attr.attr,
	&sensor_dev_attr_port_10_rst.dev_attr.attr,
	&sensor_dev_attr_port_11_rst.dev_attr.attr,
	&sensor_dev_attr_port_12_rst.dev_attr.attr,
	&sensor_dev_attr_port_13_rst.dev_attr.attr,
	&sensor_dev_attr_port_14_rst.dev_attr.attr,
	&sensor_dev_attr_port_15_rst.dev_attr.attr,
	&sensor_dev_attr_port_16_rst.dev_attr.attr,
	&sensor_dev_attr_port_17_rst.dev_attr.attr,
	&sensor_dev_attr_port_18_rst.dev_attr.attr,
	&sensor_dev_attr_port_19_rst.dev_attr.attr,
	&sensor_dev_attr_port_20_rst.dev_attr.attr,
	&sensor_dev_attr_port_21_rst.dev_attr.attr,
	&sensor_dev_attr_port_22_rst.dev_attr.attr,
	&sensor_dev_attr_port_23_rst.dev_attr.attr,
	&sensor_dev_attr_port_24_rst.dev_attr.attr,
	&sensor_dev_attr_port_25_rst.dev_attr.attr,
	&sensor_dev_attr_port_26_rst.dev_attr.attr,
	&sensor_dev_attr_port_27_rst.dev_attr.attr,
	&sensor_dev_attr_port_28_rst.dev_attr.attr,
	&sensor_dev_attr_port_29_rst.dev_attr.attr,
	&sensor_dev_attr_port_30_rst.dev_attr.attr,
	&sensor_dev_attr_port_31_rst.dev_attr.attr,
	&sensor_dev_attr_port_32_rst.dev_attr.attr,
	&sensor_dev_attr_port_33_rst.dev_attr.attr,
	&sensor_dev_attr_port_34_rst.dev_attr.attr,
	&sensor_dev_attr_port_35_rst.dev_attr.attr,
	&sensor_dev_attr_port_36_rst.dev_attr.attr,

	&sensor_dev_attr_port_1_led.dev_attr.attr,
	&sensor_dev_attr_port_2_led.dev_attr.attr,
	&sensor_dev_attr_port_3_led.dev_attr.attr,
	&sensor_dev_attr_port_4_led.dev_attr.attr,
	&sensor_dev_attr_port_5_led.dev_attr.attr,
	&sensor_dev_attr_port_6_led.dev_attr.attr,
	&sensor_dev_attr_port_7_led.dev_attr.attr,
	&sensor_dev_attr_port_8_led.dev_attr.attr,
	&sensor_dev_attr_port_9_led.dev_attr.attr,
	&sensor_dev_attr_port_10_led.dev_attr.attr,
	&sensor_dev_attr_port_11_led.dev_attr.attr,
	&sensor_dev_attr_port_12_led.dev_attr.attr,
	&sensor_dev_attr_port_13_led.dev_attr.attr,
	&sensor_dev_attr_port_14_led.dev_attr.attr,
	&sensor_dev_attr_port_15_led.dev_attr.attr,
	&sensor_dev_attr_port_16_led.dev_attr.attr,
	&sensor_dev_attr_port_17_led.dev_attr.attr,
	&sensor_dev_attr_port_18_led.dev_attr.attr,
	&sensor_dev_attr_port_19_led.dev_attr.attr,
	&sensor_dev_attr_port_20_led.dev_attr.attr,
	&sensor_dev_attr_port_21_led.dev_attr.attr,
	&sensor_dev_attr_port_22_led.dev_attr.attr,
	&sensor_dev_attr_port_23_led.dev_attr.attr,
	&sensor_dev_attr_port_24_led.dev_attr.attr,
	&sensor_dev_attr_port_25_led.dev_attr.attr,
	&sensor_dev_attr_port_26_led.dev_attr.attr,
	&sensor_dev_attr_port_27_led.dev_attr.attr,
	&sensor_dev_attr_port_28_led.dev_attr.attr,
	&sensor_dev_attr_port_29_led.dev_attr.attr,
	&sensor_dev_attr_port_30_led.dev_attr.attr,
	&sensor_dev_attr_port_31_led.dev_attr.attr,
	&sensor_dev_attr_port_32_led.dev_attr.attr,
	&sensor_dev_attr_port_33_led.dev_attr.attr,
	&sensor_dev_attr_port_34_led.dev_attr.attr,
	&sensor_dev_attr_port_35_led.dev_attr.attr,
	&sensor_dev_attr_port_36_led.dev_attr.attr,
	NULL,
};

static const struct attribute_group io_vermilion_ctl_group = {
	.attrs = io_vermilion_ctl_attrs,
};

static void port_init(CTLDEV *pdev)
{
	u32 reg_val = 0;

	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_ABS_BASE);
	dev_info(&pdev->pcidev->dev, "MOD_ABS: 0x%08x\n", reg_val);
	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_ABS_BASE + 4);
	dev_info(&pdev->pcidev->dev, "MOD_ABS+4: 0x%08x\n", reg_val);

	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_RST_BASE);
	dev_info(&pdev->pcidev->dev, "MOD_RST: 0x%08x\n", reg_val);
	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_RST_BASE + 4);
	dev_info(&pdev->pcidev->dev, "MOD_RST+4: 0x%08x\n", reg_val);
	
	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_LPMODE_BASE);
	dev_info(&pdev->pcidev->dev, "MOD_LPMODE: 0x%08x\n", reg_val);
	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_LPMODE_BASE + 4);
	dev_info(&pdev->pcidev->dev, "MOD_LPMODE+4: 0x%08x\n", reg_val);

	reg_val = ctl_reg_read(pdev, CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE);
	dev_info(&pdev->pcidev->dev, "MOD_SEL: 0x%08x\n", reg_val);
	reg_val = ctl_reg_read(pdev, CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE + 4);
	dev_info(&pdev->pcidev->dev, "MOD_SEL+4: 0x%08x\n", reg_val);

	spin_lock(&pdev->lock);
	if (board == brd_x1b) {
		ctl_reg_write(pdev, IO_A32_PORT_MOD_LPMODE_BASE, 0xff000000);
		reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_LPMODE_BASE + 4);
		reg_val = reg_val & 0xfffffff0;
		ctl_reg_write(pdev, IO_A32_PORT_MOD_LPMODE_BASE + 4, (reg_val | 0xf));
	} else {
		ctl_reg_write(pdev, IO_A32_PORT_MOD_LPMODE_BASE, 0xffffffff);
		reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_LPMODE_BASE + 4);
		reg_val = reg_val & 0xfffffff0;
		ctl_reg_write(pdev, IO_A32_PORT_MOD_LPMODE_BASE + 4, (reg_val | 0xf));
	}
	spin_unlock(&pdev->lock);
	flush_delay(pdev);

	spin_lock(&pdev->lock);
	ctl_reg_write(pdev, IO_A32_PORT_MOD_RST_BASE, 0xffffffff);
	reg_val = ctl_reg_read(pdev, IO_A32_PORT_MOD_RST_BASE + 4);
	reg_val = reg_val & 0xfffffff0;
	ctl_reg_write(pdev, IO_A32_PORT_MOD_RST_BASE + 4, (reg_val | 0xf));	
	spin_unlock(&pdev->lock);
	flush_delay(pdev);
}

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

	if (pdev->ctlv->ctl_type == ctl_io_vermilion) {
		port_init(pdev);
	}

	return rc;
}

void ctl_sysfs_remove(CTLDEV *pdev)
{
	if (pdev->sysfs)
		sysfs_remove_group(&pdev->pcidev->dev.kobj, pdev->sysfs);
}
