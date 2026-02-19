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

#define DRIVER_NAME "port_cpld2"

// REGISTERS ADDRESS MAP
#define VER_MAJOR_REG           0x00
#define VER_MINOR_REG           0x01
#define SCRATCH_REG             0x04
#define PORT_LPMODE_REG0        0x70
#define PORT_EFUSE_REG0         0x72
#define PORT_RST_REG0           0x78
#define PORT_MODPRS_REG0        0x88
#define PORT_PWGOOD_REG0        0x90
#define PORT_ENABLE_REG0        0x98

static const unsigned short cpld_address_list[] = {0x73, 0x76, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int port_efuse;
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

    val0 = cpld_i2c_read(data, PORT_RST_REG0);
    val1 = cpld_i2c_read(data, PORT_RST_REG0 + 1);
    dev_info(&client->dev, "[PORT_CPLD2]PORT_RESET_REG: 0x%02x, 0x%02x\n", val0, val1);

    val0 = cpld_i2c_read(data, PORT_LPMODE_REG0);
    val1 = cpld_i2c_read(data, PORT_LPMODE_REG0 + 1);
    dev_info(&client->dev, "[PORT_CPLD2]PORT_LPMODE_REG: 0x%02x, 0x%02x\n", val0, val1);

    val0 = cpld_i2c_read(data, PORT_MODPRS_REG0);
    val1 = cpld_i2c_read(data, PORT_MODPRS_REG0 + 1);
    dev_info(&client->dev, "[PORT_CPLD2]PORT_MODPRES_REG: 0x%02x, 0x%02x\n", val0, val1);

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

static ssize_t show_port_efuse(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    
    return sprintf(buf, "%s\n", (data->port_efuse) ? "Enabled":"Disabled");
}

static ssize_t set_port_efuse(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    const char *str_en = "Enable\n";
    const char *str_dis = "Disable\n";
    
    if (strcmp(buf, str_en) == 0) {
        cpld_i2c_write(data, PORT_EFUSE_REG0, 0xFF);
        cpld_i2c_write(data, PORT_EFUSE_REG0+1, 0xFF);
        data->port_efuse = 1;
    }
    else if (strcmp(buf, str_dis) == 0) {
        cpld_i2c_write(data, PORT_EFUSE_REG0, 0x0);
        cpld_i2c_write(data, PORT_EFUSE_REG0+1, 0x0);
        data->port_efuse = 0;
    }
    else
        return -EINVAL;

    return count;
}

static ssize_t show_port_en(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    return sprintf(buf, "na\n");
    val = cpld_i2c_read(data, PORT_ENABLE_REG0 + (sda->index / 8));

    return sprintf(buf, "%d\n", (val>>(sda->index % 8)) & 0x1 ? 1:0);
}

static ssize_t set_port_en(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    return 0;
    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(1 << (sda->index % 8))) & 0xFF;
    reg_val = cpld_i2c_read(data, PORT_ENABLE_REG0 + (sda->index / 8));
    reg_val = reg_val & mask;
    usr_val = usr_val << (sda->index % 8);
    cpld_i2c_write(data, PORT_ENABLE_REG0 + (sda->index / 8), (reg_val | usr_val));

    return count;
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);

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

static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 0);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 1);

static SENSOR_DEVICE_ATTR(port_1_en, S_IRUGO, show_port_en, set_port_en, 0);
static SENSOR_DEVICE_ATTR(port_2_en, S_IRUGO, show_port_en, set_port_en, 1);
static SENSOR_DEVICE_ATTR(port_3_en, S_IRUGO, show_port_en, set_port_en, 2);
static SENSOR_DEVICE_ATTR(port_4_en, S_IRUGO, show_port_en, set_port_en, 3);
static SENSOR_DEVICE_ATTR(port_5_en, S_IRUGO, show_port_en, set_port_en, 4);
static SENSOR_DEVICE_ATTR(port_6_en, S_IRUGO, show_port_en, set_port_en, 5);
static SENSOR_DEVICE_ATTR(port_7_en, S_IRUGO, show_port_en, set_port_en, 6);
static SENSOR_DEVICE_ATTR(port_8_en, S_IRUGO, show_port_en, set_port_en, 7);
static SENSOR_DEVICE_ATTR(port_9_en, S_IRUGO, show_port_en, set_port_en, 8);
static SENSOR_DEVICE_ATTR(port_10_en, S_IRUGO, show_port_en, set_port_en, 9);
static SENSOR_DEVICE_ATTR(port_11_en, S_IRUGO, show_port_en, set_port_en, 10);
static SENSOR_DEVICE_ATTR(port_12_en, S_IRUGO, show_port_en, set_port_en, 11);
static SENSOR_DEVICE_ATTR(port_13_en, S_IRUGO, show_port_en, set_port_en, 12);
static SENSOR_DEVICE_ATTR(port_14_en, S_IRUGO, show_port_en, set_port_en, 13);
static SENSOR_DEVICE_ATTR(port_15_en, S_IRUGO, show_port_en, set_port_en, 14);
static SENSOR_DEVICE_ATTR(port_16_en, S_IRUGO, show_port_en, set_port_en, 15);

static SENSOR_DEVICE_ATTR(port_efuse, S_IRUGO | S_IWUSR, show_port_efuse, set_port_efuse, 0);

static struct attribute *port_cpld2_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,

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

    &sensor_dev_attr_modprs_reg1.dev_attr.attr,
    &sensor_dev_attr_modprs_reg2.dev_attr.attr,

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

    &sensor_dev_attr_port_efuse.dev_attr.attr,

    NULL
};

static const struct attribute_group port_cpld2_group = {
    .attrs = port_cpld2_attributes,
};

static int port_cpld2_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia PORT_CPLD2 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &port_cpld2_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }
    
    dump_reg(data);
    cpld_i2c_write(data, PORT_EFUSE_REG0, 0xFF);
    cpld_i2c_write(data, PORT_EFUSE_REG0+1, 0xFF);
    dev_info(&client->dev, "[PORT_CPLD2]Reseting PORTs ...\n");
    cpld_i2c_write(data, PORT_LPMODE_REG0, 0x0);
    cpld_i2c_write(data, PORT_LPMODE_REG0+1, 0x0);
    cpld_i2c_write(data, PORT_RST_REG0, 0x0);
    cpld_i2c_write(data, PORT_RST_REG0+1, 0x0);
    msleep(500);
    cpld_i2c_write(data, PORT_RST_REG0, 0xFF);
    cpld_i2c_write(data, PORT_RST_REG0+1, 0xFF);
    dev_info(&client->dev, "[PORT_CPLD2]PORTs reset done.\n");
    dump_reg(data);
    
    return 0;

exit:
    return status;
}

static void port_cpld2_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &port_cpld2_group);
    kfree(data);
}

static const struct of_device_id port_cpld2_of_ids[] = {
    {
        .compatible = "port_cpld2",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, port_cpld2_of_ids);

static const struct i2c_device_id port_cpld2_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, port_cpld2_ids);

static struct i2c_driver port_cpld2_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(port_cpld2_of_ids),
    },
    .probe        = port_cpld2_probe,
    .remove       = port_cpld2_remove,
    .id_table     = port_cpld2_ids,
    .address_list = cpld_address_list,
};

static int __init port_cpld2_init(void)
{
    return i2c_add_driver(&port_cpld2_driver);
}

static void __exit port_cpld2_exit(void)
{
    i2c_del_driver(&port_cpld2_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA H6-128 PORT_CPLD2 driver");
MODULE_LICENSE("GPL");

module_init(port_cpld2_init);
module_exit(port_cpld2_exit);
