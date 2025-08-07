//  * CPLD driver for Nokia-7220-IXR-H5-64D Router
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

#define DRIVER_NAME "swpld3"

// REGISTERS ADDRESS MAP
#define SCRATCH_REG             0x00
#define CODE_REV_REG            0x01
#define BOARD_REV_REG           0x02
#define BOARD_CFG_REG           0x03
#define SYS_EEPROM_REG          0x05
#define BOARD_CTRL_REG          0x07
#define LED_TEST_REG            0x08
#define RST_PLD_REG             0x10
#define INT_CLR_REG             0x20
#define INT_MSK_REG             0x21
#define INT_REG                 0x22
#define PLD_INT_REG0            0x23
#define PLD_INT_REG1            0x24
#define PLD_INT_REG2            0x25
#define PLD_INT_REG3            0x26
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
#define PERIF_STAT_REG0         0x50
#define PERIF_STAT_REG1         0x51
#define PERIF_STAT_REG2         0x54
#define PERIF_STAT_REG3         0x55
#define PWR_STATUS_REG0         0x68
#define PWR_STATUS_REG1         0x69
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

// common index of each qsfp modules
#define QSFP17_INDEX            0x0
#define QSFP18_INDEX            0x1
#define QSFP19_INDEX            0x2
#define QSFP20_INDEX            0x3
#define QSFP21_INDEX            0x4
#define QSFP22_INDEX            0x5
#define QSFP23_INDEX            0x6
#define QSFP24_INDEX            0x7
#define QSFP25_INDEX            0x0
#define QSFP26_INDEX            0x1
#define QSFP27_INDEX            0x2
#define QSFP28_INDEX            0x3
#define QSFP29_INDEX            0x4
#define QSFP30_INDEX            0x5
#define QSFP31_INDEX            0x6
#define QSFP32_INDEX            0x7
#define QSFP49_INDEX            0x0
#define QSFP50_INDEX            0x1
#define QSFP51_INDEX            0x2
#define QSFP52_INDEX            0x3
#define QSFP53_INDEX            0x4
#define QSFP54_INDEX            0x5
#define QSFP55_INDEX            0x6
#define QSFP56_INDEX            0x7
#define QSFP57_INDEX            0x0
#define QSFP58_INDEX            0x1
#define QSFP59_INDEX            0x2
#define QSFP60_INDEX            0x3
#define QSFP61_INDEX            0x4
#define QSFP62_INDEX            0x5
#define QSFP63_INDEX            0x6
#define QSFP64_INDEX            0x7

