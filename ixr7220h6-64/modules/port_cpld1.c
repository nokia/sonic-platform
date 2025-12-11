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

#define DRIVER_NAME "port_cpld1"

// REGISTERS ADDRESS MAP
#define VER_MAJOR_REG           0x00
#define VER_MINOR_REG           0x01
#define SCRATCH_REG             0x04
#define OSFP_LPMODE_REG0        0x70
#define OSFP_RST_REG0           0x78
#define OSFP_MODPRS_REG0        0x88
#define OSFP_PWGOOD_REG0        0x90
#define OSFP_ENABLE_REG0        0x94
#define OSFP_LOOPBK_REG0        0x98

// REG BIT FIELD POSITION or MASK


static const unsigned short cpld_address_list[] = {0x65, I2C_CLIENT_END};

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
    dev_info(&client->dev, "[PORT_CPLD1]OSFP_RESET_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

    val0 = cpld_i2c_read(data, OSFP_LPMODE_REG0);
    val1 = cpld_i2c_read(data, OSFP_LPMODE_REG0 + 1);
    val2 = cpld_i2c_read(data, OSFP_LPMODE_REG0 + 2);
    val3 = cpld_i2c_read(data, OSFP_LPMODE_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD1]OSFP_LPMODE_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

    val0 = cpld_i2c_read(data, OSFP_MODPRS_REG0);
    val1 = cpld_i2c_read(data, OSFP_MODPRS_REG0 + 1);
    val2 = cpld_i2c_read(data, OSFP_MODPRS_REG0 + 2);
    val3 = cpld_i2c_read(data, OSFP_MODPRS_REG0 + 3);
    dev_info(&client->dev, "[PORT_CPLD1]OSFP_MODPRES_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

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

static SENSOR_DEVICE_ATTR(port_17_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 0);
static SENSOR_DEVICE_ATTR(port_18_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 1);
static SENSOR_DEVICE_ATTR(port_19_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 2);
static SENSOR_DEVICE_ATTR(port_20_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 3);
static SENSOR_DEVICE_ATTR(port_21_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 4);
static SENSOR_DEVICE_ATTR(port_22_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 5);
static SENSOR_DEVICE_ATTR(port_23_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 6);
static SENSOR_DEVICE_ATTR(port_24_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 7);
static SENSOR_DEVICE_ATTR(port_25_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 8);
static SENSOR_DEVICE_ATTR(port_26_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 9);
static SENSOR_DEVICE_ATTR(port_27_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 10);
static SENSOR_DEVICE_ATTR(port_28_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 11);
static SENSOR_DEVICE_ATTR(port_29_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 12);
static SENSOR_DEVICE_ATTR(port_30_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 13);
static SENSOR_DEVICE_ATTR(port_31_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 14);
static SENSOR_DEVICE_ATTR(port_32_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 15);
static SENSOR_DEVICE_ATTR(port_49_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 16);
static SENSOR_DEVICE_ATTR(port_50_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 17);
static SENSOR_DEVICE_ATTR(port_51_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 18);
static SENSOR_DEVICE_ATTR(port_52_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 19);
static SENSOR_DEVICE_ATTR(port_53_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 20);
static SENSOR_DEVICE_ATTR(port_54_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 21);
static SENSOR_DEVICE_ATTR(port_55_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 22);
static SENSOR_DEVICE_ATTR(port_56_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 23);
static SENSOR_DEVICE_ATTR(port_57_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 24);
static SENSOR_DEVICE_ATTR(port_58_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 25);
static SENSOR_DEVICE_ATTR(port_59_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 26);
static SENSOR_DEVICE_ATTR(port_60_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 27);
static SENSOR_DEVICE_ATTR(port_61_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 28);
static SENSOR_DEVICE_ATTR(port_62_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 29);
static SENSOR_DEVICE_ATTR(port_63_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 30);
static SENSOR_DEVICE_ATTR(port_64_lpmod, S_IRUGO | S_IWUSR, show_osfp_lpmode, set_osfp_lpmode, 31);

static SENSOR_DEVICE_ATTR(port_17_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 0);
static SENSOR_DEVICE_ATTR(port_18_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 1);
static SENSOR_DEVICE_ATTR(port_19_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 2);
static SENSOR_DEVICE_ATTR(port_20_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 3);
static SENSOR_DEVICE_ATTR(port_21_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 4);
static SENSOR_DEVICE_ATTR(port_22_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 5);
static SENSOR_DEVICE_ATTR(port_23_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 6);
static SENSOR_DEVICE_ATTR(port_24_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 7);
static SENSOR_DEVICE_ATTR(port_25_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 8);
static SENSOR_DEVICE_ATTR(port_26_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 9);
static SENSOR_DEVICE_ATTR(port_27_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 10);
static SENSOR_DEVICE_ATTR(port_28_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 11);
static SENSOR_DEVICE_ATTR(port_29_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 12);
static SENSOR_DEVICE_ATTR(port_30_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 13);
static SENSOR_DEVICE_ATTR(port_31_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 14);
static SENSOR_DEVICE_ATTR(port_32_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 15);
static SENSOR_DEVICE_ATTR(port_49_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 16);
static SENSOR_DEVICE_ATTR(port_50_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 17);
static SENSOR_DEVICE_ATTR(port_51_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 18);
static SENSOR_DEVICE_ATTR(port_52_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 19);
static SENSOR_DEVICE_ATTR(port_53_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 20);
static SENSOR_DEVICE_ATTR(port_54_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 21);
static SENSOR_DEVICE_ATTR(port_55_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 22);
static SENSOR_DEVICE_ATTR(port_56_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 23);
static SENSOR_DEVICE_ATTR(port_57_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 24);
static SENSOR_DEVICE_ATTR(port_58_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 25);
static SENSOR_DEVICE_ATTR(port_59_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 26);
static SENSOR_DEVICE_ATTR(port_60_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 27);
static SENSOR_DEVICE_ATTR(port_61_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 28);
static SENSOR_DEVICE_ATTR(port_62_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 29);
static SENSOR_DEVICE_ATTR(port_63_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 30);
static SENSOR_DEVICE_ATTR(port_64_rst, S_IRUGO | S_IWUSR, show_osfp_rst, set_osfp_rst, 31);

static SENSOR_DEVICE_ATTR(port_17_prs, S_IRUGO, show_osfp_prs, NULL, 0);
static SENSOR_DEVICE_ATTR(port_18_prs, S_IRUGO, show_osfp_prs, NULL, 1);
static SENSOR_DEVICE_ATTR(port_19_prs, S_IRUGO, show_osfp_prs, NULL, 2);
static SENSOR_DEVICE_ATTR(port_20_prs, S_IRUGO, show_osfp_prs, NULL, 3);
static SENSOR_DEVICE_ATTR(port_21_prs, S_IRUGO, show_osfp_prs, NULL, 4);
static SENSOR_DEVICE_ATTR(port_22_prs, S_IRUGO, show_osfp_prs, NULL, 5);
static SENSOR_DEVICE_ATTR(port_23_prs, S_IRUGO, show_osfp_prs, NULL, 6);
static SENSOR_DEVICE_ATTR(port_24_prs, S_IRUGO, show_osfp_prs, NULL, 7);
static SENSOR_DEVICE_ATTR(port_25_prs, S_IRUGO, show_osfp_prs, NULL, 8);
static SENSOR_DEVICE_ATTR(port_26_prs, S_IRUGO, show_osfp_prs, NULL, 9);
static SENSOR_DEVICE_ATTR(port_27_prs, S_IRUGO, show_osfp_prs, NULL, 10);
static SENSOR_DEVICE_ATTR(port_28_prs, S_IRUGO, show_osfp_prs, NULL, 11);
static SENSOR_DEVICE_ATTR(port_29_prs, S_IRUGO, show_osfp_prs, NULL, 12);
static SENSOR_DEVICE_ATTR(port_30_prs, S_IRUGO, show_osfp_prs, NULL, 13);
static SENSOR_DEVICE_ATTR(port_31_prs, S_IRUGO, show_osfp_prs, NULL, 14);
static SENSOR_DEVICE_ATTR(port_32_prs, S_IRUGO, show_osfp_prs, NULL, 15);
static SENSOR_DEVICE_ATTR(port_49_prs, S_IRUGO, show_osfp_prs, NULL, 16);
static SENSOR_DEVICE_ATTR(port_50_prs, S_IRUGO, show_osfp_prs, NULL, 17);
static SENSOR_DEVICE_ATTR(port_51_prs, S_IRUGO, show_osfp_prs, NULL, 18);
static SENSOR_DEVICE_ATTR(port_52_prs, S_IRUGO, show_osfp_prs, NULL, 19);
static SENSOR_DEVICE_ATTR(port_53_prs, S_IRUGO, show_osfp_prs, NULL, 20);
static SENSOR_DEVICE_ATTR(port_54_prs, S_IRUGO, show_osfp_prs, NULL, 21);
static SENSOR_DEVICE_ATTR(port_55_prs, S_IRUGO, show_osfp_prs, NULL, 22);
static SENSOR_DEVICE_ATTR(port_56_prs, S_IRUGO, show_osfp_prs, NULL, 23);
static SENSOR_DEVICE_ATTR(port_57_prs, S_IRUGO, show_osfp_prs, NULL, 24);
static SENSOR_DEVICE_ATTR(port_58_prs, S_IRUGO, show_osfp_prs, NULL, 25);
static SENSOR_DEVICE_ATTR(port_59_prs, S_IRUGO, show_osfp_prs, NULL, 26);
static SENSOR_DEVICE_ATTR(port_60_prs, S_IRUGO, show_osfp_prs, NULL, 27);
static SENSOR_DEVICE_ATTR(port_61_prs, S_IRUGO, show_osfp_prs, NULL, 28);
static SENSOR_DEVICE_ATTR(port_62_prs, S_IRUGO, show_osfp_prs, NULL, 29);
static SENSOR_DEVICE_ATTR(port_63_prs, S_IRUGO, show_osfp_prs, NULL, 30);
static SENSOR_DEVICE_ATTR(port_64_prs, S_IRUGO, show_osfp_prs, NULL, 31);

static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 0);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 1);
static SENSOR_DEVICE_ATTR(modprs_reg3, S_IRUGO, show_modprs_reg, NULL, 2);
static SENSOR_DEVICE_ATTR(modprs_reg4, S_IRUGO, show_modprs_reg, NULL, 3);

