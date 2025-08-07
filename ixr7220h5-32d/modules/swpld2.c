//  * CPLD driver for Nokia-7220-IXR-H5-32D Router
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

#define DRIVER_NAME "swpld2"

// REGISTERS ADDRESS MAP
#define SCRATCH_REG             0x00
#define CODE_REV_REG            0x01
#define BOARD_REV_REG           0x02
#define BOARD_CFG_REG           0x03
#define LED_TEST_REG            0x08
#define RST_PLD_REG             0x10
#define RST_MSK_REG             0x11
#define RST_CTRL_REG            0x12
#define INT_CLR_REG             0x20
#define INT_MSK_REG             0x21
#define INT_REG                 0x22
#define PLD_INT_REG             0x23
#define SFP_INT_REG             0x24
#define QSFP_PRS_INT_REG0       0x28
#define QSFP_PRS_INT_REG1       0x29
#define QSFP_PRS_INT_REG2       0x2A
#define QSFP_PRS_INT_REG3       0x2B
#define QSFP_INT_EVT_REG0       0x2C
#define QSFP_INT_EVT_REG1       0x2D
#define QSFP_INT_EVT_REG2       0x2E
#define QSFP_INT_EVT_REG3       0x2F
#define QSFP_RST_REG0           0x30
#define QSFP_RST_REG1           0x31
#define QSFP_RST_REG2           0x32
#define QSFP_RST_REG3           0x33
#define QSFP_LPMODE_REG0        0x34
#define QSFP_LPMODE_REG1        0x35
#define QSFP_LPMODE_REG2        0x36
#define QSFP_LPMODE_REG3        0x37
#define QSFP_MODSEL_REG0        0x38
#define QSFP_MODSEL_REG1        0x39
#define QSFP_MODSEL_REG2        0x3A
#define QSFP_MODSEL_REG3        0x3B
#define QSFP_MODPRS_REG0        0x3C
#define QSFP_MODPRS_REG1        0x3D
#define QSFP_MODPRS_REG2        0x3E
#define QSFP_MODPRS_REG3        0x3F
#define QSFP_INT_STAT_REG0      0x40
#define QSFP_INT_STAT_REG1      0x41
#define QSFP_INT_STAT_REG2      0x42
#define QSFP_INT_STAT_REG3      0x43
#define SFP_CTRL_REG            0x44
#define SFP_STAT_REG            0x45
#define QSFP_LED_REG1           0x90
#define QSFP_BRKNUM_REG1        0xD0
#define CODE_DAY_REG            0xF0
#define CODE_MONTH_REG          0xF1
#define CODE_YEAR_REG           0xF2
#define TEST_CODE_REV_REG       0xF3

// REG BIT FIELD POSITION or MASK
#define BOARD_REV_REG_VER_MSK   0x7

#define LED_TEST_REG_AMB        0x0
#define LED_TEST_REG_GRN        0x1
#define LED_TEST_REG_BLINK      0x3
#define LED_TEST_REG_SRC_SEL    0x7

#define RST_PLD_REG_SOFT_RST    0x0

#define SFP0_TX_EN              0x0
#define SFP0_LED                0x2
#define SFP1_TX_EN              0x4
#define SFP1_LED                0x6

#define SFP0_PRS                0x0
#define SFP0_RX_LOS             0x1
#define SFP0_TX_FAULT           0x2
#define SFP1_PRS                0x4
#define SFP1_RX_LOS             0x5
#define SFP1_TX_FAULT           0x6

// common index of each qsfp modules
#define QSFP01_INDEX            0x0
#define QSFP02_INDEX            0x1
#define QSFP03_INDEX            0x2
#define QSFP04_INDEX            0x3
#define QSFP05_INDEX            0x4
#define QSFP06_INDEX            0x5
#define QSFP07_INDEX            0x6
#define QSFP08_INDEX            0x7
#define QSFP09_INDEX            0x0
#define QSFP10_INDEX            0x1
#define QSFP11_INDEX            0x2
#define QSFP12_INDEX            0x3
#define QSFP13_INDEX            0x4
#define QSFP14_INDEX            0x5
#define QSFP15_INDEX            0x6
#define QSFP16_INDEX            0x7

