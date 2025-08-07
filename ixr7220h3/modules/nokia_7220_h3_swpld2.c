//  * CPLD driver for Nokia-7220-IXR-H3 Router
//  *
//  * Copyright (C) 2024 Nokia Corporation.
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

#define DRIVER_NAME "nokia_7220h3_swpld2"

// SWPLD2 & SWPLD3 REG ADDRESS-MAP
#define SWPLD23_REV_REG               0x01
#define SWPLD23_TEST_REG              0x0F

#define SWPLD23_QSFP01_08_RSTN_REG    0x11
#define SWPLD23_QSFP09_16_RSTN_REG    0x12
#define SWPLD23_QSFP01_08_INITMOD_REG 0x21
#define SWPLD23_QSFP09_16_INITMOD_REG 0x22
#define SWPLD23_QSFP01_08_MODSEL_REG  0x31
#define SWPLD23_QSFP09_16_MODSEL_REG  0x32
#define SWPLD23_QSFP01_08_MODPRS_REG  0x51
#define SWPLD23_QSFP09_16_MODPRS_REG  0x52
#define SWPLD23_QSFP01_08_INTN_REG    0x61
#define SWPLD23_QSFP09_16_INTN_REG    0x62

// REG BIT FIELD POSITION or MASK
#define SWPLD23_REV_REG_TYPE         0x07
#define SWPLD23_REV_REG_MSK          0x3F

// common index of each qsfp modules
#define QSFP01_INDEX                 0x7
#define QSFP02_INDEX                 0x6
#define QSFP03_INDEX                 0x5
#define QSFP04_INDEX                 0x4
#define QSFP05_INDEX                 0x3
#define QSFP06_INDEX                 0x2
#define QSFP07_INDEX                 0x1
#define QSFP08_INDEX                 0x0
#define QSFP09_INDEX                 0x7
#define QSFP10_INDEX                 0x6
#define QSFP11_INDEX                 0x5
#define QSFP12_INDEX                 0x4
#define QSFP13_INDEX                 0x3
#define QSFP14_INDEX                 0x2
#define QSFP15_INDEX                 0x1
#define QSFP16_INDEX                 0x0


static const unsigned short cpld_address_list[] = {0x34, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int cpld_version;
    int cpld_type;
};

static int nokia_7220_h3_swpld_read(struct cpld_data *data, u8 reg)
{
    int val = 0;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
         dev_err(&client->dev, "CPLD READ ERROR: reg(0x%02x) err %d\n", reg, val);
    }
    mutex_unlock(&data->update_lock);

    return val;
}

static void nokia_7220_h3_swpld_write(struct cpld_data *data, u8 reg, u8 value)
{
    int res = 0;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    res = i2c_smbus_write_byte_data(client, reg, value);
    if (res < 0) {
        dev_err(&client->dev, "CPLD WRITE ERROR: reg(0x%02x) err %d\n", reg, res);
    }
    mutex_unlock(&data->update_lock);
}

static ssize_t show_cpld_version(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "0x%02x\n", data->cpld_version);
}

static ssize_t show_cpld_type(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *cpld_type = NULL;
    
    switch (data->cpld_type) {
    case 0:
        cpld_type = "Official type";
        break;
    case 1:
        cpld_type = "Test type";
        break;
    default:
        cpld_type = "Unknown";
        break;
    }

    return sprintf(buf, "0x%02x %s\n", data->cpld_type, cpld_type);
}

static ssize_t show_scratch(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_TEST_REG);

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

    nokia_7220_h3_swpld_write(data, SWPLD23_TEST_REG, usr_val);

    return count;
}

