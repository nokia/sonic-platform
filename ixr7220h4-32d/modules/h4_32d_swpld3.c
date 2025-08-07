//  * CPLD driver for Nokia-7220-IXR-H4-32D
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
#include <linux/delay.h>

#define DRIVER_NAME "h4_32d_swpld3"

// REGISTERS ADDRESS MAP
#define CODE_REV_REG            0x01
#define LED_TEST_REG            0x08
#define SCRATCH_REG             0x0F
#define RST_REG                 0x10
#define QSFP_RST_REG0           0x11
#define QSFP_RST_REG1           0x12
#define QSFP_INITMOD_REG0       0x21
#define QSFP_INITMOD_REG1       0x22
#define QSFP_MODSEL_REG0        0x31
#define QSFP_MODSEL_REG1        0x32
#define HITLESS_REG             0x39
#define QSFP_MODPRS_REG0        0x51
#define QSFP_MODPRS_REG1        0x52
#define QSFP_INT_REG0           0x61
#define QSFP_INT_REG1           0x62
#define SFP_REG0                0x71
#define SFP_REG1                0x72
#define QSFP_LED_REG1           0x90
#define CODE_DAY_REG            0xF0
#define CODE_MONTH_REG          0xF1
#define CODE_YEAR_REG           0xF2
#define TEST_CODE_REV_REG       0xF3

// REG BIT FIELD POSITION or MASK
#define LED_TEST_REG_AMB        0x0
#define LED_TEST_REG_GRN        0x1
#define LED_TEST_REG_BLINK      0x3
#define LED_TEST_REG_SRC_SEL    0x7

#define RST_REG_PLD_SOFT_RST    0x0

#define HITLESS_REG_EN          0x0

#define SFP_REG0_TX_FAULT       0x4
#define SFP_REG0_RX_LOS         0x5
#define SFP_REG0_PRS            0x6

#define SFP_REG1_LED            0x4
#define SFP_REG1_TX_EN          0x7

// common index of each qsfp modules
#define QSFP17_INDEX            0x7
#define QSFP18_INDEX            0x6
#define QSFP19_INDEX            0x5
#define QSFP20_INDEX            0x4
#define QSFP21_INDEX            0x3
#define QSFP22_INDEX            0x2
#define QSFP23_INDEX            0x1
#define QSFP24_INDEX            0x0
#define QSFP25_INDEX            0x7
#define QSFP26_INDEX            0x6
#define QSFP27_INDEX            0x5
#define QSFP28_INDEX            0x4
#define QSFP29_INDEX            0x3
#define QSFP30_INDEX            0x2
#define QSFP31_INDEX            0x1
#define QSFP32_INDEX            0x0

static const unsigned short cpld_address_list[] = {0x35, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int reset_list[16];
};

static int cpld_i2c_read(struct cpld_data *data, u8 reg)
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

static void cpld_i2c_write(struct cpld_data *data, u8 reg, u8 value)
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

static void dump_reg(struct cpld_data *data) 
{    
    struct i2c_client *client = data->client;
    u8 val0 = 0;
    u8 val1 = 0;
      
    val0 = cpld_i2c_read(data, QSFP_RST_REG0);
    val1 = cpld_i2c_read(data, QSFP_RST_REG1);
    dev_info(&client->dev, "[SWPLD3]QSFP_RESET_REG: 0x%02x, 0x%02x\n", val0, val1);

    val0 = cpld_i2c_read(data, QSFP_INITMOD_REG0);
    val1 = cpld_i2c_read(data, QSFP_INITMOD_REG1);
    dev_info(&client->dev, "[SWPLD3]QSFP_LPMODE_REG: 0x%02x, 0x%02x\n", val0, val1);
    
    val0 = cpld_i2c_read(data, QSFP_MODSEL_REG0);
    val1 = cpld_i2c_read(data, QSFP_MODSEL_REG1);
    dev_info(&client->dev, "[SWPLD3]QSFP_MODSEL_REG: 0x%02x, 0x%02x\n", val0, val1);
    
    val0 = cpld_i2c_read(data, QSFP_MODPRS_REG0);    
    val1 = cpld_i2c_read(data, QSFP_MODPRS_REG1);
    dev_info(&client->dev, "[SWPLD3]QSFP_MODPRES_REG: 0x%02x, 0x%02x\n", val0, val1);

}