static const unsigned short cpld_address_list[] = {0x41, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
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
    dev_info(&client->dev, "[SWPLD2]QSFP_RESET_REG: 0x%02x, 0x%02x\n", val0, val1);

    val0 = cpld_i2c_read(data, QSFP_LPMODE_REG0);
    val1 = cpld_i2c_read(data, QSFP_LPMODE_REG1);
    dev_info(&client->dev, "[SWPLD2]QSFP_LPMODE_REG: 0x%02x, 0x%02x\n", val0, val1);
    
    val0 = cpld_i2c_read(data, QSFP_MODSEL_REG0);
    val1 = cpld_i2c_read(data, QSFP_MODSEL_REG1);
    dev_info(&client->dev, "[SWPLD2]QSFP_MODSEL_REG: 0x%02x, 0x%02x\n", val0, val1);
    
    val0 = cpld_i2c_read(data, QSFP_MODPRS_REG0);
    val1 = cpld_i2c_read(data, QSFP_MODPRS_REG1);
    dev_info(&client->dev, "[SWPLD2]QSFP_MODPRES_REG: 0x%02x, 0x%02x\n", val0, val1);
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

static ssize_t show_code_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_REV_REG);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_board_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, BOARD_REV_REG) & BOARD_REV_REG_VER_MSK;
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

static ssize_t show_rst(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, RST_PLD_REG);

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
    reg_val = cpld_i2c_read(data, RST_PLD_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, RST_PLD_REG, (reg_val | usr_val));

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

static ssize_t show_qsfp_lpmode0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_LPMODE_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_lpmode0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_LPMODE_REG0);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_LPMODE_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_lpmode1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_LPMODE_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_lpmode1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_LPMODE_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_LPMODE_REG1, (reg_val | usr_val));

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

static ssize_t show_qsfp_prs0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_prs1(struct device *dev, struct device_attribute *devattr, char *buf) 
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
    if (sda->index == 3)
        val = cpld_i2c_read(data, QSFP_MODPRS_REG2);
    if (sda->index == 4)
        val = cpld_i2c_read(data, QSFP_MODPRS_REG3);

    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_sfp_ctrl_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 reg_val = 0;
      
    val = cpld_i2c_read(data, SFP_CTRL_REG);

    switch (sda->index) {
    case SFP0_TX_EN:
    case SFP1_TX_EN:
        return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
    case SFP0_LED:
    case SFP1_LED:
        reg_val = (val>>sda->index) & 0x3;
        return sprintf(buf, "%d\n", reg_val);  
    default:
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    }
}

static ssize_t set_sfp_ctrl_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    case SFP0_TX_EN:
    case SFP1_TX_EN:
        if (usr_val > 1) {
            return -EINVAL;
        }
        mask = (~(1 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, SFP_CTRL_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, SFP_CTRL_REG, (reg_val | usr_val));
        break;
    case SFP0_LED:
    case SFP1_LED:
        if (usr_val > 3) {
            return -EINVAL;
        }
        mask = (~(3 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, SFP_CTRL_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, SFP_CTRL_REG, (reg_val | usr_val));
        break;
    default:
        sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    }
    
    return count;
}

static ssize_t show_sfp_stat_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, SFP_STAT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
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

static ssize_t show_qsfp_led(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val1 = 0;
    u8 val2 = 0;

    val1 = cpld_i2c_read(data, QSFP_LED_REG1 + sda->index * 2);
    val2 = cpld_i2c_read(data, QSFP_LED_REG1 + sda->index * 2 + 1);

    return sprintf(buf, "0x%02x%02x\n", val2, val1);
}

static ssize_t set_qsfp_led(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u16 usr_val = 0;
    u8 val = 0;

    int ret = kstrtou16(buf, 16, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 0xFFFF) {
        return -EINVAL;
    }

    val = usr_val & 0xFF;
    cpld_i2c_write(data, QSFP_LED_REG1 + sda->index*2, val);
    val = (usr_val & 0xFF00) >> 8;
    cpld_i2c_write(data, QSFP_LED_REG1 + sda->index*2 + 1, val);

    return count;
}