static SENSOR_DEVICE_ATTR(port_17_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 0);
static SENSOR_DEVICE_ATTR(port_18_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 1);
static SENSOR_DEVICE_ATTR(port_19_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 2);
static SENSOR_DEVICE_ATTR(port_20_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 3);
static SENSOR_DEVICE_ATTR(port_21_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 4);
static SENSOR_DEVICE_ATTR(port_22_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 5);
static SENSOR_DEVICE_ATTR(port_23_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 6);
static SENSOR_DEVICE_ATTR(port_24_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 7);
static SENSOR_DEVICE_ATTR(port_25_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 8);
static SENSOR_DEVICE_ATTR(port_26_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 9);
static SENSOR_DEVICE_ATTR(port_27_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 10);
static SENSOR_DEVICE_ATTR(port_28_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 11);
static SENSOR_DEVICE_ATTR(port_29_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 12);
static SENSOR_DEVICE_ATTR(port_30_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 13);
static SENSOR_DEVICE_ATTR(port_31_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 14);
static SENSOR_DEVICE_ATTR(port_32_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 15);
static SENSOR_DEVICE_ATTR(port_49_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 16);
static SENSOR_DEVICE_ATTR(port_50_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 17);
static SENSOR_DEVICE_ATTR(port_51_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 18);
static SENSOR_DEVICE_ATTR(port_52_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 19);
static SENSOR_DEVICE_ATTR(port_53_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 20);
static SENSOR_DEVICE_ATTR(port_54_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 21);
static SENSOR_DEVICE_ATTR(port_55_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 22);
static SENSOR_DEVICE_ATTR(port_56_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 23);
static SENSOR_DEVICE_ATTR(port_57_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 24);
static SENSOR_DEVICE_ATTR(port_58_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 25);
static SENSOR_DEVICE_ATTR(port_59_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 26);
static SENSOR_DEVICE_ATTR(port_60_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 27);
static SENSOR_DEVICE_ATTR(port_61_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 28);
static SENSOR_DEVICE_ATTR(port_62_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 29);
static SENSOR_DEVICE_ATTR(port_63_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 30);
static SENSOR_DEVICE_ATTR(port_64_pwgood, S_IRUGO, show_osfp_pwgood, NULL, 31);

