//  * CPLD driver for Nokia-7220-IXR-H6-128 Router
//  *
//  * Copyright (C) 2026 Nokia Corporation.
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

#define DRIVER_NAME "port_cpld1"

// REGISTERS ADDRESS MAP
#define VER_MAJOR_REG           0x00
#define VER_MINOR_REG           0x01
//#define SFP_CTRL_REG            0x03
#define SCRATCH_REG             0x04
//#define SFP_MISC_REG            0x05
#define SFP_TXFAULT_REG         0x10
#define SFP_TXDIS_REG           0x18
#define SFP_RXLOSS_REG          0x20
#define SFP_MODPRS_REG          0x28
#define PORT_LPMODE_REG0        0x70
#define PORT_RST_REG0           0x78
#define PORT_MODPRS_REG0        0x88
#define PORT_PWGOOD_REG0        0x90

static const unsigned short cpld_address_list[] = {0x75, I2C_CLIENT_END};

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

    val0 = cpld_i2c_read(data, PORT_RST_REG0);
    val1 = cpld_i2c_read(data, PORT_RST_REG0 + 1);
    val2 = cpld_i2c_read(data, PORT_RST_REG0 + 2);
    val3 = cpld_i2c_read(data, PORT_RST_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD1]PORT_RESET_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

    val0 = cpld_i2c_read(data, PORT_LPMODE_REG0);
    val1 = cpld_i2c_read(data, PORT_LPMODE_REG0 + 1);
    val2 = cpld_i2c_read(data, PORT_LPMODE_REG0 + 2);
    val3 = cpld_i2c_read(data, PORT_LPMODE_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD1]PORT_LPMODE_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

    val0 = cpld_i2c_read(data, PORT_MODPRS_REG0);
    val1 = cpld_i2c_read(data, PORT_MODPRS_REG0 + 1);
    val2 = cpld_i2c_read(data, PORT_MODPRS_REG0 + 2);
    val3 = cpld_i2c_read(data, PORT_MODPRS_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD1]PORT_MODPRES_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

}

static ssize_t show_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 reg_major = 0;
    u8 reg_minor = 0;

    reg_major = cpld_i2c_read(data, VER_MAJOR_REG);
    reg_minor = cpld_i2c_read(data, VER_MINOR_REG);

    return sprintf(buf, "%02x.%02x\n", reg_major, reg_minor);
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

static ssize_t show_port_lpmode(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PORT_LPMODE_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t set_port_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, PORT_LPMODE_REG0 + (sda->index / 8));
    reg_val = reg_val & mask;
    usr_val = usr_val << (sda->index % 8);
    cpld_i2c_write(data, PORT_LPMODE_REG0 + (sda->index / 8), (reg_val | usr_val));

    return count;
}

static ssize_t show_port_rst(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PORT_RST_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t set_port_rst(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, PORT_RST_REG0 + (sda->index / 8));
    reg_val = reg_val & mask;
    usr_val = usr_val << (sda->index % 8);
    cpld_i2c_write(data, PORT_RST_REG0 + (sda->index / 8), (reg_val | usr_val));

    return count;
}

