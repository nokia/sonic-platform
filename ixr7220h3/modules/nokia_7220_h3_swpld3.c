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

#define DRIVER_NAME "nokia_7220h3_swpld3"

// SWPLD2 & SWPLD3 REG ADDRESS-MAP
#define SWPLD23_REV_REG              0x01
#define SWPLD23_TEST_REG             0x0F

#define SWPLD23_QSFP17_24_RSTN_REG    0x11
#define SWPLD23_QSFP25_32_RSTN_REG    0x12
#define SWPLD23_QSFP17_24_INITMOD_REG 0x21
#define SWPLD23_QSFP25_32_INITMOD_REG 0x22
#define SWPLD23_QSFP17_24_MODSEL_REG  0x31
#define SWPLD23_QSFP25_32_MODSEL_REG  0x32
#define SWPLD23_QSFP17_24_MODPRS_REG  0x51
#define SWPLD23_QSFP25_32_MODPRS_REG  0x52
#define SWPLD23_QSFP17_24_INTN_REG    0x61
#define SWPLD23_QSFP25_32_INTN_REG    0x62
#define SWPLD23_SFP_REG1              0x71
#define SWPLD23_SFP_REG2              0x72

// REG BIT FIELD POSITION or MASK
#define SWPLD23_REV_REG_TYPE         0x07
#define SWPLD23_REV_REG_MSK          0x3F

// common index of each qsfp modules
#define QSFP17_INDEX                 0x7
#define QSFP18_INDEX                 0x6
#define QSFP19_INDEX                 0x5
#define QSFP20_INDEX                 0x4
#define QSFP21_INDEX                 0x3
#define QSFP22_INDEX                 0x2
#define QSFP23_INDEX                 0x1
#define QSFP24_INDEX                 0x0
#define QSFP25_INDEX                 0x7
#define QSFP26_INDEX                 0x6
#define QSFP27_INDEX                 0x5
#define QSFP28_INDEX                 0x4
#define QSFP29_INDEX                 0x3
#define QSFP30_INDEX                 0x2
#define QSFP31_INDEX                 0x1
#define QSFP32_INDEX                 0x0

#define SWPLD23_SFP_REG1_P0_PRS      0x6
#define SWPLD23_SFP_REG1_P0_RXLOS    0x5
#define SWPLD23_SFP_REG1_P0_TXFAULT  0x4
#define SWPLD23_SFP_REG1_P1_PRS      0x2
#define SWPLD23_SFP_REG1_P1_RXLOS    0x1
#define SWPLD23_SFP_REG1_P1_TXFAULT  0x0

#define SWPLD23_SFP_REG2_P0_TXDIS    0x7
#define SWPLD23_SFP_REG2_P1_TXDIS    0x3

static const unsigned short cpld_address_list[] = {0x35, I2C_CLIENT_END};

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