static SENSOR_DEVICE_ATTR(port_17_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 0);
static SENSOR_DEVICE_ATTR(port_18_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 1);
static SENSOR_DEVICE_ATTR(port_19_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 2);
static SENSOR_DEVICE_ATTR(port_20_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 3);
static SENSOR_DEVICE_ATTR(port_21_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 4);
static SENSOR_DEVICE_ATTR(port_22_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 5);
static SENSOR_DEVICE_ATTR(port_23_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 6);
static SENSOR_DEVICE_ATTR(port_24_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 7);
static SENSOR_DEVICE_ATTR(port_25_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 8);
static SENSOR_DEVICE_ATTR(port_26_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 9);
static SENSOR_DEVICE_ATTR(port_27_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 10);
static SENSOR_DEVICE_ATTR(port_28_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 11);
static SENSOR_DEVICE_ATTR(port_29_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 12);
static SENSOR_DEVICE_ATTR(port_30_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 13);
static SENSOR_DEVICE_ATTR(port_31_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 14);
static SENSOR_DEVICE_ATTR(port_32_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 15);
static SENSOR_DEVICE_ATTR(port_49_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 16);
static SENSOR_DEVICE_ATTR(port_50_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 17);
static SENSOR_DEVICE_ATTR(port_51_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 18);
static SENSOR_DEVICE_ATTR(port_52_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 19);
static SENSOR_DEVICE_ATTR(port_53_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 20);
static SENSOR_DEVICE_ATTR(port_54_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 21);
static SENSOR_DEVICE_ATTR(port_55_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 22);
static SENSOR_DEVICE_ATTR(port_56_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 23);
static SENSOR_DEVICE_ATTR(port_57_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 24);
static SENSOR_DEVICE_ATTR(port_58_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 25);
static SENSOR_DEVICE_ATTR(port_59_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 26);
static SENSOR_DEVICE_ATTR(port_60_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 27);
static SENSOR_DEVICE_ATTR(port_61_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 28);
static SENSOR_DEVICE_ATTR(port_62_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 29);
static SENSOR_DEVICE_ATTR(port_63_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 30);
static SENSOR_DEVICE_ATTR(port_64_en, S_IRUGO | S_IWUSR, show_osfp_en, set_osfp_en, 31);