static const unsigned short cpld_address_list[] = {0x45, I2C_CLIENT_END};

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
    u8 val2 = 0;
    u8 val3 = 0;

    val0 = cpld_i2c_read(data, QSFP_RST_REG0);
    val1 = cpld_i2c_read(data, QSFP_RST_REG1);
    val2 = cpld_i2c_read(data, QSFP_RST_REG2);
    val3 = cpld_i2c_read(data, QSFP_RST_REG3);
    dev_info(&client->dev, "[SWPLD3]QSFP_RESET_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

    val0 = cpld_i2c_read(data, QSFP_LPMODE_REG0);
    val1 = cpld_i2c_read(data, QSFP_LPMODE_REG1);
    val2 = cpld_i2c_read(data, QSFP_LPMODE_REG2);
    val3 = cpld_i2c_read(data, QSFP_LPMODE_REG3);
    dev_info(&client->dev, "[SWPLD3]QSFP_LPMODE_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);
    
    val0 = cpld_i2c_read(data, QSFP_MODSEL_REG0);
    val1 = cpld_i2c_read(data, QSFP_MODSEL_REG1);
    val2 = cpld_i2c_read(data, QSFP_MODSEL_REG2);
    val3 = cpld_i2c_read(data, QSFP_MODSEL_REG3);
    dev_info(&client->dev, "[SWPLD3]QSFP_MODSEL_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);
    
    val0 = cpld_i2c_read(data, QSFP_MODPRS_REG0);
    val1 = cpld_i2c_read(data, QSFP_MODPRS_REG1);
    val2 = cpld_i2c_read(data, QSFP_MODPRS_REG2);
    val3 = cpld_i2c_read(data, QSFP_MODPRS_REG3);
    dev_info(&client->dev, "[SWPLD3]QSFP_MODPRES_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3);

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

static ssize_t show_qsfp_rst2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_RST_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_rst2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_RST_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_RST_REG2, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_rst3(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_RST_REG3);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_rst3(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_RST_REG3);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_RST_REG3, (reg_val | usr_val));

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

static ssize_t show_qsfp_lpmode2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_LPMODE_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_lpmode2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_LPMODE_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_LPMODE_REG2, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_lpmode3(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_LPMODE_REG3);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_lpmode3(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_LPMODE_REG3);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_LPMODE_REG3, (reg_val | usr_val));

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

static ssize_t show_qsfp_modsel2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODSEL_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_modsel2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_MODSEL_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_MODSEL_REG2, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_modsel3(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODSEL_REG3);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_qsfp_modsel3(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, QSFP_MODSEL_REG3);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, QSFP_MODSEL_REG3, (reg_val | usr_val));

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

static ssize_t show_qsfp_prs2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_prs3(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG3);

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

static SENSOR_DEVICE_ATTR(port_17_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(port_18_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(port_19_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(port_20_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(port_21_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(port_22_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(port_23_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(port_24_rst, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(port_25_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(port_26_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(port_27_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(port_28_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(port_29_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(port_30_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(port_31_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(port_32_rst, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(port_49_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP49_INDEX);
static SENSOR_DEVICE_ATTR(port_50_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP50_INDEX);
static SENSOR_DEVICE_ATTR(port_51_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP51_INDEX);
static SENSOR_DEVICE_ATTR(port_52_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP52_INDEX);
static SENSOR_DEVICE_ATTR(port_53_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP53_INDEX);
static SENSOR_DEVICE_ATTR(port_54_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP54_INDEX);
static SENSOR_DEVICE_ATTR(port_55_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP55_INDEX);
static SENSOR_DEVICE_ATTR(port_56_rst, S_IRUGO | S_IWUSR, show_qsfp_rst2, set_qsfp_rst2, QSFP56_INDEX);
static SENSOR_DEVICE_ATTR(port_57_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP57_INDEX);
static SENSOR_DEVICE_ATTR(port_58_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP58_INDEX);
static SENSOR_DEVICE_ATTR(port_59_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP59_INDEX);
static SENSOR_DEVICE_ATTR(port_60_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP60_INDEX);
static SENSOR_DEVICE_ATTR(port_61_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP61_INDEX);
static SENSOR_DEVICE_ATTR(port_62_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP62_INDEX);
static SENSOR_DEVICE_ATTR(port_63_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP63_INDEX);
static SENSOR_DEVICE_ATTR(port_64_rst, S_IRUGO | S_IWUSR, show_qsfp_rst3, set_qsfp_rst3, QSFP64_INDEX);

static SENSOR_DEVICE_ATTR(port_17_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(port_18_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(port_19_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(port_20_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(port_21_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(port_22_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(port_23_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(port_24_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode0, set_qsfp_lpmode0, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(port_25_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(port_26_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(port_27_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(port_28_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(port_29_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(port_30_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(port_31_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(port_32_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode1, set_qsfp_lpmode1, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(port_49_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP49_INDEX);
static SENSOR_DEVICE_ATTR(port_50_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP50_INDEX);
static SENSOR_DEVICE_ATTR(port_51_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP51_INDEX);
static SENSOR_DEVICE_ATTR(port_52_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP52_INDEX);
static SENSOR_DEVICE_ATTR(port_53_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP53_INDEX);
static SENSOR_DEVICE_ATTR(port_54_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP54_INDEX);
static SENSOR_DEVICE_ATTR(port_55_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP55_INDEX);
static SENSOR_DEVICE_ATTR(port_56_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode2, set_qsfp_lpmode2, QSFP56_INDEX);
static SENSOR_DEVICE_ATTR(port_57_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP57_INDEX);
static SENSOR_DEVICE_ATTR(port_58_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP58_INDEX);
static SENSOR_DEVICE_ATTR(port_59_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP59_INDEX);
static SENSOR_DEVICE_ATTR(port_60_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP60_INDEX);
static SENSOR_DEVICE_ATTR(port_61_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP61_INDEX);
static SENSOR_DEVICE_ATTR(port_62_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP62_INDEX);
static SENSOR_DEVICE_ATTR(port_63_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP63_INDEX);
static SENSOR_DEVICE_ATTR(port_64_lpmod, S_IRUGO | S_IWUSR, show_qsfp_lpmode3, set_qsfp_lpmode3, QSFP64_INDEX);

static SENSOR_DEVICE_ATTR(port_17_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(port_18_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(port_19_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(port_20_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(port_21_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(port_22_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(port_23_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(port_24_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(port_25_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(port_26_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(port_27_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(port_28_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(port_29_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(port_30_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(port_31_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(port_32_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(port_49_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP49_INDEX);
static SENSOR_DEVICE_ATTR(port_50_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP50_INDEX);
static SENSOR_DEVICE_ATTR(port_51_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP51_INDEX);
static SENSOR_DEVICE_ATTR(port_52_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP52_INDEX);
static SENSOR_DEVICE_ATTR(port_53_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP53_INDEX);
static SENSOR_DEVICE_ATTR(port_54_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP54_INDEX);
static SENSOR_DEVICE_ATTR(port_55_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP55_INDEX);
static SENSOR_DEVICE_ATTR(port_56_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel2, set_qsfp_modsel2, QSFP56_INDEX);
static SENSOR_DEVICE_ATTR(port_57_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP57_INDEX);
static SENSOR_DEVICE_ATTR(port_58_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP58_INDEX);
static SENSOR_DEVICE_ATTR(port_59_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP59_INDEX);
static SENSOR_DEVICE_ATTR(port_60_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP60_INDEX);
static SENSOR_DEVICE_ATTR(port_61_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP61_INDEX);
static SENSOR_DEVICE_ATTR(port_62_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP62_INDEX);
static SENSOR_DEVICE_ATTR(port_63_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP63_INDEX);
static SENSOR_DEVICE_ATTR(port_64_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel3, set_qsfp_modsel3, QSFP64_INDEX);

static SENSOR_DEVICE_ATTR(port_17_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(port_18_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(port_19_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(port_20_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(port_21_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(port_22_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(port_23_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(port_24_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(port_25_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(port_26_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(port_27_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(port_28_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(port_29_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(port_30_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(port_31_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(port_32_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP32_INDEX);
static SENSOR_DEVICE_ATTR(port_49_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP49_INDEX);
static SENSOR_DEVICE_ATTR(port_50_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP50_INDEX);
static SENSOR_DEVICE_ATTR(port_51_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP51_INDEX);
static SENSOR_DEVICE_ATTR(port_52_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP52_INDEX);
static SENSOR_DEVICE_ATTR(port_53_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP53_INDEX);
static SENSOR_DEVICE_ATTR(port_54_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP54_INDEX);
static SENSOR_DEVICE_ATTR(port_55_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP55_INDEX);
static SENSOR_DEVICE_ATTR(port_56_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP56_INDEX);
static SENSOR_DEVICE_ATTR(port_57_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP57_INDEX);
static SENSOR_DEVICE_ATTR(port_58_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP58_INDEX);
static SENSOR_DEVICE_ATTR(port_59_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP59_INDEX);
static SENSOR_DEVICE_ATTR(port_60_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP60_INDEX);
static SENSOR_DEVICE_ATTR(port_61_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP61_INDEX);
static SENSOR_DEVICE_ATTR(port_62_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP62_INDEX);
static SENSOR_DEVICE_ATTR(port_63_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP63_INDEX);
static SENSOR_DEVICE_ATTR(port_64_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP64_INDEX);

static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 1);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 2);
static SENSOR_DEVICE_ATTR(modprs_reg3, S_IRUGO, show_modprs_reg, NULL, 3);
static SENSOR_DEVICE_ATTR(modprs_reg4, S_IRUGO, show_modprs_reg, NULL, 4);

static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);

static SENSOR_DEVICE_ATTR(port_17_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 0);
static SENSOR_DEVICE_ATTR(port_18_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 1);
static SENSOR_DEVICE_ATTR(port_19_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 2);
static SENSOR_DEVICE_ATTR(port_20_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 3);
static SENSOR_DEVICE_ATTR(port_21_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 4);
static SENSOR_DEVICE_ATTR(port_22_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 5);
static SENSOR_DEVICE_ATTR(port_23_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 6);
static SENSOR_DEVICE_ATTR(port_24_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 7);
static SENSOR_DEVICE_ATTR(port_25_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 8);
static SENSOR_DEVICE_ATTR(port_26_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 9);
static SENSOR_DEVICE_ATTR(port_27_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 10);
static SENSOR_DEVICE_ATTR(port_28_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 11);
static SENSOR_DEVICE_ATTR(port_29_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 12);
static SENSOR_DEVICE_ATTR(port_30_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 13);
static SENSOR_DEVICE_ATTR(port_31_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 14);
static SENSOR_DEVICE_ATTR(port_32_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 15);
static SENSOR_DEVICE_ATTR(port_49_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 16);
static SENSOR_DEVICE_ATTR(port_50_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 17);
static SENSOR_DEVICE_ATTR(port_51_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 18);
static SENSOR_DEVICE_ATTR(port_52_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 19);
static SENSOR_DEVICE_ATTR(port_53_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 20);
static SENSOR_DEVICE_ATTR(port_54_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 21);
static SENSOR_DEVICE_ATTR(port_55_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 22);
static SENSOR_DEVICE_ATTR(port_56_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 23);
static SENSOR_DEVICE_ATTR(port_57_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 24);
static SENSOR_DEVICE_ATTR(port_58_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 25);
static SENSOR_DEVICE_ATTR(port_59_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 26);
static SENSOR_DEVICE_ATTR(port_60_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 27);
static SENSOR_DEVICE_ATTR(port_61_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 28);
static SENSOR_DEVICE_ATTR(port_62_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 29);
static SENSOR_DEVICE_ATTR(port_63_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 30);
static SENSOR_DEVICE_ATTR(port_64_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 31);

static SENSOR_DEVICE_ATTR(port_17_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 0);
static SENSOR_DEVICE_ATTR(port_18_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 1);
static SENSOR_DEVICE_ATTR(port_19_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 2);
static SENSOR_DEVICE_ATTR(port_20_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 3);
static SENSOR_DEVICE_ATTR(port_21_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 4);
static SENSOR_DEVICE_ATTR(port_22_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 5);
static SENSOR_DEVICE_ATTR(port_23_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 6);
static SENSOR_DEVICE_ATTR(port_24_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 7);
static SENSOR_DEVICE_ATTR(port_25_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 8);
static SENSOR_DEVICE_ATTR(port_26_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 9);
static SENSOR_DEVICE_ATTR(port_27_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 10);
static SENSOR_DEVICE_ATTR(port_28_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 11);
static SENSOR_DEVICE_ATTR(port_29_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 12);
static SENSOR_DEVICE_ATTR(port_30_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 13);
static SENSOR_DEVICE_ATTR(port_31_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 14);
static SENSOR_DEVICE_ATTR(port_32_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 15);
static SENSOR_DEVICE_ATTR(port_49_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 16);
static SENSOR_DEVICE_ATTR(port_50_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 17);
static SENSOR_DEVICE_ATTR(port_51_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 18);
static SENSOR_DEVICE_ATTR(port_52_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 19);
static SENSOR_DEVICE_ATTR(port_53_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 20);
static SENSOR_DEVICE_ATTR(port_54_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 21);
static SENSOR_DEVICE_ATTR(port_55_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 22);
static SENSOR_DEVICE_ATTR(port_56_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 23);
static SENSOR_DEVICE_ATTR(port_57_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 24);
static SENSOR_DEVICE_ATTR(port_58_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 25);
static SENSOR_DEVICE_ATTR(port_59_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 26);
static SENSOR_DEVICE_ATTR(port_60_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 27);
static SENSOR_DEVICE_ATTR(port_61_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 28);
static SENSOR_DEVICE_ATTR(port_62_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 29);
static SENSOR_DEVICE_ATTR(port_63_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 30);
static SENSOR_DEVICE_ATTR(port_64_brknum, S_IRUGO | S_IWUSR, show_qsfp_brknum, set_qsfp_brknum, 31);

static struct attribute *swpld3_attributes[] = {
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,
    &sensor_dev_attr_led_test_amb.dev_attr.attr,
    &sensor_dev_attr_led_test_grn.dev_attr.attr,
    &sensor_dev_attr_led_test_blink.dev_attr.attr,
    &sensor_dev_attr_led_test_src_sel.dev_attr.attr,
    &sensor_dev_attr_rst_pld_soft.dev_attr.attr,

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

    &sensor_dev_attr_port_17_modsel.dev_attr.attr,
    &sensor_dev_attr_port_18_modsel.dev_attr.attr,
    &sensor_dev_attr_port_19_modsel.dev_attr.attr,
    &sensor_dev_attr_port_20_modsel.dev_attr.attr,
    &sensor_dev_attr_port_21_modsel.dev_attr.attr,
    &sensor_dev_attr_port_22_modsel.dev_attr.attr,
    &sensor_dev_attr_port_23_modsel.dev_attr.attr,
    &sensor_dev_attr_port_24_modsel.dev_attr.attr,
    &sensor_dev_attr_port_25_modsel.dev_attr.attr,
    &sensor_dev_attr_port_26_modsel.dev_attr.attr,
    &sensor_dev_attr_port_27_modsel.dev_attr.attr,
    &sensor_dev_attr_port_28_modsel.dev_attr.attr,
    &sensor_dev_attr_port_29_modsel.dev_attr.attr,
    &sensor_dev_attr_port_30_modsel.dev_attr.attr,
    &sensor_dev_attr_port_31_modsel.dev_attr.attr,
    &sensor_dev_attr_port_32_modsel.dev_attr.attr,
    &sensor_dev_attr_port_49_modsel.dev_attr.attr,
    &sensor_dev_attr_port_50_modsel.dev_attr.attr,
    &sensor_dev_attr_port_51_modsel.dev_attr.attr,
    &sensor_dev_attr_port_52_modsel.dev_attr.attr,
    &sensor_dev_attr_port_53_modsel.dev_attr.attr,
    &sensor_dev_attr_port_54_modsel.dev_attr.attr,
    &sensor_dev_attr_port_55_modsel.dev_attr.attr,
    &sensor_dev_attr_port_56_modsel.dev_attr.attr,
    &sensor_dev_attr_port_57_modsel.dev_attr.attr,
    &sensor_dev_attr_port_58_modsel.dev_attr.attr,
    &sensor_dev_attr_port_59_modsel.dev_attr.attr,
    &sensor_dev_attr_port_60_modsel.dev_attr.attr,
    &sensor_dev_attr_port_61_modsel.dev_attr.attr,
    &sensor_dev_attr_port_62_modsel.dev_attr.attr,
    &sensor_dev_attr_port_63_modsel.dev_attr.attr,
    &sensor_dev_attr_port_64_modsel.dev_attr.attr,
    
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

    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,

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
    &sensor_dev_attr_port_49_led.dev_attr.attr,
    &sensor_dev_attr_port_50_led.dev_attr.attr,
    &sensor_dev_attr_port_51_led.dev_attr.attr,
    &sensor_dev_attr_port_52_led.dev_attr.attr,
    &sensor_dev_attr_port_53_led.dev_attr.attr,
    &sensor_dev_attr_port_54_led.dev_attr.attr,
    &sensor_dev_attr_port_55_led.dev_attr.attr,
    &sensor_dev_attr_port_56_led.dev_attr.attr,
    &sensor_dev_attr_port_57_led.dev_attr.attr,
    &sensor_dev_attr_port_58_led.dev_attr.attr,
    &sensor_dev_attr_port_59_led.dev_attr.attr,
    &sensor_dev_attr_port_60_led.dev_attr.attr,
    &sensor_dev_attr_port_61_led.dev_attr.attr,
    &sensor_dev_attr_port_62_led.dev_attr.attr,
    &sensor_dev_attr_port_63_led.dev_attr.attr,
    &sensor_dev_attr_port_64_led.dev_attr.attr,

    &sensor_dev_attr_port_17_brknum.dev_attr.attr,
    &sensor_dev_attr_port_18_brknum.dev_attr.attr,
    &sensor_dev_attr_port_19_brknum.dev_attr.attr,
    &sensor_dev_attr_port_20_brknum.dev_attr.attr,
    &sensor_dev_attr_port_21_brknum.dev_attr.attr,
    &sensor_dev_attr_port_22_brknum.dev_attr.attr,
    &sensor_dev_attr_port_23_brknum.dev_attr.attr,
    &sensor_dev_attr_port_24_brknum.dev_attr.attr,
    &sensor_dev_attr_port_25_brknum.dev_attr.attr,
    &sensor_dev_attr_port_26_brknum.dev_attr.attr,
    &sensor_dev_attr_port_27_brknum.dev_attr.attr,
    &sensor_dev_attr_port_28_brknum.dev_attr.attr,
    &sensor_dev_attr_port_29_brknum.dev_attr.attr,
    &sensor_dev_attr_port_30_brknum.dev_attr.attr,
    &sensor_dev_attr_port_31_brknum.dev_attr.attr,
    &sensor_dev_attr_port_32_brknum.dev_attr.attr,
    &sensor_dev_attr_port_49_brknum.dev_attr.attr,
    &sensor_dev_attr_port_50_brknum.dev_attr.attr,
    &sensor_dev_attr_port_51_brknum.dev_attr.attr,
    &sensor_dev_attr_port_52_brknum.dev_attr.attr,
    &sensor_dev_attr_port_53_brknum.dev_attr.attr,
    &sensor_dev_attr_port_54_brknum.dev_attr.attr,
    &sensor_dev_attr_port_55_brknum.dev_attr.attr,
    &sensor_dev_attr_port_56_brknum.dev_attr.attr,
    &sensor_dev_attr_port_57_brknum.dev_attr.attr,
    &sensor_dev_attr_port_58_brknum.dev_attr.attr,
    &sensor_dev_attr_port_59_brknum.dev_attr.attr,
    &sensor_dev_attr_port_60_brknum.dev_attr.attr,
    &sensor_dev_attr_port_61_brknum.dev_attr.attr,
    &sensor_dev_attr_port_62_brknum.dev_attr.attr,
    &sensor_dev_attr_port_63_brknum.dev_attr.attr,
    &sensor_dev_attr_port_64_brknum.dev_attr.attr,
    
    NULL
};

static const struct attribute_group swpld3_group = {
    .attrs = swpld3_attributes,
};

static int swpld3_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;
    int i;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia SWPLD3 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &swpld3_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }
    
    dump_reg(data);
    dev_info(&client->dev, "[SWPLD3]Reseting PORTs ...\n");
    cpld_i2c_write(data, QSFP_MODSEL_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG2, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG3, 0xFF);
    cpld_i2c_write(data, QSFP_LPMODE_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_LPMODE_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_LPMODE_REG2, 0xFF);
    cpld_i2c_write(data, QSFP_LPMODE_REG3, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG2, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG3, 0xFF);
    msleep(500);
    cpld_i2c_write(data, QSFP_RST_REG0, 0x0);
    cpld_i2c_write(data, QSFP_RST_REG1, 0x0);
    cpld_i2c_write(data, QSFP_RST_REG2, 0x0);
    cpld_i2c_write(data, QSFP_RST_REG3, 0x0);
    dev_info(&client->dev, "[SWPLD3]PORTs reset done.\n");
    dump_reg(data);
    
    return 0;

exit:
    return status;
}

static void swpld3_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &swpld3_group);
    kfree(data);
}

static const struct of_device_id swpld3_of_ids[] = {
    {
        .compatible = "nokia,swpld3",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, swpld3_of_ids);

static const struct i2c_device_id swpld3_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, swpld3_ids);

static struct i2c_driver swpld3_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(swpld3_of_ids),
    },
    .probe        = swpld3_probe,
    .remove       = swpld3_remove,
    .id_table     = swpld3_ids,
    .address_list = cpld_address_list,
};

static int __init swpld3_init(void)
{
    return i2c_add_driver(&swpld3_driver);
}

static void __exit swpld3_exit(void)
{
    i2c_del_driver(&swpld3_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA CPLD driver");
MODULE_LICENSE("GPL");

module_init(swpld3_init);
module_exit(swpld3_exit);