static ssize_t show_port_prs(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PORT_MODPRS_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t show_modprs_reg(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PORT_MODPRS_REG0 + sda->index);
    
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_port_pwgood(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PORT_PWGOOD_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);

static SENSOR_DEVICE_ATTR(port_33_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, 0);
static SENSOR_DEVICE_ATTR(port_34_tx_fault, S_IRUGO, show_sfp_tx_fault, NULL, 1);
static SENSOR_DEVICE_ATTR(port_33_tx_en, S_IRUGO | S_IWUSR, show_sfp_tx_en, set_sfp_tx_en, 0);
static SENSOR_DEVICE_ATTR(port_34_tx_en, S_IRUGO | S_IWUSR, show_sfp_tx_en, set_sfp_tx_en, 1);
static SENSOR_DEVICE_ATTR(port_33_rx_los, S_IRUGO, show_sfp_rx_los, NULL, 0);
static SENSOR_DEVICE_ATTR(port_34_rx_los, S_IRUGO, show_sfp_rx_los, NULL, 1);
static SENSOR_DEVICE_ATTR(port_33_prs, S_IRUGO, show_sfp_prs, NULL, 0);
static SENSOR_DEVICE_ATTR(port_34_prs, S_IRUGO, show_sfp_prs, NULL, 1);

static SENSOR_DEVICE_ATTR(port_1_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 0);
static SENSOR_DEVICE_ATTR(port_2_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 1);
static SENSOR_DEVICE_ATTR(port_3_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 2);
static SENSOR_DEVICE_ATTR(port_4_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 3);
static SENSOR_DEVICE_ATTR(port_5_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 4);
static SENSOR_DEVICE_ATTR(port_6_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 5);
static SENSOR_DEVICE_ATTR(port_7_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 6);
static SENSOR_DEVICE_ATTR(port_8_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 7);
static SENSOR_DEVICE_ATTR(port_9_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 8);
static SENSOR_DEVICE_ATTR(port_10_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 9);
static SENSOR_DEVICE_ATTR(port_11_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 10);
static SENSOR_DEVICE_ATTR(port_12_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 11);
static SENSOR_DEVICE_ATTR(port_13_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 12);
static SENSOR_DEVICE_ATTR(port_14_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 13);
static SENSOR_DEVICE_ATTR(port_15_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 14);
static SENSOR_DEVICE_ATTR(port_16_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 15);
static SENSOR_DEVICE_ATTR(port_17_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 16);
static SENSOR_DEVICE_ATTR(port_18_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 17);
static SENSOR_DEVICE_ATTR(port_19_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 18);
static SENSOR_DEVICE_ATTR(port_20_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 19);
static SENSOR_DEVICE_ATTR(port_21_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 20);
static SENSOR_DEVICE_ATTR(port_22_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 21);
static SENSOR_DEVICE_ATTR(port_23_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 22);
static SENSOR_DEVICE_ATTR(port_24_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 23);
static SENSOR_DEVICE_ATTR(port_25_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 24);
static SENSOR_DEVICE_ATTR(port_26_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 25);
static SENSOR_DEVICE_ATTR(port_27_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 26);
static SENSOR_DEVICE_ATTR(port_28_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 27);
static SENSOR_DEVICE_ATTR(port_29_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 28);
static SENSOR_DEVICE_ATTR(port_30_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 29);
static SENSOR_DEVICE_ATTR(port_31_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 30);
static SENSOR_DEVICE_ATTR(port_32_lpmod, S_IRUGO | S_IWUSR, show_port_lpmode, set_port_lpmode, 31);

static SENSOR_DEVICE_ATTR(port_1_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 0);
static SENSOR_DEVICE_ATTR(port_2_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 1);
static SENSOR_DEVICE_ATTR(port_3_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 2);
static SENSOR_DEVICE_ATTR(port_4_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 3);
static SENSOR_DEVICE_ATTR(port_5_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 4);
static SENSOR_DEVICE_ATTR(port_6_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 5);
static SENSOR_DEVICE_ATTR(port_7_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 6);
static SENSOR_DEVICE_ATTR(port_8_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 7);
static SENSOR_DEVICE_ATTR(port_9_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 8);
static SENSOR_DEVICE_ATTR(port_10_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 9);
static SENSOR_DEVICE_ATTR(port_11_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 10);
static SENSOR_DEVICE_ATTR(port_12_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 11);
static SENSOR_DEVICE_ATTR(port_13_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 12);
static SENSOR_DEVICE_ATTR(port_14_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 13);
static SENSOR_DEVICE_ATTR(port_15_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 14);
static SENSOR_DEVICE_ATTR(port_16_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 15);
static SENSOR_DEVICE_ATTR(port_17_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 16);
static SENSOR_DEVICE_ATTR(port_18_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 17);
static SENSOR_DEVICE_ATTR(port_19_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 18);
static SENSOR_DEVICE_ATTR(port_20_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 19);
static SENSOR_DEVICE_ATTR(port_21_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 20);
static SENSOR_DEVICE_ATTR(port_22_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 21);
static SENSOR_DEVICE_ATTR(port_23_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 22);
static SENSOR_DEVICE_ATTR(port_24_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 23);
static SENSOR_DEVICE_ATTR(port_25_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 24);
static SENSOR_DEVICE_ATTR(port_26_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 25);
static SENSOR_DEVICE_ATTR(port_27_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 26);
static SENSOR_DEVICE_ATTR(port_28_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 27);
static SENSOR_DEVICE_ATTR(port_29_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 28);
static SENSOR_DEVICE_ATTR(port_30_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 29);
static SENSOR_DEVICE_ATTR(port_31_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 30);
static SENSOR_DEVICE_ATTR(port_32_rst, S_IRUGO | S_IWUSR, show_port_rst, set_port_rst, 31);

static SENSOR_DEVICE_ATTR(port_1_prs, S_IRUGO, show_port_prs, NULL, 0);
static SENSOR_DEVICE_ATTR(port_2_prs, S_IRUGO, show_port_prs, NULL, 1);
static SENSOR_DEVICE_ATTR(port_3_prs, S_IRUGO, show_port_prs, NULL, 2);
static SENSOR_DEVICE_ATTR(port_4_prs, S_IRUGO, show_port_prs, NULL, 3);
static SENSOR_DEVICE_ATTR(port_5_prs, S_IRUGO, show_port_prs, NULL, 4);
static SENSOR_DEVICE_ATTR(port_6_prs, S_IRUGO, show_port_prs, NULL, 5);
static SENSOR_DEVICE_ATTR(port_7_prs, S_IRUGO, show_port_prs, NULL, 6);
static SENSOR_DEVICE_ATTR(port_8_prs, S_IRUGO, show_port_prs, NULL, 7);
static SENSOR_DEVICE_ATTR(port_9_prs, S_IRUGO, show_port_prs, NULL, 8);
static SENSOR_DEVICE_ATTR(port_10_prs, S_IRUGO, show_port_prs, NULL, 9);
static SENSOR_DEVICE_ATTR(port_11_prs, S_IRUGO, show_port_prs, NULL, 10);
static SENSOR_DEVICE_ATTR(port_12_prs, S_IRUGO, show_port_prs, NULL, 11);
static SENSOR_DEVICE_ATTR(port_13_prs, S_IRUGO, show_port_prs, NULL, 12);
static SENSOR_DEVICE_ATTR(port_14_prs, S_IRUGO, show_port_prs, NULL, 13);
static SENSOR_DEVICE_ATTR(port_15_prs, S_IRUGO, show_port_prs, NULL, 14);
static SENSOR_DEVICE_ATTR(port_16_prs, S_IRUGO, show_port_prs, NULL, 15);
static SENSOR_DEVICE_ATTR(port_17_prs, S_IRUGO, show_port_prs, NULL, 16);
static SENSOR_DEVICE_ATTR(port_18_prs, S_IRUGO, show_port_prs, NULL, 17);
static SENSOR_DEVICE_ATTR(port_19_prs, S_IRUGO, show_port_prs, NULL, 18);
static SENSOR_DEVICE_ATTR(port_20_prs, S_IRUGO, show_port_prs, NULL, 19);
static SENSOR_DEVICE_ATTR(port_21_prs, S_IRUGO, show_port_prs, NULL, 20);
static SENSOR_DEVICE_ATTR(port_22_prs, S_IRUGO, show_port_prs, NULL, 21);
static SENSOR_DEVICE_ATTR(port_23_prs, S_IRUGO, show_port_prs, NULL, 22);
static SENSOR_DEVICE_ATTR(port_24_prs, S_IRUGO, show_port_prs, NULL, 23);
static SENSOR_DEVICE_ATTR(port_25_prs, S_IRUGO, show_port_prs, NULL, 24);
static SENSOR_DEVICE_ATTR(port_26_prs, S_IRUGO, show_port_prs, NULL, 25);
static SENSOR_DEVICE_ATTR(port_27_prs, S_IRUGO, show_port_prs, NULL, 26);
static SENSOR_DEVICE_ATTR(port_28_prs, S_IRUGO, show_port_prs, NULL, 27);
static SENSOR_DEVICE_ATTR(port_29_prs, S_IRUGO, show_port_prs, NULL, 28);
static SENSOR_DEVICE_ATTR(port_30_prs, S_IRUGO, show_port_prs, NULL, 29);
static SENSOR_DEVICE_ATTR(port_31_prs, S_IRUGO, show_port_prs, NULL, 30);
static SENSOR_DEVICE_ATTR(port_32_prs, S_IRUGO, show_port_prs, NULL, 31);

static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 0);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 1);
static SENSOR_DEVICE_ATTR(modprs_reg3, S_IRUGO, show_modprs_reg, NULL, 2);
static SENSOR_DEVICE_ATTR(modprs_reg4, S_IRUGO, show_modprs_reg, NULL, 3);