static ssize_t show_qsfp_brknum(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    int reg_index;
    int bit_index;

    reg_index = QSFP_BRKNUM_REG1 + (int)(sda->index/2);
    val = cpld_i2c_read(data, reg_index);
    bit_index = sda->index%2 * 4;

    return sprintf(buf, "0x%x\n", val>>bit_index & 0xF);
}

static ssize_t set_qsfp_brknum(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;
    int reg_index;
    int bit_index;

    reg_index = QSFP_BRKNUM_REG1 + (int)(sda->index/2);
    bit_index = sda->index%2 * 4;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 0xF) {
        return -EINVAL;
    }

    mask = (~(7 << bit_index)) & 0xFF;
    reg_val = cpld_i2c_read(data, reg_index);
    reg_val = reg_val & mask;
    usr_val = usr_val << bit_index;
    cpld_i2c_write(data, reg_index, (reg_val | usr_val));

    return count;
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0); 
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(board_ver, S_IRUGO, show_board_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(led_test_amb, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_AMB);
static SENSOR_DEVICE_ATTR(led_test_grn, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_GRN);
static SENSOR_DEVICE_ATTR(led_test_blink, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_BLINK);
static SENSOR_DEVICE_ATTR(led_test_src_sel, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_SRC_SEL);
static SENSOR_DEVICE_ATTR(rst_pld_soft, S_IRUGO | S_IWUSR, show_rst, set_rst, RST_PLD_REG_SOFT_RST);

static SENSOR_DEVICE_ATTR(port_1_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(port_2_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(port_3_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(port_4_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(port_5_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(port_6_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(port_7_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(port_8_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(port_9_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(port_10_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(port_11_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(port_12_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(port_13_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(port_14_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(port_15_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(port_16_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(port_1_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(port_2_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(port_3_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(port_4_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(port_5_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(port_6_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(port_7_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(port_8_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(port_9_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(port_10_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(port_11_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(port_12_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(port_13_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(port_14_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(port_15_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(port_16_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(port_1_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(port_2_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(port_3_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(port_4_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(port_5_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(port_6_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(port_7_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(port_8_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(port_9_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(port_10_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(port_11_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(port_12_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(port_13_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(port_14_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(port_15_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(port_16_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(port_1_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(port_2_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(port_3_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(port_4_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(port_5_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(port_6_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(port_7_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(port_8_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(port_9_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(port_10_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(port_11_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(port_12_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(port_13_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(port_14_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(port_15_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(port_16_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP16_INDEX);

static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 1);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 2);
static SENSOR_DEVICE_ATTR(modprs_reg3, S_IRUGO, show_modprs_reg, NULL, 3);
static SENSOR_DEVICE_ATTR(modprs_reg4, S_IRUGO, show_modprs_reg, NULL, 4);

static SENSOR_DEVICE_ATTR(port_33_tx_fault, S_IRUGO, show_sfp_stat_reg, NULL, SFP0_TX_FAULT);
static SENSOR_DEVICE_ATTR(port_33_rx_los,   S_IRUGO, show_sfp_stat_reg, NULL, SFP0_RX_LOS);
static SENSOR_DEVICE_ATTR(port_33_prs,      S_IRUGO, show_sfp_stat_reg, NULL, SFP0_PRS);
static SENSOR_DEVICE_ATTR(port_34_tx_fault, S_IRUGO, show_sfp_stat_reg, NULL, SFP1_TX_FAULT);
static SENSOR_DEVICE_ATTR(port_34_rx_los,   S_IRUGO, show_sfp_stat_reg, NULL, SFP1_RX_LOS);
static SENSOR_DEVICE_ATTR(port_34_prs,      S_IRUGO, show_sfp_stat_reg, NULL, SFP1_PRS);
static SENSOR_DEVICE_ATTR(port_33_tx_en,    S_IRUGO | S_IWUSR, show_sfp_ctrl_reg, set_sfp_ctrl_reg, SFP0_TX_EN);
static SENSOR_DEVICE_ATTR(port_33_led,      S_IRUGO | S_IWUSR, show_sfp_ctrl_reg, set_sfp_ctrl_reg, SFP0_LED);
static SENSOR_DEVICE_ATTR(port_34_tx_en,    S_IRUGO | S_IWUSR, show_sfp_ctrl_reg, set_sfp_ctrl_reg, SFP1_TX_EN);
static SENSOR_DEVICE_ATTR(port_34_led,      S_IRUGO | S_IWUSR, show_sfp_ctrl_reg, set_sfp_ctrl_reg, SFP1_LED);

static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);

static SENSOR_DEVICE_ATTR(port_1_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 0);
static SENSOR_DEVICE_ATTR(port_2_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 1);
static SENSOR_DEVICE_ATTR(port_3_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 2);
static SENSOR_DEVICE_ATTR(port_4_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 3);
static SENSOR_DEVICE_ATTR(port_5_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 4);
static SENSOR_DEVICE_ATTR(port_6_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 5);
static SENSOR_DEVICE_ATTR(port_7_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 6);
static SENSOR_DEVICE_ATTR(port_8_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 7);
static SENSOR_DEVICE_ATTR(port_9_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 8);
static SENSOR_DEVICE_ATTR(port_10_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 9);
static SENSOR_DEVICE_ATTR(port_11_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 10);
static SENSOR_DEVICE_ATTR(port_12_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 11);
static SENSOR_DEVICE_ATTR(port_13_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 12);
static SENSOR_DEVICE_ATTR(port_14_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 13);
static SENSOR_DEVICE_ATTR(port_15_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 14);
static SENSOR_DEVICE_ATTR(port_16_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 15);

static SENSOR_DEVICE_ATTR(port_1_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 0);
static SENSOR_DEVICE_ATTR(port_2_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 1);
static SENSOR_DEVICE_ATTR(port_3_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 2);
static SENSOR_DEVICE_ATTR(port_4_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 3);
static SENSOR_DEVICE_ATTR(port_5_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 4);
static SENSOR_DEVICE_ATTR(port_6_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 5);
static SENSOR_DEVICE_ATTR(port_7_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 6);
static SENSOR_DEVICE_ATTR(port_8_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 7);
static SENSOR_DEVICE_ATTR(port_9_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 8);
static SENSOR_DEVICE_ATTR(port_10_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 9);
static SENSOR_DEVICE_ATTR(port_11_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 10);
static SENSOR_DEVICE_ATTR(port_12_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 11);
static SENSOR_DEVICE_ATTR(port_13_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 12);
static SENSOR_DEVICE_ATTR(port_14_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 13);
static SENSOR_DEVICE_ATTR(port_15_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 14);
static SENSOR_DEVICE_ATTR(port_16_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 15);

static struct attribute *swpld2_attributes[] = {
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,
    &sensor_dev_attr_led_test_amb.dev_attr.attr,
    &sensor_dev_attr_led_test_grn.dev_attr.attr,
    &sensor_dev_attr_led_test_blink.dev_attr.attr,
    &sensor_dev_attr_led_test_src_sel.dev_attr.attr,
    &sensor_dev_attr_rst_pld_soft.dev_attr.attr,

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

    &sensor_dev_attr_port_1_modsel.dev_attr.attr,
    &sensor_dev_attr_port_2_modsel.dev_attr.attr,
    &sensor_dev_attr_port_3_modsel.dev_attr.attr,
    &sensor_dev_attr_port_4_modsel.dev_attr.attr,
    &sensor_dev_attr_port_5_modsel.dev_attr.attr,
    &sensor_dev_attr_port_6_modsel.dev_attr.attr,
    &sensor_dev_attr_port_7_modsel.dev_attr.attr,
    &sensor_dev_attr_port_8_modsel.dev_attr.attr,
    &sensor_dev_attr_port_9_modsel.dev_attr.attr,
    &sensor_dev_attr_port_10_modsel.dev_attr.attr,
    &sensor_dev_attr_port_11_modsel.dev_attr.attr,
    &sensor_dev_attr_port_12_modsel.dev_attr.attr,
    &sensor_dev_attr_port_13_modsel.dev_attr.attr,
    &sensor_dev_attr_port_14_modsel.dev_attr.attr,
    &sensor_dev_attr_port_15_modsel.dev_attr.attr,
    &sensor_dev_attr_port_16_modsel.dev_attr.attr,

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
    &sensor_dev_attr_modprs_reg3.dev_attr.attr,
    &sensor_dev_attr_modprs_reg4.dev_attr.attr,

    &sensor_dev_attr_port_33_tx_fault.dev_attr.attr,
    &sensor_dev_attr_port_33_rx_los.dev_attr.attr,
    &sensor_dev_attr_port_33_prs.dev_attr.attr,
    &sensor_dev_attr_port_34_tx_fault.dev_attr.attr,
    &sensor_dev_attr_port_34_rx_los.dev_attr.attr,
    &sensor_dev_attr_port_34_prs.dev_attr.attr,
    &sensor_dev_attr_port_33_tx_en.dev_attr.attr,
    &sensor_dev_attr_port_33_led.dev_attr.attr,
    &sensor_dev_attr_port_34_tx_en.dev_attr.attr,
    &sensor_dev_attr_port_34_led.dev_attr.attr,

    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,

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

    &sensor_dev_attr_port_1_brknum.dev_attr.attr,
    &sensor_dev_attr_port_2_brknum.dev_attr.attr,
    &sensor_dev_attr_port_3_brknum.dev_attr.attr,
    &sensor_dev_attr_port_4_brknum.dev_attr.attr,
    &sensor_dev_attr_port_5_brknum.dev_attr.attr,
    &sensor_dev_attr_port_6_brknum.dev_attr.attr,
    &sensor_dev_attr_port_7_brknum.dev_attr.attr,
    &sensor_dev_attr_port_8_brknum.dev_attr.attr,
    &sensor_dev_attr_port_9_brknum.dev_attr.attr,
    &sensor_dev_attr_port_10_brknum.dev_attr.attr,
    &sensor_dev_attr_port_11_brknum.dev_attr.attr,
    &sensor_dev_attr_port_12_brknum.dev_attr.attr,
    &sensor_dev_attr_port_13_brknum.dev_attr.attr,
    &sensor_dev_attr_port_14_brknum.dev_attr.attr,
    &sensor_dev_attr_port_15_brknum.dev_attr.attr,
    &sensor_dev_attr_port_16_brknum.dev_attr.attr,
    NULL
};

static const struct attribute_group swpld2_group = {
    .attrs = swpld2_attributes,
};

static int swpld2_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;
    int i;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia SWPLD2 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &swpld2_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    dump_reg(data);
    dev_info(&client->dev, "[SWPLD2]Reseting PORTs ...\n");
    cpld_i2c_write(data, QSFP_MODSEL_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_LPMODE_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_LPMODE_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG1, 0xFF);
    msleep(500);
    cpld_i2c_write(data, QSFP_RST_REG0, 0x0);
    cpld_i2c_write(data, QSFP_RST_REG1, 0x0);
    dev_info(&client->dev, "[SWPLD2]PORTs reset done.\n");
    cpld_i2c_write(data, SFP_CTRL_REG, 0x0);
    dump_reg(data);

    return 0;

exit:
    return status;
}

static void swpld2_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &swpld2_group);
    kfree(data);
}

static const struct of_device_id swpld2_of_ids[] = {
    {
        .compatible = "nokia,swpld2",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, swpld2_of_ids);

static const struct i2c_device_id swpld2_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, swpld2_ids);

static struct i2c_driver swpld2_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(swpld2_of_ids),
    },
    .probe        = swpld2_probe,
    .remove       = swpld2_remove,
    .id_table     = swpld2_ids,
    .address_list = cpld_address_list,
};

static int __init swpld2_init(void)
{
    return i2c_add_driver(&swpld2_driver);
}

static void __exit swpld2_exit(void)
{
    i2c_del_driver(&swpld2_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA CPLD driver");
MODULE_LICENSE("GPL");

module_init(swpld2_init);
module_exit(swpld2_exit);