static ssize_t show_qsfp_g3_rstn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_RSTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g3_rstn(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_RSTN_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP17_24_RSTN_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g4_rstn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_RSTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g4_rstn(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_RSTN_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP25_32_RSTN_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g3_lpmod(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_INITMOD_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g3_lpmod(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_INITMOD_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP17_24_INITMOD_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g4_lpmod(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_INITMOD_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g4_lpmod(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_INITMOD_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP25_32_INITMOD_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g3_modseln(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_MODSEL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g3_modseln(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_MODSEL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP17_24_MODSEL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g4_modseln(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_MODSEL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_g4_modseln(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_MODSEL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_QSFP25_32_MODSEL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_g3_prs(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_MODPRS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_g4_prs(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_MODPRS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_g3_intn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP17_24_INTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_g4_intn(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_QSFP25_32_INTN_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_reg1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_SFP_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_reg2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD23_SFP_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_sfp_reg2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD23_SFP_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD23_SFP_REG2, (reg_val | usr_val));

    return count;
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_cpld_version, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_type, S_IRUGO, show_cpld_type, NULL, SWPLD23_REV_REG_TYPE);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);

static SENSOR_DEVICE_ATTR(qsfp17_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_rstn, S_IRUGO | S_IWUSR, show_qsfp_g3_rstn, set_qsfp_g3_rstn, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_rstn, S_IRUGO | S_IWUSR, show_qsfp_g4_rstn, set_qsfp_g4_rstn, QSFP32_INDEX);

static SENSOR_DEVICE_ATTR(qsfp17_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g3_lpmod, set_qsfp_g3_lpmod, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_lpmod, S_IRUGO | S_IWUSR, show_qsfp_g4_lpmod, set_qsfp_g4_lpmod, QSFP32_INDEX);

static SENSOR_DEVICE_ATTR(qsfp17_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_modseln, S_IRUGO | S_IWUSR, show_qsfp_g3_modseln, set_qsfp_g3_modseln, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_modseln, S_IRUGO | S_IWUSR, show_qsfp_g4_modseln, set_qsfp_g4_modseln, QSFP32_INDEX);

static SENSOR_DEVICE_ATTR(qsfp17_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_prs, S_IRUGO, show_qsfp_g3_prs, NULL, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_prs, S_IRUGO, show_qsfp_g4_prs, NULL, QSFP32_INDEX);

static SENSOR_DEVICE_ATTR(qsfp17_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_intn, S_IRUGO, show_qsfp_g3_intn, NULL, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_intn, S_IRUGO, show_qsfp_g4_intn, NULL, QSFP32_INDEX);

static SENSOR_DEVICE_ATTR(sfp0_prs,     S_IRUGO, show_sfp_reg1, NULL, SWPLD23_SFP_REG1_P0_PRS);
static SENSOR_DEVICE_ATTR(sfp0_rxlos,   S_IRUGO, show_sfp_reg1, NULL, SWPLD23_SFP_REG1_P0_RXLOS);
static SENSOR_DEVICE_ATTR(sfp0_txfault, S_IRUGO, show_sfp_reg1, NULL, SWPLD23_SFP_REG1_P0_TXFAULT);
static SENSOR_DEVICE_ATTR(sfp1_prs,     S_IRUGO, show_sfp_reg1, NULL, SWPLD23_SFP_REG1_P1_PRS);
static SENSOR_DEVICE_ATTR(sfp1_rxlos,   S_IRUGO, show_sfp_reg1, NULL, SWPLD23_SFP_REG1_P1_RXLOS);
static SENSOR_DEVICE_ATTR(sfp1_txfault, S_IRUGO, show_sfp_reg1, NULL, SWPLD23_SFP_REG1_P1_TXFAULT);

static SENSOR_DEVICE_ATTR(sfp0_txdis, S_IRUGO | S_IWUSR, show_sfp_reg2, set_sfp_reg2, SWPLD23_SFP_REG2_P0_TXDIS);
static SENSOR_DEVICE_ATTR(sfp1_txdis, S_IRUGO | S_IWUSR, show_sfp_reg2, set_sfp_reg2, SWPLD23_SFP_REG2_P1_TXDIS);

static struct attribute *nokia_7220_h3_swpld3_attributes[] = {
    &sensor_dev_attr_cpld_version.dev_attr.attr,
    &sensor_dev_attr_cpld_type.dev_attr.attr,    
    &sensor_dev_attr_scratch.dev_attr.attr,
    
    &sensor_dev_attr_qsfp17_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp18_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp19_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp20_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp21_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp22_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp23_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp24_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp25_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp26_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp27_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp28_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp29_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp30_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp31_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp32_rstn.dev_attr.attr,

    &sensor_dev_attr_qsfp17_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp18_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp19_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp20_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp21_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp22_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp23_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp24_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp25_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp26_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp27_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp28_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp29_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp30_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp31_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp32_lpmod.dev_attr.attr,

    &sensor_dev_attr_qsfp17_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp18_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp19_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp20_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp21_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp22_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp23_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp24_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp25_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp26_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp27_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp28_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp29_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp30_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp31_modseln.dev_attr.attr,
    &sensor_dev_attr_qsfp32_modseln.dev_attr.attr,

    &sensor_dev_attr_qsfp17_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp18_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp19_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp20_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp21_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp22_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp23_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp24_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp25_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp26_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp27_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp28_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp29_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp30_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp31_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp32_prs.dev_attr.attr,

    &sensor_dev_attr_qsfp17_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp18_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp19_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp20_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp21_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp22_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp23_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp24_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp25_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp26_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp27_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp28_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp29_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp30_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp31_intn.dev_attr.attr,
    &sensor_dev_attr_qsfp32_intn.dev_attr.attr,

    &sensor_dev_attr_sfp0_prs.dev_attr.attr,
    &sensor_dev_attr_sfp0_rxlos.dev_attr.attr,
    &sensor_dev_attr_sfp0_txfault.dev_attr.attr,
    &sensor_dev_attr_sfp1_prs.dev_attr.attr,
    &sensor_dev_attr_sfp1_rxlos.dev_attr.attr,
    &sensor_dev_attr_sfp1_txfault.dev_attr.attr,

    &sensor_dev_attr_sfp0_txdis.dev_attr.attr,
    &sensor_dev_attr_sfp1_txdis.dev_attr.attr,
    NULL
};

static const struct attribute_group nokia_7220_h3_swpld3_group = {
    .attrs = nokia_7220_h3_swpld3_attributes,
};

static int nokia_7220_h3_swpld3_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H3 SWPLD3 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &nokia_7220_h3_swpld3_group);
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

static void nokia_7220_h3_swpld3_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &nokia_7220_h3_swpld3_group);
    kfree(data);
}

static const struct of_device_id nokia_7220_h3_swpld3_of_ids[] = {
    {
        .compatible = "nokia,7220_h3_swpld3",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, nokia_7220_h3_swpld3_of_ids);

static const struct i2c_device_id nokia_7220_h3_swpld3_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, nokia_7220_h3_swpld3_ids);

static struct i2c_driver nokia_7220_h3_swpld3_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(nokia_7220_h3_swpld3_of_ids),
    },
    .probe        = nokia_7220_h3_swpld3_probe,
    .remove       = nokia_7220_h3_swpld3_remove,
    .id_table     = nokia_7220_h3_swpld3_ids,
    .address_list = cpld_address_list,
};

static int __init nokia_7220_h3_swpld3_init(void)
{
    return i2c_add_driver(&nokia_7220_h3_swpld3_driver);
}

static void __exit nokia_7220_h3_swpld3_exit(void)
{
    i2c_del_driver(&nokia_7220_h3_swpld3_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H3 CPLD driver");
MODULE_LICENSE("GPL");

module_init(nokia_7220_h3_swpld3_init);
module_exit(nokia_7220_h3_swpld3_exit);