static SENSOR_DEVICE_ATTR(port_1_pwgood, S_IRUGO, show_port_pwgood, NULL, 0);
static SENSOR_DEVICE_ATTR(port_2_pwgood, S_IRUGO, show_port_pwgood, NULL, 1);
static SENSOR_DEVICE_ATTR(port_3_pwgood, S_IRUGO, show_port_pwgood, NULL, 2);
static SENSOR_DEVICE_ATTR(port_4_pwgood, S_IRUGO, show_port_pwgood, NULL, 3);
static SENSOR_DEVICE_ATTR(port_5_pwgood, S_IRUGO, show_port_pwgood, NULL, 4);
static SENSOR_DEVICE_ATTR(port_6_pwgood, S_IRUGO, show_port_pwgood, NULL, 5);
static SENSOR_DEVICE_ATTR(port_7_pwgood, S_IRUGO, show_port_pwgood, NULL, 6);
static SENSOR_DEVICE_ATTR(port_8_pwgood, S_IRUGO, show_port_pwgood, NULL, 7);
static SENSOR_DEVICE_ATTR(port_9_pwgood, S_IRUGO, show_port_pwgood, NULL, 8);
static SENSOR_DEVICE_ATTR(port_10_pwgood, S_IRUGO, show_port_pwgood, NULL, 9);
static SENSOR_DEVICE_ATTR(port_11_pwgood, S_IRUGO, show_port_pwgood, NULL, 10);
static SENSOR_DEVICE_ATTR(port_12_pwgood, S_IRUGO, show_port_pwgood, NULL, 11);
static SENSOR_DEVICE_ATTR(port_13_pwgood, S_IRUGO, show_port_pwgood, NULL, 12);
static SENSOR_DEVICE_ATTR(port_14_pwgood, S_IRUGO, show_port_pwgood, NULL, 13);
static SENSOR_DEVICE_ATTR(port_15_pwgood, S_IRUGO, show_port_pwgood, NULL, 14);
static SENSOR_DEVICE_ATTR(port_16_pwgood, S_IRUGO, show_port_pwgood, NULL, 15);
static SENSOR_DEVICE_ATTR(port_17_pwgood, S_IRUGO, show_port_pwgood, NULL, 16);
static SENSOR_DEVICE_ATTR(port_18_pwgood, S_IRUGO, show_port_pwgood, NULL, 17);
static SENSOR_DEVICE_ATTR(port_19_pwgood, S_IRUGO, show_port_pwgood, NULL, 18);
static SENSOR_DEVICE_ATTR(port_20_pwgood, S_IRUGO, show_port_pwgood, NULL, 19);
static SENSOR_DEVICE_ATTR(port_21_pwgood, S_IRUGO, show_port_pwgood, NULL, 20);
static SENSOR_DEVICE_ATTR(port_22_pwgood, S_IRUGO, show_port_pwgood, NULL, 21);
static SENSOR_DEVICE_ATTR(port_23_pwgood, S_IRUGO, show_port_pwgood, NULL, 22);
static SENSOR_DEVICE_ATTR(port_24_pwgood, S_IRUGO, show_port_pwgood, NULL, 23);
static SENSOR_DEVICE_ATTR(port_25_pwgood, S_IRUGO, show_port_pwgood, NULL, 24);
static SENSOR_DEVICE_ATTR(port_26_pwgood, S_IRUGO, show_port_pwgood, NULL, 25);
static SENSOR_DEVICE_ATTR(port_27_pwgood, S_IRUGO, show_port_pwgood, NULL, 26);
static SENSOR_DEVICE_ATTR(port_28_pwgood, S_IRUGO, show_port_pwgood, NULL, 27);
static SENSOR_DEVICE_ATTR(port_29_pwgood, S_IRUGO, show_port_pwgood, NULL, 28);
static SENSOR_DEVICE_ATTR(port_30_pwgood, S_IRUGO, show_port_pwgood, NULL, 29);
static SENSOR_DEVICE_ATTR(port_31_pwgood, S_IRUGO, show_port_pwgood, NULL, 30);
static SENSOR_DEVICE_ATTR(port_32_pwgood, S_IRUGO, show_port_pwgood, NULL, 31);