static SENSOR_DEVICE_ATTR(port_17_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 0);
static SENSOR_DEVICE_ATTR(port_18_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 1);
static SENSOR_DEVICE_ATTR(port_19_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 2);
static SENSOR_DEVICE_ATTR(port_20_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 3);
static SENSOR_DEVICE_ATTR(port_21_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 4);
static SENSOR_DEVICE_ATTR(port_22_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 5);
static SENSOR_DEVICE_ATTR(port_23_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 6);
static SENSOR_DEVICE_ATTR(port_24_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 7);
static SENSOR_DEVICE_ATTR(port_25_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 8);
static SENSOR_DEVICE_ATTR(port_26_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 9);
static SENSOR_DEVICE_ATTR(port_27_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 10);
static SENSOR_DEVICE_ATTR(port_28_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 11);
static SENSOR_DEVICE_ATTR(port_29_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 12);
static SENSOR_DEVICE_ATTR(port_30_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 13);
static SENSOR_DEVICE_ATTR(port_31_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 14);
static SENSOR_DEVICE_ATTR(port_32_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 15);
static SENSOR_DEVICE_ATTR(port_49_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 16);
static SENSOR_DEVICE_ATTR(port_50_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 17);
static SENSOR_DEVICE_ATTR(port_51_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 18);
static SENSOR_DEVICE_ATTR(port_52_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 19);
static SENSOR_DEVICE_ATTR(port_53_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 20);
static SENSOR_DEVICE_ATTR(port_54_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 21);
static SENSOR_DEVICE_ATTR(port_55_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 22);
static SENSOR_DEVICE_ATTR(port_56_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 23);
static SENSOR_DEVICE_ATTR(port_57_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 24);
static SENSOR_DEVICE_ATTR(port_58_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 25);
static SENSOR_DEVICE_ATTR(port_59_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 26);
static SENSOR_DEVICE_ATTR(port_60_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 27);
static SENSOR_DEVICE_ATTR(port_61_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 28);
static SENSOR_DEVICE_ATTR(port_62_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 29);
static SENSOR_DEVICE_ATTR(port_63_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 30);
static SENSOR_DEVICE_ATTR(port_64_loopb, S_IRUGO | S_IWUSR, show_osfp_loopb, set_osfp_loopb, 31);