static ssize_t show_code_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_REV_REG);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_led_test(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, LED_TEST_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_led_test(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, LED_TEST_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, LED_TEST_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_scratch(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, SCRATCH_REG);

    return sprintf(buf, "%02x\n", val);
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

static ssize_t show_rst(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, RST_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_rst(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, RST_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, RST_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_rst0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_RST_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_rst0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_RST_REG0);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_RST_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_rst1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_RST_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_rst1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_RST_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_RST_REG1, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_initmod0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_INITMOD_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_initmod0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_INITMOD_REG0);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_INITMOD_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_initmod1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_INITMOD_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_initmod1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_INITMOD_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_INITMOD_REG1, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_modsel0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODSEL_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_modsel0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_MODSEL_REG0);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_MODSEL_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_modsel1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODSEL_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_modsel1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_MODSEL_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_MODSEL_REG1, (reg_val | usr_val));

    return count;
}

static ssize_t show_hitless(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, HITLESS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_modprs0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_modprs1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_modprs_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    if (sda->index == 1)
        val = cpld_i2c_read(data, QSFP_MODPRS_REG0);
    if (sda->index == 2)
        val = cpld_i2c_read(data, QSFP_MODPRS_REG1);

    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_qsfp_int0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_INT_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_int1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_INT_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_reg0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, SFP_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_sfp_reg1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 reg_val = 0;
      
    val = cpld_i2c_read(data, SFP_REG1);
    switch (sda->index) {
    case SFP_REG1_TX_EN:    
        return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
    case SFP_REG1_LED:
        reg_val = (val>>sda->index) & 0x3;
        return sprintf(buf, "%d\n", reg_val);  
    default:
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    }

}

static ssize_t set_sfp_reg1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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

    switch (sda->index) {
    case SFP_REG1_TX_EN:    
        if (usr_val > 1) {
            return -EINVAL;
        }
        mask = (~(1 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, SFP_REG1);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, SFP_REG1, (reg_val | usr_val));
        break;
    case SFP_REG1_LED:
        if (usr_val > 3) {
            return -EINVAL;
        }
        mask = (~(3 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, SFP_REG1);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, SFP_REG1, (reg_val | usr_val));
        break;
    default:
        sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    }

    return count;
}

static ssize_t show_code_day(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_DAY_REG);
    return sprintf(buf, "%d\n", val);
}

static ssize_t show_code_month(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_MONTH_REG);
    return sprintf(buf, "%d\n", val);
}

static ssize_t show_code_year(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_YEAR_REG);
    return sprintf(buf, "%d\n", val);
}

static ssize_t show_qsfp_reset(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);

    return sprintf(buf, "%d\n", data->reset_list[sda->index]);
}

static ssize_t set_qsfp_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 usr_val = 0;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 0xFF) {
        return -EINVAL;
    }

    data->reset_list[sda->index] = usr_val;
    return count;
}