static struct attribute *port_cpld1_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,

    &sensor_dev_attr_port_33_tx_fault.dev_attr.attr,
    &sensor_dev_attr_port_34_tx_fault.dev_attr.attr,
    &sensor_dev_attr_port_33_tx_en.dev_attr.attr,
    &sensor_dev_attr_port_34_tx_en.dev_attr.attr,
    &sensor_dev_attr_port_33_rx_los.dev_attr.attr,
    &sensor_dev_attr_port_34_rx_los.dev_attr.attr,
    &sensor_dev_attr_port_33_prs.dev_attr.attr,
    &sensor_dev_attr_port_34_prs.dev_attr.attr,

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
    &sensor_dev_attr_port_17_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_18_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_19_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_20_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_21_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_22_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_23_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_24_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_25_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_26_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_27_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_28_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_29_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_30_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_31_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_32_pwgood.dev_attr.attr,

    NULL
};

static const struct attribute_group port_cpld1_group = {
    .attrs = port_cpld1_attributes,
};

static int port_cpld1_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia PORT_CPLD1 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &port_cpld1_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    dump_reg(data);
    dev_info(&client->dev, "[PORT_CPLD1]Reseting PORTs ...\n");
    cpld_i2c_write(data, PORT_LPMODE_REG0, 0x0);
    cpld_i2c_write(data, PORT_LPMODE_REG0+1, 0x0);
    cpld_i2c_write(data, PORT_LPMODE_REG0+2, 0x0);
    cpld_i2c_write(data, PORT_LPMODE_REG0+3, 0x0);
    cpld_i2c_write(data, PORT_RST_REG0, 0x0);
    cpld_i2c_write(data, PORT_RST_REG0+1, 0x0);
    cpld_i2c_write(data, PORT_RST_REG0+2, 0x0);
    cpld_i2c_write(data, PORT_RST_REG0+3, 0x0);
    msleep(500);
    cpld_i2c_write(data, PORT_RST_REG0, 0xFF);
    cpld_i2c_write(data, PORT_RST_REG0+1, 0xFF);
    cpld_i2c_write(data, PORT_RST_REG0+2, 0xFF);
    cpld_i2c_write(data, PORT_RST_REG0+3, 0xFF);
    dev_info(&client->dev, "[PORT_CPLD1]PORTs reset done.\n");
    dump_reg(data);
    
    return 0;

exit:
    return status;
}

static void port_cpld1_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &port_cpld1_group);
    kfree(data);
}

static const struct of_device_id port_cpld1_of_ids[] = {
    {
        .compatible = "port_cpld1",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, port_cpld1_of_ids);

static const struct i2c_device_id port_cpld1_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, port_cpld1_ids);

static struct i2c_driver port_cpld1_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(port_cpld1_of_ids),
    },
    .probe        = port_cpld1_probe,
    .remove       = port_cpld1_remove,
    .id_table     = port_cpld1_ids,
    .address_list = cpld_address_list,
};

static int __init port_cpld1_init(void)
{
    return i2c_add_driver(&port_cpld1_driver);
}

static void __exit port_cpld1_exit(void)
{
    i2c_del_driver(&port_cpld1_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA H6-128 PORT_CPLD1 driver");
MODULE_LICENSE("GPL");

module_init(port_cpld1_init);
module_exit(port_cpld1_exit);
