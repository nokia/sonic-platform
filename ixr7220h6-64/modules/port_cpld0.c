//  * CPLD driver for Nokia-7220-IXR-H6-64 Router
//  *
//  * Copyright (C) 2025 Nokia Corporation.
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation, either version 3 of the License, or
//  * any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  * GNU General Public License for more details.
//  * see <http://www.gnu.org/licenses/>


#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#define DRIVER_NAME "port_cpld0"

// REGISTERS ADDRESS MAP
#define VER_MAJOR_REG           0x00
#define VER_MINOR_REG           0x01
#define SFP_CTRL_REG            0x03
#define SCRATCH_REG             0x04
#define SFP_MISC_REG            0x05
#define SFP_TXFAULT_REG         0x06
#define SFP_TXDIS_REG           0x07
#define SFP_RXLOSS_REG          0x08
#define SFP_MODPRS_REG          0x09
#define SFP_EN_LP_REG           0x10
#define OSFP_LPMODE_REG0        0x70
#define OSFP_RST_REG0           0x78
#define OSFP_MODPRS_REG0        0x88
#define OSFP_PWGOOD_REG0        0x90
#define OSFP_ENABLE_REG0        0x94
#define OSFP_LOOPBK_REG0        0x98

// REG BIT FIELD POSITION or MASK
#define SFP0                    0x0
#define SFP1                    0x1

static const unsigned short cpld_address_list[] = {0x64, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
};

static int cpld_i2c_read(struct cpld_data *data, u8 reg)
{
    int val = 0;
    struct i2c_client *client = data->client;

    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
         dev_warn(&client->dev, "CPLD READ ERROR: reg(0x%02x) err %d\n", reg, val);
    }

    return val;
}

static void cpld_i2c_write(struct cpld_data *data, u8 reg, u8 value)
{
    int res = 0;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    res = i2c_smbus_write_byte_data(client, reg, value);
    if (res < 0) {
        dev_warn(&client->dev, "CPLD WRITE ERROR: reg(0x%02x) err %d\n", reg, res);
    }
    mutex_unlock(&data->update_lock);
}

static void dump_reg(struct cpld_data *data)
{
    struct i2c_client *client = data->client;
    u8 val0 = 0;
    u8 val1 = 0;
    u8 val2 = 0;
    u8 val3 = 0;

    val0 = cpld_i2c_read(data, OSFP_RST_REG0);
    val1 = cpld_i2c_read(data, OSFP_RST_REG0 + 1);
    val2 = cpld_i2c_read(data, OSFP_RST_REG0 + 2);
    val3 = cpld_i2c_read(data, OSFP_RST_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD0]OSFP_RESET_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

    val0 = cpld_i2c_read(data, OSFP_LPMODE_REG0);
    val1 = cpld_i2c_read(data, OSFP_LPMODE_REG0 + 1);
    val2 = cpld_i2c_read(data, OSFP_LPMODE_REG0 + 2);
    val3 = cpld_i2c_read(data, OSFP_LPMODE_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD0]OSFP_LPMODE_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

    val0 = cpld_i2c_read(data, OSFP_MODPRS_REG0);
    val1 = cpld_i2c_read(data, OSFP_MODPRS_REG0 + 1);
    val2 = cpld_i2c_read(data, OSFP_MODPRS_REG0 + 2);
    val3 = cpld_i2c_read(data, OSFP_MODPRS_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD0]OSFP_MODPRES_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

}

static ssize_t show_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 reg_major = 0;
    u8 reg_minor = 0;

    reg_major = cpld_i2c_read(data, VER_MAJOR_REG);
    reg_minor = cpld_i2c_read(data, VER_MINOR_REG);

    return sprintf(buf, "%d.%d\n", reg_major, reg_minor);
}

static ssize_t show_scratch(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;

    val = cpld_i2c_read(data, SCRATCH_REG);

    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t set_scratch(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val = 0;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 0xFF) {
        return -EINVAL;
    }

    cpld_i2c_write(data, SCRATCH_REG, usr_val);

    return count;
}