static struct attribute *port_cpld1_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,

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
    &sensor_dev_attr_port_49_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_50_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_51_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_52_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_53_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_54_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_55_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_56_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_57_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_58_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_59_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_60_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_61_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_62_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_63_lpmod.dev_attr.attr,
    &sensor_dev_attr_port_64_lpmod.dev_attr.attr,

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
    &sensor_dev_attr_port_49_rst.dev_attr.attr,
    &sensor_dev_attr_port_50_rst.dev_attr.attr,
    &sensor_dev_attr_port_51_rst.dev_attr.attr,
    &sensor_dev_attr_port_52_rst.dev_attr.attr,
    &sensor_dev_attr_port_53_rst.dev_attr.attr,
    &sensor_dev_attr_port_54_rst.dev_attr.attr,
    &sensor_dev_attr_port_55_rst.dev_attr.attr,
    &sensor_dev_attr_port_56_rst.dev_attr.attr,
    &sensor_dev_attr_port_57_rst.dev_attr.attr,
    &sensor_dev_attr_port_58_rst.dev_attr.attr,
    &sensor_dev_attr_port_59_rst.dev_attr.attr,
    &sensor_dev_attr_port_60_rst.dev_attr.attr,
    &sensor_dev_attr_port_61_rst.dev_attr.attr,
    &sensor_dev_attr_port_62_rst.dev_attr.attr,
    &sensor_dev_attr_port_63_rst.dev_attr.attr,
    &sensor_dev_attr_port_64_rst.dev_attr.attr,

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
    &sensor_dev_attr_port_49_prs.dev_attr.attr,
    &sensor_dev_attr_port_50_prs.dev_attr.attr,
    &sensor_dev_attr_port_51_prs.dev_attr.attr,
    &sensor_dev_attr_port_52_prs.dev_attr.attr,
    &sensor_dev_attr_port_53_prs.dev_attr.attr,
    &sensor_dev_attr_port_54_prs.dev_attr.attr,
    &sensor_dev_attr_port_55_prs.dev_attr.attr,
    &sensor_dev_attr_port_56_prs.dev_attr.attr,
    &sensor_dev_attr_port_57_prs.dev_attr.attr,
    &sensor_dev_attr_port_58_prs.dev_attr.attr,
    &sensor_dev_attr_port_59_prs.dev_attr.attr,
    &sensor_dev_attr_port_60_prs.dev_attr.attr,
    &sensor_dev_attr_port_61_prs.dev_attr.attr,
    &sensor_dev_attr_port_62_prs.dev_attr.attr,
    &sensor_dev_attr_port_63_prs.dev_attr.attr,
    &sensor_dev_attr_port_64_prs.dev_attr.attr,

    &sensor_dev_attr_modprs_reg1.dev_attr.attr,
    &sensor_dev_attr_modprs_reg2.dev_attr.attr,
    &sensor_dev_attr_modprs_reg3.dev_attr.attr,
    &sensor_dev_attr_modprs_reg4.dev_attr.attr,

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
    &sensor_dev_attr_port_49_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_50_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_51_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_52_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_53_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_54_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_55_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_56_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_57_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_58_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_59_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_60_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_61_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_62_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_63_pwgood.dev_attr.attr,
    &sensor_dev_attr_port_64_pwgood.dev_attr.attr,

    &sensor_dev_attr_port_17_en.dev_attr.attr,
    &sensor_dev_attr_port_18_en.dev_attr.attr,
    &sensor_dev_attr_port_19_en.dev_attr.attr,
    &sensor_dev_attr_port_20_en.dev_attr.attr,
    &sensor_dev_attr_port_21_en.dev_attr.attr,
    &sensor_dev_attr_port_22_en.dev_attr.attr,
    &sensor_dev_attr_port_23_en.dev_attr.attr,
    &sensor_dev_attr_port_24_en.dev_attr.attr,
    &sensor_dev_attr_port_25_en.dev_attr.attr,
    &sensor_dev_attr_port_26_en.dev_attr.attr,
    &sensor_dev_attr_port_27_en.dev_attr.attr,
    &sensor_dev_attr_port_28_en.dev_attr.attr,
    &sensor_dev_attr_port_29_en.dev_attr.attr,
    &sensor_dev_attr_port_30_en.dev_attr.attr,
    &sensor_dev_attr_port_31_en.dev_attr.attr,
    &sensor_dev_attr_port_32_en.dev_attr.attr,
    &sensor_dev_attr_port_49_en.dev_attr.attr,
    &sensor_dev_attr_port_50_en.dev_attr.attr,
    &sensor_dev_attr_port_51_en.dev_attr.attr,
    &sensor_dev_attr_port_52_en.dev_attr.attr,
    &sensor_dev_attr_port_53_en.dev_attr.attr,
    &sensor_dev_attr_port_54_en.dev_attr.attr,
    &sensor_dev_attr_port_55_en.dev_attr.attr,
    &sensor_dev_attr_port_56_en.dev_attr.attr,
    &sensor_dev_attr_port_57_en.dev_attr.attr,
    &sensor_dev_attr_port_58_en.dev_attr.attr,
    &sensor_dev_attr_port_59_en.dev_attr.attr,
    &sensor_dev_attr_port_60_en.dev_attr.attr,
    &sensor_dev_attr_port_61_en.dev_attr.attr,
    &sensor_dev_attr_port_62_en.dev_attr.attr,
    &sensor_dev_attr_port_63_en.dev_attr.attr,
    &sensor_dev_attr_port_64_en.dev_attr.attr,

    &sensor_dev_attr_port_17_loopb.dev_attr.attr,
    &sensor_dev_attr_port_18_loopb.dev_attr.attr,
    &sensor_dev_attr_port_19_loopb.dev_attr.attr,
    &sensor_dev_attr_port_20_loopb.dev_attr.attr,
    &sensor_dev_attr_port_21_loopb.dev_attr.attr,
    &sensor_dev_attr_port_22_loopb.dev_attr.attr,
    &sensor_dev_attr_port_23_loopb.dev_attr.attr,
    &sensor_dev_attr_port_24_loopb.dev_attr.attr,
    &sensor_dev_attr_port_25_loopb.dev_attr.attr,
    &sensor_dev_attr_port_26_loopb.dev_attr.attr,
    &sensor_dev_attr_port_27_loopb.dev_attr.attr,
    &sensor_dev_attr_port_28_loopb.dev_attr.attr,
    &sensor_dev_attr_port_29_loopb.dev_attr.attr,
    &sensor_dev_attr_port_30_loopb.dev_attr.attr,
    &sensor_dev_attr_port_31_loopb.dev_attr.attr,
    &sensor_dev_attr_port_32_loopb.dev_attr.attr,
    &sensor_dev_attr_port_49_loopb.dev_attr.attr,
    &sensor_dev_attr_port_50_loopb.dev_attr.attr,
    &sensor_dev_attr_port_51_loopb.dev_attr.attr,
    &sensor_dev_attr_port_52_loopb.dev_attr.attr,
    &sensor_dev_attr_port_53_loopb.dev_attr.attr,
    &sensor_dev_attr_port_54_loopb.dev_attr.attr,
    &sensor_dev_attr_port_55_loopb.dev_attr.attr,
    &sensor_dev_attr_port_56_loopb.dev_attr.attr,
    &sensor_dev_attr_port_57_loopb.dev_attr.attr,
    &sensor_dev_attr_port_58_loopb.dev_attr.attr,
    &sensor_dev_attr_port_59_loopb.dev_attr.attr,
    &sensor_dev_attr_port_60_loopb.dev_attr.attr,
    &sensor_dev_attr_port_61_loopb.dev_attr.attr,
    &sensor_dev_attr_port_62_loopb.dev_attr.attr,
    &sensor_dev_attr_port_63_loopb.dev_attr.attr,
    &sensor_dev_attr_port_64_loopb.dev_attr.attr,

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
        .compatible = "nokia,port_cpld1",
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
MODULE_DESCRIPTION("NOKIA H6-64 CPLD1 driver");
MODULE_LICENSE("GPL");

module_init(port_cpld1_init);
module_exit(port_cpld1_exit);