static ssize_t show_qsfp_g1_rstn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_RSTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g1_rstn(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_RSTN_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP01_08_RSTN_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g2_rstn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_RSTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g2_rstn(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_RSTN_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP09_16_RSTN_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g1_lpmod(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_INITMOD_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g1_lpmod(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_INITMOD_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP01_08_INITMOD_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g2_lpmod(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_INITMOD_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g2_lpmod(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_INITMOD_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP09_16_INITMOD_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g1_modseln(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_MODSEL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g1_modseln(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_MODSEL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP01_08_MODSEL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g2_modseln(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_MODSEL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g2_modseln(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_MODSEL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP09_16_MODSEL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g1_prs(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_MODPRS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_g2_prs(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_MODPRS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_g1_intn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP01_08_INTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_g2_intn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP09_16_INTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_cpld_version, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_type, S_IRUGO, show_cpld_type, NULL, SWPLD23_REV_REG_TYPE);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);

static SENSOR_DEVICE_ATTR(qsfp1_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_rstn, S_IRUGO | S_IWUSR, show_qsfp_g1_rstn, set_qsfp_g1_rstn, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_rstn, S_IRUGO | S_IWUSR, show_qsfp_g2_rstn, set_qsfp_g2_rstn, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(qsfp1_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g1_lpmod, set_qsfp_g1_lpmod, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g2_lpmod, set_qsfp_g2_lpmod, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(qsfp1_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_modseln, S_IRUGO | S_IWUSR, show_qsfp_g1_modseln, set_qsfp_g1_modseln, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_modseln, S_IRUGO | S_IWUSR, show_qsfp_g2_modseln, set_qsfp_g2_modseln, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(qsfp1_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_prs, S_IRUGO, show_qsfp_g1_prs, NULL, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_prs, S_IRUGO, show_qsfp_g2_prs, NULL, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(qsfp1_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_intn, S_IRUGO, show_qsfp_g1_intn, NULL, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_intn, S_IRUGO, show_qsfp_g2_intn, NULL, QSFP16_INDEX);

static struct attribute *nokia_7220_h3_swpld2_attributes[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_type.dev_attr.attr,    
    &sensor_dev_attr_scratch.dev_attr.attr,

    &sensor_dev_attr_qsfp1_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp2_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp3_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp4_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp5_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp6_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp7_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp8_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp9_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp10_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp11_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp12_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp13_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp14_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp15_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp16_rstn.dev_attr.attr,

    &sensor_dev_attr_qsfp1_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp2_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp3_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp4_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp5_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp6_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp7_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp8_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp9_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp10_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp11_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp12_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp13_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp14_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp15_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp16_lpmod.dev_attr.attr,

    &sensor_dev_attr_qsfp1_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp2_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp3_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp4_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp5_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp6_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp7_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp8_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp9_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp10_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp11_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp12_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp13_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp14_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp15_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp16_modseln.dev_attr.attr,

    &sensor_dev_attr_qsfp1_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp2_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp3_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp4_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp5_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp6_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp7_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp8_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp9_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp10_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp11_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp12_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp13_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp14_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp15_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp16_prs.dev_attr.attr,

    &sensor_dev_attr_qsfp1_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp2_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp3_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp4_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp5_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp6_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp7_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp8_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp9_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp10_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp11_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp12_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp13_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp14_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp15_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp16_intn.dev_attr.attr,   
    NULL
};

static const struct attribute_group nokia_7220_h3_swpld2_group = {
    .attrs = nokia_7220_h3_swpld2_attributes,
};

static int nokia_7220_h3_swpld2_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H3 SWPLD2 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &nokia_7220_h3_swpld2_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->cpld_version = nokia_7220_h3_swpld_read(data, SWPLD23_REV_REG) & SWPLD23_REV_REG_MSK;
    data->cpld_type = nokia_7220_h3_swpld_read(data, SWPLD23_REV_REG) >> SWPLD23_REV_REG_TYPE;  
    
    return 0;

exit:
    return status;
}

static void nokia_7220_h3_swpld2_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &nokia_7220_h3_swpld2_group);
    kfree(data);
}

static const struct of_device_id nokia_7220_h3_swpld2_of_ids[] = {
    {
        .compatible = "nokia,7220_h3_swpld2",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, nokia_7220_h3_swpld2_of_ids);

static const struct i2c_device_id nokia_7220_h3_swpld2_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, nokia_7220_h3_swpld2_ids);

static struct i2c_driver nokia_7220_h3_swpld2_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(nokia_7220_h3_swpld2_of_ids),
    },
    .probe        = nokia_7220_h3_swpld2_probe,
    .remove       = nokia_7220_h3_swpld2_remove,
    .id_table     = nokia_7220_h3_swpld2_ids,
    .address_list = cpld_address_list,
};

static int __init nokia_7220_h3_swpld2_init(void)
{
    return i2c_add_driver(&nokia_7220_h3_swpld2_driver);
}

static void __exit nokia_7220_h3_swpld2_exit(void)
{
    i2c_del_driver(&nokia_7220_h3_swpld2_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H3 CPLD driver");
MODULE_LICENSE("GPL");

module_init(nokia_7220_h3_swpld2_init);
module_exit(nokia_7220_h3_swpld2_exit);