static ssize_t show_qsfp_led(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_LED_REG1 + sda->index);

    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t set_qsfp_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 usr_val = 0;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 0xFF) {
        return -EINVAL;
    }

    cpld_i2c_write(data, QSFP_LED_REG1 + sda->index, usr_val);

    return count;
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(led_test_amb, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_AMB);
static SENSOR_DEVICE_ATTR(led_test_grn, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_GRN);
static SENSOR_DEVICE_ATTR(led_test_blink, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_BLINK);
static SENSOR_DEVICE_ATTR(led_test_src_sel, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_SRC_SEL);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);
static SENSOR_DEVICE_ATTR(rst_pld_soft, S_IRUGO | S_IWUSR, show_rst, set_rst, RST_REG_PLD_SOFT_RST);
static SENSOR_DEVICE_ATTR(qsfp17_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(qsfp17_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(qsfp17_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(hitless_en, S_IRUGO, show_hitless, NULL, HITLESS_REG_EN);
static SENSOR_DEVICE_ATTR(qsfp17_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 1);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 2);
static SENSOR_DEVICE_ATTR(qsfp17_int, S_IRUGO, show_qsfp_int0, NULL, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_int, S_IRUGO, show_qsfp_int0, NULL, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_int, S_IRUGO, show_qsfp_int0, NULL, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_int, S_IRUGO, show_qsfp_int0, NULL, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_int, S_IRUGO, show_qsfp_int0, NULL, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_int, S_IRUGO, show_qsfp_int0, NULL, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_int, S_IRUGO, show_qsfp_int0, NULL, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_int, S_IRUGO, show_qsfp_int0, NULL, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_int, S_IRUGO, show_qsfp_int1, NULL, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_int, S_IRUGO, show_qsfp_int1, NULL, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_int, S_IRUGO, show_qsfp_int1, NULL, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_int, S_IRUGO, show_qsfp_int1, NULL, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_int, S_IRUGO, show_qsfp_int1, NULL, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_int, S_IRUGO, show_qsfp_int1, NULL, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_int, S_IRUGO, show_qsfp_int1, NULL, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_int, S_IRUGO, show_qsfp_int1, NULL, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(sfp0_tx_fault, S_IRUGO, show_sfp_reg0, NULL, SFP_REG0_TX_FAULT);
static SENSOR_DEVICE_ATTR(sfp0_rx_los, S_IRUGO, show_sfp_reg0, NULL, SFP_REG0_RX_LOS);
static SENSOR_DEVICE_ATTR(sfp0_prs, S_IRUGO, show_sfp_reg0, NULL, SFP_REG0_PRS);
static SENSOR_DEVICE_ATTR(sfp0_tx_en, S_IRUGO | S_IWUSR, show_sfp_reg1, set_sfp_reg1, SFP_REG1_TX_EN);
static SENSOR_DEVICE_ATTR(sfp0_led, S_IRUGO | S_IWUSR, show_sfp_reg1, set_sfp_reg1, SFP_REG1_LED);
static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp17_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 0);
static SENSOR_DEVICE_ATTR(qsfp18_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 1);
static SENSOR_DEVICE_ATTR(qsfp19_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 2);
static SENSOR_DEVICE_ATTR(qsfp20_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 3);
static SENSOR_DEVICE_ATTR(qsfp21_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 4);
static SENSOR_DEVICE_ATTR(qsfp22_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 5);
static SENSOR_DEVICE_ATTR(qsfp23_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 6);
static SENSOR_DEVICE_ATTR(qsfp24_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 7);
static SENSOR_DEVICE_ATTR(qsfp25_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 8);
static SENSOR_DEVICE_ATTR(qsfp26_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 9);
static SENSOR_DEVICE_ATTR(qsfp27_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 10);
static SENSOR_DEVICE_ATTR(qsfp28_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 11);
static SENSOR_DEVICE_ATTR(qsfp29_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 12);
static SENSOR_DEVICE_ATTR(qsfp30_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 13);
static SENSOR_DEVICE_ATTR(qsfp31_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 14);
static SENSOR_DEVICE_ATTR(qsfp32_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 15);

static SENSOR_DEVICE_ATTR(qsfp17_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 0);
static SENSOR_DEVICE_ATTR(qsfp18_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 1);
static SENSOR_DEVICE_ATTR(qsfp19_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 2);
static SENSOR_DEVICE_ATTR(qsfp20_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 3);
static SENSOR_DEVICE_ATTR(qsfp21_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 4);
static SENSOR_DEVICE_ATTR(qsfp22_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 5);
static SENSOR_DEVICE_ATTR(qsfp23_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 6);
static SENSOR_DEVICE_ATTR(qsfp24_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 7);
static SENSOR_DEVICE_ATTR(qsfp25_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 8);
static SENSOR_DEVICE_ATTR(qsfp26_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 9);
static SENSOR_DEVICE_ATTR(qsfp27_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 10);
static SENSOR_DEVICE_ATTR(qsfp28_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 11);
static SENSOR_DEVICE_ATTR(qsfp29_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 12);
static SENSOR_DEVICE_ATTR(qsfp30_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 13);
static SENSOR_DEVICE_ATTR(qsfp31_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 14);
static SENSOR_DEVICE_ATTR(qsfp32_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 15);

static struct attribute *h4_32d_swpld3_attributes[] = {
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_led_test_amb.dev_attr.attr,
    &sensor_dev_attr_led_test_grn.dev_attr.attr,
    &sensor_dev_attr_led_test_blink.dev_attr.attr,
    &sensor_dev_attr_led_test_src_sel.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_rst_pld_soft.dev_attr.attr,    
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
    &sensor_dev_attr_qsfp17_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp18_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp19_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp20_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp21_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp22_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp23_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp24_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp25_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp26_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp27_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp28_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp29_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp30_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp31_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp32_modsel.dev_attr.attr,
    &sensor_dev_attr_hitless_en.dev_attr.attr,
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
    &sensor_dev_attr_modprs_reg1.dev_attr.attr,
    &sensor_dev_attr_modprs_reg2.dev_attr.attr,
    &sensor_dev_attr_qsfp17_int.dev_attr.attr,
    &sensor_dev_attr_qsfp18_int.dev_attr.attr,
    &sensor_dev_attr_qsfp19_int.dev_attr.attr,
    &sensor_dev_attr_qsfp20_int.dev_attr.attr,
    &sensor_dev_attr_qsfp21_int.dev_attr.attr,
    &sensor_dev_attr_qsfp22_int.dev_attr.attr,
    &sensor_dev_attr_qsfp23_int.dev_attr.attr,
    &sensor_dev_attr_qsfp24_int.dev_attr.attr,
    &sensor_dev_attr_qsfp25_int.dev_attr.attr,
    &sensor_dev_attr_qsfp26_int.dev_attr.attr,
    &sensor_dev_attr_qsfp27_int.dev_attr.attr,
    &sensor_dev_attr_qsfp28_int.dev_attr.attr,
    &sensor_dev_attr_qsfp29_int.dev_attr.attr,
    &sensor_dev_attr_qsfp30_int.dev_attr.attr,
    &sensor_dev_attr_qsfp31_int.dev_attr.attr,
    &sensor_dev_attr_qsfp32_int.dev_attr.attr,
    &sensor_dev_attr_sfp0_tx_fault.dev_attr.attr,
    &sensor_dev_attr_sfp0_rx_los.dev_attr.attr,
    &sensor_dev_attr_sfp0_prs.dev_attr.attr,
    &sensor_dev_attr_sfp0_tx_en.dev_attr.attr,
    &sensor_dev_attr_sfp0_led.dev_attr.attr,
    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,
    &sensor_dev_attr_qsfp17_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp18_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp19_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp20_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp21_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp22_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp23_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp24_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp25_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp26_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp27_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp28_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp29_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp30_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp31_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp32_reset.dev_attr.attr,

    &sensor_dev_attr_qsfp17_led.dev_attr.attr,
    &sensor_dev_attr_qsfp18_led.dev_attr.attr,
    &sensor_dev_attr_qsfp19_led.dev_attr.attr,
    &sensor_dev_attr_qsfp20_led.dev_attr.attr,
    &sensor_dev_attr_qsfp21_led.dev_attr.attr,
    &sensor_dev_attr_qsfp22_led.dev_attr.attr,
    &sensor_dev_attr_qsfp23_led.dev_attr.attr,
    &sensor_dev_attr_qsfp24_led.dev_attr.attr,
    &sensor_dev_attr_qsfp25_led.dev_attr.attr,
    &sensor_dev_attr_qsfp26_led.dev_attr.attr,
    &sensor_dev_attr_qsfp27_led.dev_attr.attr,
    &sensor_dev_attr_qsfp28_led.dev_attr.attr,
    &sensor_dev_attr_qsfp29_led.dev_attr.attr,
    &sensor_dev_attr_qsfp30_led.dev_attr.attr,
    &sensor_dev_attr_qsfp31_led.dev_attr.attr,
    &sensor_dev_attr_qsfp32_led.dev_attr.attr,
    NULL
};

static const struct attribute_group h4_32d_swpld3_group = {
    .attrs = h4_32d_swpld3_attributes,
};

static int h4_32d_swpld3_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;
    int i;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H4-32D SWPLD3 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &h4_32d_swpld3_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    dump_reg(data);
    dev_info(&client->dev, "[SWPLD3]Reseting QSFPs and SWPLD Registers...\n");
    cpld_i2c_write(data, QSFP_RST_REG0, 0x0);
    cpld_i2c_write(data, QSFP_RST_REG1, 0x0);
    cpld_i2c_write(data, QSFP_INITMOD_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_INITMOD_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG0, 0x0);
    cpld_i2c_write(data, QSFP_MODSEL_REG1, 0x0);
    msleep(2000);
    cpld_i2c_write(data, QSFP_RST_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG1, 0xFF);
    dev_info(&client->dev, "[SWPLD3]QSFPs and SWPLD Registers reset done.\n");
    cpld_i2c_write(data, SFP_REG1, 0x80);
    dump_reg(data);
    for (i=0;i<16;i++) data->reset_list[i] = 0;

    return 0;

exit:
    return status;
}

static void h4_32d_swpld3_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &h4_32d_swpld3_group);
    kfree(data);
}

static const struct of_device_id h4_32d_swpld3_of_ids[] = {
    {
        .compatible = "nokia,h4-32d_swpld3",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, h4_32d_swpld3_of_ids);

static const struct i2c_device_id h4_32d_swpld3_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, h4_32d_swpld3_ids);

static struct i2c_driver h4_32d_swpld3_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(h4_32d_swpld3_of_ids),
    },
    .probe        = h4_32d_swpld3_probe,
    .remove       = h4_32d_swpld3_remove,
    .id_table     = h4_32d_swpld3_ids,
    .address_list = cpld_address_list,
};

static int __init h4_32d_swpld3_init(void)
{
    return i2c_add_driver(&h4_32d_swpld3_driver);
}

static void __exit h4_32d_swpld3_exit(void)
{
    i2c_del_driver(&h4_32d_swpld3_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H4-32D CPLD driver");
MODULE_LICENSE("GPL");

module_init(h4_32d_swpld3_init);
module_exit(h4_32d_swpld3_exit);