static ssize_t show_sfp_ctl(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SFP_CTRL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_sfp_ctl(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SFP_CTRL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SFP_CTRL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_sfp_misc(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SFP_MISC_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_sfp_misc(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SFP_MISC_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SFP_MISC_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_sfp_tx_fault(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SFP_TXFAULT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_tx_en(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SFP_TXDIS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_sfp_tx_en(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SFP_TXDIS_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SFP_TXDIS_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_sfp_rx_los(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SFP_RXLOSS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_prs(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SFP_MODPRS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_en_lp(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SFP_EN_LP_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_sfp_en_lp(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SFP_EN_LP_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SFP_EN_LP_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_osfp_lpmode(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, OSFP_LPMODE_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t set_osfp_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << (sda->index % 8))) & 0xFF;
    reg_val = cpld_i2c_read(data, OSFP_LPMODE_REG0 + (sda->index / 8));
    reg_val = reg_val & mask;
    usr_val = usr_val << (sda->index % 8);
    cpld_i2c_write(data, OSFP_LPMODE_REG0 + (sda->index / 8), (reg_val | usr_val));

    return count;
}

static ssize_t show_osfp_rst(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, OSFP_RST_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t set_osfp_rst(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << (sda->index % 8))) & 0xFF;
    reg_val = cpld_i2c_read(data, OSFP_RST_REG0 + (sda->index / 8));
    reg_val = reg_val & mask;
    usr_val = usr_val << (sda->index % 8);
    cpld_i2c_write(data, OSFP_RST_REG0 + (sda->index / 8), (reg_val | usr_val));

    return count;
}

static ssize_t show_osfp_prs(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, OSFP_MODPRS_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t show_modprs_reg(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, OSFP_MODPRS_REG0 + sda->index);
    
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_osfp_pwgood(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, OSFP_PWGOOD_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t show_osfp_en(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, OSFP_ENABLE_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t set_osfp_en(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << (sda->index % 8))) & 0xFF;
    reg_val = cpld_i2c_read(data, OSFP_ENABLE_REG0 + (sda->index / 8));
    reg_val = reg_val & mask;
    usr_val = usr_val << (sda->index % 8);
    cpld_i2c_write(data, OSFP_ENABLE_REG0 + (sda->index / 8), (reg_val | usr_val));

    return count;
}

static ssize_t show_osfp_loopb(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, OSFP_LOOPBK_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t set_osfp_loopb(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << (sda->index % 8))) & 0xFF;
    reg_val = cpld_i2c_read(data, OSFP_LOOPBK_REG0 + (sda->index / 8));
    reg_val = reg_val & mask;
    usr_val = usr_val << (sda->index % 8);
    cpld_i2c_write(data, OSFP_LOOPBK_REG0 + (sda->index / 8), (reg_val | usr_val));

    return count;
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);

static SENSOR_DEVICE_ATTR(port_65_rx_rate, S_IRUGO | S_IWUSR, show_sfp_ctl, set_sfp_ctl, 0);
static SENSOR_DEVICE_ATTR(port_65_tx_rate, S_IRUGO | S_IWUSR, show_sfp_ctl, set_sfp_ctl, 1);
static SENSOR_DEVICE_ATTR(port_66_rx_rate, S_IRUGO | S_IWUSR, show_sfp_ctl, set_sfp_ctl, 2);
static SENSOR_DEVICE_ATTR(port_66_tx_rate, S_IRUGO | S_IWUSR, show_sfp_ctl, set_sfp_ctl, 3);
static SENSOR_DEVICE_ATTR(port_65_efuse_en, S_IRUGO | S_IWUSR, show_sfp_misc, set_sfp_misc, 0);
static SENSOR_DEVICE_ATTR(port_66_efuse_en, S_IRUGO | S_IWUSR, show_sfp_misc, set_sfp_misc, 1);
static SENSOR_DEVICE_ATTR(port_65_efuse_flag, S_IRUGO | S_IWUSR, show_sfp_misc, set_sfp_misc, 2);
static SENSOR_DEVICE_ATTR(port_66_efuse_flag, S_IRUGO | S_IWUSR, show_sfp_misc, set_sfp_misc, 3);
static SENSOR_DEVICE_ATTR(port_65_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, SFP0);
static SENSOR_DEVICE_ATTR(port_66_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, SFP1);
static SENSOR_DEVICE_ATTR(port_65_tx_en, S_IRUGO | S_IWUSR, show_sfp_tx_en, set_sfp_tx_en, SFP0);
static SENSOR_DEVICE_ATTR(port_66_tx_en, S_IRUGO | S_IWUSR, show_sfp_tx_en, set_sfp_tx_en, SFP1);
static SENSOR_DEVICE_ATTR(port_65_rx_los, S_IRUGO, show_sfp_rx_los, NULL, SFP0);
static SENSOR_DEVICE_ATTR(port_66_rx_los, S_IRUGO, show_sfp_rx_los, NULL, SFP1);
static SENSOR_DEVICE_ATTR(port_65_prs, S_IRUGO, show_sfp_prs, NULL, SFP0);
static SENSOR_DEVICE_ATTR(port_66_prs, S_IRUGO, show_sfp_prs, NULL, SFP1);
static SENSOR_DEVICE_ATTR(port_65_en, S_IRUGO | S_IWUSR, show_sfp_en_lp, set_sfp_en_lp, 0);
static SENSOR_DEVICE_ATTR(port_66_en, S_IRUGO | S_IWUSR, show_sfp_en_lp, set_sfp_en_lp, 1);
static SENSOR_DEVICE_ATTR(port_65_loopb, S_IRUGO | S_IWUSR, show_sfp_en_lp, set_sfp_en_lp, 2);
static SENSOR_DEVICE_ATTR(port_66_loopb, S_IRUGO | S_IWUSR, show_sfp_en_lp, set_sfp_en_lp, 3);

static SENSOR_DEVICE_ATTR(port_1_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 0);
static SENSOR_DEVICE_ATTR(port_2_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 1);
static SENSOR_DEVICE_ATTR(port_3_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 2);
static SENSOR_DEVICE_ATTR(port_4_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 3);
static SENSOR_DEVICE_ATTR(port_5_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 4);
static SENSOR_DEVICE_ATTR(port_6_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 5);
static SENSOR_DEVICE_ATTR(port_7_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 6);
static SENSOR_DEVICE_ATTR(port_8_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 7);
static SENSOR_DEVICE_ATTR(port_9_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 8);
static SENSOR_DEVICE_ATTR(port_10_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 9);
static SENSOR_DEVICE_ATTR(port_11_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 10);
static SENSOR_DEVICE_ATTR(port_12_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 11);
static SENSOR_DEVICE_ATTR(port_13_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 12);
static SENSOR_DEVICE_ATTR(port_14_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 13);
static SENSOR_DEVICE_ATTR(port_15_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 14);
static SENSOR_DEVICE_ATTR(port_16_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 15);
static SENSOR_DEVICE_ATTR(port_33_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 16);
static SENSOR_DEVICE_ATTR(port_34_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 17);
static SENSOR_DEVICE_ATTR(port_35_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 18);
static SENSOR_DEVICE_ATTR(port_36_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 19);
static SENSOR_DEVICE_ATTR(port_37_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 20);
static SENSOR_DEVICE_ATTR(port_38_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 21);
static SENSOR_DEVICE_ATTR(port_39_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 22);
static SENSOR_DEVICE_ATTR(port_40_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 23);
static SENSOR_DEVICE_ATTR(port_41_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 24);
static SENSOR_DEVICE_ATTR(port_42_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 25);
static SENSOR_DEVICE_ATTR(port_43_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 26);
static SENSOR_DEVICE_ATTR(port_44_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 27);
static SENSOR_DEVICE_ATTR(port_45_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 28);
static SENSOR_DEVICE_ATTR(port_46_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 29);
static SENSOR_DEVICE_ATTR(port_47_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 30);
static SENSOR_DEVICE_ATTR(port_48_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 31);

static SENSOR_DEVICE_ATTR(port_1_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 0);
static SENSOR_DEVICE_ATTR(port_2_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 1);
static SENSOR_DEVICE_ATTR(port_3_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 2);
static SENSOR_DEVICE_ATTR(port_4_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 3);
static SENSOR_DEVICE_ATTR(port_5_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 4);
static SENSOR_DEVICE_ATTR(port_6_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 5);
static SENSOR_DEVICE_ATTR(port_7_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 6);
static SENSOR_DEVICE_ATTR(port_8_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 7);
static SENSOR_DEVICE_ATTR(port_9_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 8);
static SENSOR_DEVICE_ATTR(port_10_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 9);
static SENSOR_DEVICE_ATTR(port_11_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 10);
static SENSOR_DEVICE_ATTR(port_12_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 11);
static SENSOR_DEVICE_ATTR(port_13_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 12);
static SENSOR_DEVICE_ATTR(port_14_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 13);
static SENSOR_DEVICE_ATTR(port_15_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 14);
static SENSOR_DEVICE_ATTR(port_16_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 15);
static SENSOR_DEVICE_ATTR(port_33_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 16);
static SENSOR_DEVICE_ATTR(port_34_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 17);
static SENSOR_DEVICE_ATTR(port_35_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 18);
static SENSOR_DEVICE_ATTR(port_36_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 19);
static SENSOR_DEVICE_ATTR(port_37_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 20);
static SENSOR_DEVICE_ATTR(port_38_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 21);
static SENSOR_DEVICE_ATTR(port_39_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 22);
static SENSOR_DEVICE_ATTR(port_40_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 23);
static SENSOR_DEVICE_ATTR(port_41_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 24);
static SENSOR_DEVICE_ATTR(port_42_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 25);
static SENSOR_DEVICE_ATTR(port_43_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 26);
static SENSOR_DEVICE_ATTR(port_44_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 27);
static SENSOR_DEVICE_ATTR(port_45_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 28);
static SENSOR_DEVICE_ATTR(port_46_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 29);
static SENSOR_DEVICE_ATTR(port_47_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 30);
static SENSOR_DEVICE_ATTR(port_48_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 31);

static SENSOR_DEVICE_ATTR(port_1_prs, S_IRUGO, show_osfp_prs, NULL, 0);
static SENSOR_DEVICE_ATTR(port_2_prs, S_IRUGO, show_osfp_prs, NULL, 1);
static SENSOR_DEVICE_ATTR(port_3_prs, S_IRUGO, show_osfp_prs, NULL, 2);
static SENSOR_DEVICE_ATTR(port_4_prs, S_IRUGO, show_osfp_prs, NULL, 3);
static SENSOR_DEVICE_ATTR(port_5_prs, S_IRUGO, show_osfp_prs, NULL, 4);
static SENSOR_DEVICE_ATTR(port_6_prs, S_IRUGO, show_osfp_prs, NULL, 5);
static SENSOR_DEVICE_ATTR(port_7_prs, S_IRUGO, show_osfp_prs, NULL, 6);
static SENSOR_DEVICE_ATTR(port_8_prs, S_IRUGO, show_osfp_prs, NULL, 7);
static SENSOR_DEVICE_ATTR(port_9_prs, S_IRUGO, show_osfp_prs, NULL, 8);
static SENSOR_DEVICE_ATTR(port_10_prs, S_IRUGO, show_osfp_prs, NULL, 9);
static SENSOR_DEVICE_ATTR(port_11_prs, S_IRUGO, show_osfp_prs, NULL, 10);
static SENSOR_DEVICE_ATTR(port_12_prs, S_IRUGO, show_osfp_prs, NULL, 11);
static SENSOR_DEVICE_ATTR(port_13_prs, S_IRUGO, show_osfp_prs, NULL, 12);
static SENSOR_DEVICE_ATTR(port_14_prs, S_IRUGO, show_osfp_prs, NULL, 13);
static SENSOR_DEVICE_ATTR(port_15_prs, S_IRUGO, show_osfp_prs, NULL, 14);
static SENSOR_DEVICE_ATTR(port_16_prs, S_IRUGO, show_osfp_prs, NULL, 15);
static SENSOR_DEVICE_ATTR(port_33_prs, S_IRUGO, show_osfp_prs, NULL, 16);
static SENSOR_DEVICE_ATTR(port_34_prs, S_IRUGO, show_osfp_prs, NULL, 17);
static SENSOR_DEVICE_ATTR(port_35_prs, S_IRUGO, show_osfp_prs, NULL, 18);
static SENSOR_DEVICE_ATTR(port_36_prs, S_IRUGO, show_osfp_prs, NULL, 19);
static SENSOR_DEVICE_ATTR(port_37_prs, S_IRUGO, show_osfp_prs, NULL, 20);
static SENSOR_DEVICE_ATTR(port_38_prs, S_IRUGO, show_osfp_prs, NULL, 21);
static SENSOR_DEVICE_ATTR(port_39_prs, S_IRUGO, show_osfp_prs, NULL, 22);
static SENSOR_DEVICE_ATTR(port_40_prs, S_IRUGO, show_osfp_prs, NULL, 23);
static SENSOR_DEVICE_ATTR(port_41_prs, S_IRUGO, show_osfp_prs, NULL, 24);
static SENSOR_DEVICE_ATTR(port_42_prs, S_IRUGO, show_osfp_prs, NULL, 25);
static SENSOR_DEVICE_ATTR(port_43_prs, S_IRUGO, show_osfp_prs, NULL, 26);
static SENSOR_DEVICE_ATTR(port_44_prs, S_IRUGO, show_osfp_prs, NULL, 27);
static SENSOR_DEVICE_ATTR(port_45_prs, S_IRUGO, show_osfp_prs, NULL, 28);
static SENSOR_DEVICE_ATTR(port_46_prs, S_IRUGO, show_osfp_prs, NULL, 29);
static SENSOR_DEVICE_ATTR(port_47_prs, S_IRUGO, show_osfp_prs, NULL, 30);
static SENSOR_DEVICE_ATTR(port_48_prs, S_IRUGO, show_osfp_prs, NULL, 31);

static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 0);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 1);
static SENSOR_DEVICE_ATTR(modprs_reg3, S_IRUGO, show_modprs_reg, NULL, 2);
static SENSOR_DEVICE_ATTR(modprs_reg4, S_IRUGO, show_modprs_reg, NULL, 3);

static SENSOR_DEVICE_ATTR(port_1_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 0);
static SENSOR_DEVICE_ATTR(port_2_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 1);
static SENSOR_DEVICE_ATTR(port_3_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 2);
static SENSOR_DEVICE_ATTR(port_4_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 3);
static SENSOR_DEVICE_ATTR(port_5_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 4);
static SENSOR_DEVICE_ATTR(port_6_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 5);
static SENSOR_DEVICE_ATTR(port_7_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 6);
static SENSOR_DEVICE_ATTR(port_8_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 7);
static SENSOR_DEVICE_ATTR(port_9_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 8);
static SENSOR_DEVICE_ATTR(port_10_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 9);
static SENSOR_DEVICE_ATTR(port_11_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 10);
static SENSOR_DEVICE_ATTR(port_12_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 11);
static SENSOR_DEVICE_ATTR(port_13_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 12);
static SENSOR_DEVICE_ATTR(port_14_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 13);
static SENSOR_DEVICE_ATTR(port_15_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 14);
static SENSOR_DEVICE_ATTR(port_16_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 15);
static SENSOR_DEVICE_ATTR(port_33_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 16);
static SENSOR_DEVICE_ATTR(port_34_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 17);
static SENSOR_DEVICE_ATTR(port_35_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 18);
static SENSOR_DEVICE_ATTR(port_36_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 19);
static SENSOR_DEVICE_ATTR(port_37_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 20);
static SENSOR_DEVICE_ATTR(port_38_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 21);
static SENSOR_DEVICE_ATTR(port_39_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 22);
static SENSOR_DEVICE_ATTR(port_40_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 23);
static SENSOR_DEVICE_ATTR(port_41_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 24);
static SENSOR_DEVICE_ATTR(port_42_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 25);
static SENSOR_DEVICE_ATTR(port_43_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 26);
static SENSOR_DEVICE_ATTR(port_44_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 27);
static SENSOR_DEVICE_ATTR(port_45_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 28);
static SENSOR_DEVICE_ATTR(port_46_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 29);
static SENSOR_DEVICE_ATTR(port_47_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 30);
static SENSOR_DEVICE_ATTR(port_48_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 31);

static SENSOR_DEVICE_ATTR(port_1_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 0);
static SENSOR_DEVICE_ATTR(port_2_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 1);
static SENSOR_DEVICE_ATTR(port_3_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 2);
static SENSOR_DEVICE_ATTR(port_4_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 3);
static SENSOR_DEVICE_ATTR(port_5_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 4);
static SENSOR_DEVICE_ATTR(port_6_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 5);
static SENSOR_DEVICE_ATTR(port_7_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 6);
static SENSOR_DEVICE_ATTR(port_8_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 7);
static SENSOR_DEVICE_ATTR(port_9_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 8);
static SENSOR_DEVICE_ATTR(port_10_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 9);
static SENSOR_DEVICE_ATTR(port_11_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 10);
static SENSOR_DEVICE_ATTR(port_12_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 11);
static SENSOR_DEVICE_ATTR(port_13_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 12);
static SENSOR_DEVICE_ATTR(port_14_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 13);
static SENSOR_DEVICE_ATTR(port_15_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 14);
static SENSOR_DEVICE_ATTR(port_16_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 15);
static SENSOR_DEVICE_ATTR(port_33_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 16);
static SENSOR_DEVICE_ATTR(port_34_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 17);
static SENSOR_DEVICE_ATTR(port_35_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 18);
static SENSOR_DEVICE_ATTR(port_36_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 19);
static SENSOR_DEVICE_ATTR(port_37_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 20);
static SENSOR_DEVICE_ATTR(port_38_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 21);
static SENSOR_DEVICE_ATTR(port_39_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 22);
static SENSOR_DEVICE_ATTR(port_40_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 23);
static SENSOR_DEVICE_ATTR(port_41_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 24);
static SENSOR_DEVICE_ATTR(port_42_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 25);
static SENSOR_DEVICE_ATTR(port_43_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 26);
static SENSOR_DEVICE_ATTR(port_44_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 27);
static SENSOR_DEVICE_ATTR(port_45_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 28);
static SENSOR_DEVICE_ATTR(port_46_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 29);
static SENSOR_DEVICE_ATTR(port_47_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 30);
static SENSOR_DEVICE_ATTR(port_48_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 31);

static SENSOR_DEVICE_ATTR(port_1_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 0);
static SENSOR_DEVICE_ATTR(port_2_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 1);
static SENSOR_DEVICE_ATTR(port_3_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 2);
static SENSOR_DEVICE_ATTR(port_4_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 3);
static SENSOR_DEVICE_ATTR(port_5_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 4);
static SENSOR_DEVICE_ATTR(port_6_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 5);
static SENSOR_DEVICE_ATTR(port_7_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 6);
static SENSOR_DEVICE_ATTR(port_8_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 7);
static SENSOR_DEVICE_ATTR(port_9_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 8);
static SENSOR_DEVICE_ATTR(port_10_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 9);
static SENSOR_DEVICE_ATTR(port_11_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 10);
static SENSOR_DEVICE_ATTR(port_12_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 11);
static SENSOR_DEVICE_ATTR(port_13_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 12);
static SENSOR_DEVICE_ATTR(port_14_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 13);
static SENSOR_DEVICE_ATTR(port_15_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 14);
static SENSOR_DEVICE_ATTR(port_16_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 15);
static SENSOR_DEVICE_ATTR(port_33_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 16);
static SENSOR_DEVICE_ATTR(port_34_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 17);
static SENSOR_DEVICE_ATTR(port_35_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 18);
static SENSOR_DEVICE_ATTR(port_36_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 19);
static SENSOR_DEVICE_ATTR(port_37_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 20);
static SENSOR_DEVICE_ATTR(port_38_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 21);
static SENSOR_DEVICE_ATTR(port_39_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 22);
static SENSOR_DEVICE_ATTR(port_40_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 23);
static SENSOR_DEVICE_ATTR(port_41_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 24);
static SENSOR_DEVICE_ATTR(port_42_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 25);
static SENSOR_DEVICE_ATTR(port_43_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 26);
static SENSOR_DEVICE_ATTR(port_44_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 27);
static SENSOR_DEVICE_ATTR(port_45_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 28);
static SENSOR_DEVICE_ATTR(port_46_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 29);
static SENSOR_DEVICE_ATTR(port_47_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 30);
static SENSOR_DEVICE_ATTR(port_48_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 31);

static struct attribute *port_cpld0_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,

    &sensor_dev_attr_port_65_rx_rate.dev_attr.attr,
    &sensor_dev_attr_port_65_tx_rate.dev_attr.attr,
    &sensor_dev_attr_port_66_rx_rate.dev_attr.attr,
    &sensor_dev_attr_port_66_tx_rate.dev_attr.attr,
    &sensor_dev_attr_port_66_efuse_en.dev_attr.attr,
    &sensor_dev_attr_port_65_efuse_en.dev_attr.attr,
    &sensor_dev_attr_port_65_efuse_flag.dev_attr.attr,
    &sensor_dev_attr_port_66_efuse_flag.dev_attr.attr,
    &sensor_dev_attr_port_65_tx_fault.dev_attr.attr,
    &sensor_dev_attr_port_66_tx_fault.dev_attr.attr,
    &sensor_dev_attr_port_65_tx_en.dev_attr.attr,
    &sensor_dev_attr_port_66_tx_en.dev_attr.attr,
    &sensor_dev_attr_port_65_rx_los.dev_attr.attr,
    &sensor_dev_attr_port_66_rx_los.dev_attr.attr,
    &sensor_dev_attr_port_65_prs.dev_attr.attr,
    &sensor_dev_attr_port_66_prs.dev_attr.attr,
    &sensor_dev_attr_port_65_en.dev_attr.attr,
    &sensor_dev_attr_port_66_en.dev_attr.attr,
    &sensor_dev_attr_port_65_loopb.dev_attr.attr,
    &sensor_dev_attr_port_66_loopb.dev_attr.attr,

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
    &sensor_dev_attr_port_33_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_34_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_35_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_36_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_37_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_38_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_39_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_40_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_41_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_42_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_43_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_44_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_45_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_46_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_47_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_48_lpmod.dev_attr.attr,

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
    &sensor_dev_attr_port_33_rst.dev_attr.attr,
    &sensor_dev_attr_port_34_rst.dev_attr.attr,
    &sensor_dev_attr_port_35_rst.dev_attr.attr,
    &sensor_dev_attr_port_36_rst.dev_attr.attr,
    &sensor_dev_attr_port_37_rst.dev_attr.attr,
    &sensor_dev_attr_port_38_rst.dev_attr.attr,
    &sensor_dev_attr_port_39_rst.dev_attr.attr,
    &sensor_dev_attr_port_40_rst.dev_attr.attr,
    &sensor_dev_attr_port_41_rst.dev_attr.attr,
    &sensor_dev_attr_port_42_rst.dev_attr.attr,
    &sensor_dev_attr_port_43_rst.dev_attr.attr,
    &sensor_dev_attr_port_44_rst.dev_attr.attr,
    &sensor_dev_attr_port_45_rst.dev_attr.attr,
    &sensor_dev_attr_port_46_rst.dev_attr.attr,
    &sensor_dev_attr_port_47_rst.dev_attr.attr,
    &sensor_dev_attr_port_48_rst.dev_attr.attr,

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
    &sensor_dev_attr_port_33_prs.dev_attr.attr,
    &sensor_dev_attr_port_34_prs.dev_attr.attr,
    &sensor_dev_attr_port_35_prs.dev_attr.attr,
    &sensor_dev_attr_port_36_prs.dev_attr.attr,
    &sensor_dev_attr_port_37_prs.dev_attr.attr,
    &sensor_dev_attr_port_38_prs.dev_attr.attr,
    &sensor_dev_attr_port_39_prs.dev_attr.attr,
    &sensor_dev_attr_port_40_prs.dev_attr.attr,
    &sensor_dev_attr_port_41_prs.dev_attr.attr,
    &sensor_dev_attr_port_42_prs.dev_attr.attr,
    &sensor_dev_attr_port_43_prs.dev_attr.attr,
    &sensor_dev_attr_port_44_prs.dev_attr.attr,
    &sensor_dev_attr_port_45_prs.dev_attr.attr,
    &sensor_dev_attr_port_46_prs.dev_attr.attr,
    &sensor_dev_attr_port_47_prs.dev_attr.attr,
    &sensor_dev_attr_port_48_prs.dev_attr.attr,

    &sensor_dev_attr_modprs_reg1.dev_attr.attr,
    &sensor_dev_attr_modprs_reg2.dev_attr.attr,
    &sensor_dev_attr_modprs_reg3.dev_attr.attr,
    &sensor_dev_attr_modprs_reg4.dev_attr.attr,

    &sensor_dev_attr_port_1_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_2_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_3_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_4_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_5_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_6_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_7_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_8_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_9_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_10_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_11_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_12_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_13_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_14_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_15_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_16_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_33_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_34_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_35_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_36_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_37_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_38_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_39_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_40_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_41_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_42_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_43_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_44_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_45_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_46_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_47_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_48_pwgood.dev_attr.attr,

    &sensor_dev_attr_port_1_en.dev_attr.attr,
    &sensor_dev_attr_port_2_en.dev_attr.attr,
    &sensor_dev_attr_port_3_en.dev_attr.attr,
    &sensor_dev_attr_port_4_en.dev_attr.attr,
    &sensor_dev_attr_port_5_en.dev_attr.attr,
    &sensor_dev_attr_port_6_en.dev_attr.attr,
    &sensor_dev_attr_port_7_en.dev_attr.attr,
    &sensor_dev_attr_port_8_en.dev_attr.attr,
    &sensor_dev_attr_port_9_en.dev_attr.attr,
    &sensor_dev_attr_port_10_en.dev_attr.attr,
    &sensor_dev_attr_port_11_en.dev_attr.attr,
    &sensor_dev_attr_port_12_en.dev_attr.attr,
    &sensor_dev_attr_port_13_en.dev_attr.attr,
    &sensor_dev_attr_port_14_en.dev_attr.attr,
    &sensor_dev_attr_port_15_en.dev_attr.attr,
    &sensor_dev_attr_port_16_en.dev_attr.attr,
    &sensor_dev_attr_port_33_en.dev_attr.attr,
    &sensor_dev_attr_port_34_en.dev_attr.attr,
    &sensor_dev_attr_port_35_en.dev_attr.attr,
    &sensor_dev_attr_port_36_en.dev_attr.attr,
    &sensor_dev_attr_port_37_en.dev_attr.attr,
    &sensor_dev_attr_port_38_en.dev_attr.attr,
    &sensor_dev_attr_port_39_en.dev_attr.attr,
    &sensor_dev_attr_port_40_en.dev_attr.attr,
    &sensor_dev_attr_port_41_en.dev_attr.attr,
    &sensor_dev_attr_port_42_en.dev_attr.attr,
    &sensor_dev_attr_port_43_en.dev_attr.attr,
    &sensor_dev_attr_port_44_en.dev_attr.attr,
    &sensor_dev_attr_port_45_en.dev_attr.attr,
    &sensor_dev_attr_port_46_en.dev_attr.attr,
    &sensor_dev_attr_port_47_en.dev_attr.attr,
    &sensor_dev_attr_port_48_en.dev_attr.attr,

    &sensor_dev_attr_port_1_loopb.dev_attr.attr,
    &sensor_dev_attr_port_2_loopb.dev_attr.attr,
    &sensor_dev_attr_port_3_loopb.dev_attr.attr,
    &sensor_dev_attr_port_4_loopb.dev_attr.attr,
    &sensor_dev_attr_port_5_loopb.dev_attr.attr,
    &sensor_dev_attr_port_6_loopb.dev_attr.attr,
    &sensor_dev_attr_port_7_loopb.dev_attr.attr,
    &sensor_dev_attr_port_8_loopb.dev_attr.attr,
    &sensor_dev_attr_port_9_loopb.dev_attr.attr,
    &sensor_dev_attr_port_10_loopb.dev_attr.attr,
    &sensor_dev_attr_port_11_loopb.dev_attr.attr,
    &sensor_dev_attr_port_12_loopb.dev_attr.attr,
    &sensor_dev_attr_port_13_loopb.dev_attr.attr,
    &sensor_dev_attr_port_14_loopb.dev_attr.attr,
    &sensor_dev_attr_port_15_loopb.dev_attr.attr,
    &sensor_dev_attr_port_16_loopb.dev_attr.attr,
    &sensor_dev_attr_port_33_loopb.dev_attr.attr,
    &sensor_dev_attr_port_34_loopb.dev_attr.attr,
    &sensor_dev_attr_port_35_loopb.dev_attr.attr,
    &sensor_dev_attr_port_36_loopb.dev_attr.attr,
    &sensor_dev_attr_port_37_loopb.dev_attr.attr,
    &sensor_dev_attr_port_38_loopb.dev_attr.attr,
    &sensor_dev_attr_port_39_loopb.dev_attr.attr,
    &sensor_dev_attr_port_40_loopb.dev_attr.attr,
    &sensor_dev_attr_port_41_loopb.dev_attr.attr,
    &sensor_dev_attr_port_42_loopb.dev_attr.attr,
    &sensor_dev_attr_port_43_loopb.dev_attr.attr,
    &sensor_dev_attr_port_44_loopb.dev_attr.attr,
    &sensor_dev_attr_port_45_loopb.dev_attr.attr,
    &sensor_dev_attr_port_46_loopb.dev_attr.attr,
    &sensor_dev_attr_port_47_loopb.dev_attr.attr,
    &sensor_dev_attr_port_48_loopb.dev_attr.attr,

    NULL
};

static const struct attribute_group port_cpld0_group = {
    .attrs = port_cpld0_attributes,
};

static int port_cpld0_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia PORT_CPLD0 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &port_cpld0_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    dump_reg(data);
    dev_info(&client->dev, "[PORT_CPLD0]Reseting PORTs ...\n");
    cpld_i2c_write(data, OSFP_LPMODE_REG0, 0x0);
    cpld_i2c_write(data, OSFP_LPMODE_REG0+1, 0x0);
    cpld_i2c_write(data, OSFP_LPMODE_REG0+2, 0x0);
    cpld_i2c_write(data, OSFP_LPMODE_REG0+3, 0x0);
    cpld_i2c_write(data, OSFP_RST_REG0, 0x0);
    cpld_i2c_write(data, OSFP_RST_REG0+1, 0x0);
    cpld_i2c_write(data, OSFP_RST_REG0+2, 0x0);
    cpld_i2c_write(data, OSFP_RST_REG0+3, 0x0);
    msleep(500);
    cpld_i2c_write(data, OSFP_RST_REG0, 0xFF);
    cpld_i2c_write(data, OSFP_RST_REG0+1, 0xFF);
    cpld_i2c_write(data, OSFP_RST_REG0+2, 0xFF);
    cpld_i2c_write(data, OSFP_RST_REG0+3, 0xFF);
    dev_info(&client->dev, "[PORT_CPLD0]PORTs reset done.\n");
    cpld_i2c_write(data, SFP_MISC_REG, 0xC);
    cpld_i2c_write(data, SFP_TXDIS_REG, 0x3);
    cpld_i2c_write(data, SFP_EN_LP_REG, 0x0);
    dump_reg(data);
    
    return 0;

exit:
    return status;
}

static void port_cpld0_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &port_cpld0_group);
    kfree(data);
}

static const struct of_device_id port_cpld0_of_ids[] = {
    {
        .compatible = "nokia,port_cpld0",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, port_cpld0_of_ids);

static const struct i2c_device_id port_cpld0_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, port_cpld0_ids);

static struct i2c_driver port_cpld0_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(port_cpld0_of_ids),
    },
    .probe        = port_cpld0_probe,
    .remove       = port_cpld0_remove,
    .id_table     = port_cpld0_ids,
    .address_list = cpld_address_list,
};

static int __init port_cpld0_init(void)
{
    return i2c_add_driver(&port_cpld0_driver);
}

static void __exit port_cpld0_exit(void)
{
    i2c_del_driver(&port_cpld0_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA H6-64 CPLD0 driver");
MODULE_LICENSE("GPL");

module_init(port_cpld0_init);
module_exit(port_cpld0_exit);
