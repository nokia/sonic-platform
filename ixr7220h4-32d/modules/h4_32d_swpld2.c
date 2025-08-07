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

#define DRIVER_NAME "h4_32d_swpld2"

// REGISTERS ADDRESS MAP
#define CODE_REV_REG            0x01
#define SYNC_REG                0x04
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
#define QSFP_LED_REG1           0x90
#define CODE_DAY_REG            0xF0
#define CODE_MONTH_REG          0xF1
#define CODE_YEAR_REG           0xF2
#define TEST_CODE_REV_REG       0xF3

// REG BIT FIELD POSITION or MASK
#define SYNC_REG_CLK_SEL0       0x0
#define SYNC_REG_CLK_SEL1       0x1
#define SYNC_REG_SYNCE_CPLD     0x6
#define SYNC_REG_PRESENCE       0x7

#define LED_TEST_REG_AMB        0x0
#define LED_TEST_REG_GRN        0x1
#define LED_TEST_REG_BLINK      0x3
#define LED_TEST_REG_SRC_SEL    0x7

#define RST_REG_PLD_SOFT_RST    0x0

#define HITLESS_REG_EN          0x0

// common index of each qsfp modules
#define QSFP01_INDEX            0x7
#define QSFP02_INDEX            0x6
#define QSFP03_INDEX            0x5
#define QSFP04_INDEX            0x4
#define QSFP05_INDEX            0x3
#define QSFP06_INDEX            0x2
#define QSFP07_INDEX            0x1
#define QSFP08_INDEX            0x0
#define QSFP09_INDEX            0x7
#define QSFP10_INDEX            0x6
#define QSFP11_INDEX            0x5
#define QSFP12_INDEX            0x4
#define QSFP13_INDEX            0x3
#define QSFP14_INDEX            0x2
#define QSFP15_INDEX            0x1
#define QSFP16_INDEX            0x0

static const unsigned short cpld_address_list[] = {0x34, I2C_CLIENT_END};

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
    dev_info(&client->dev, "[SWPLD2]QSFP_RESET_REG: 0x%02x, 0x%02x\n", val0, val1);

    val0 = cpld_i2c_read(data, QSFP_INITMOD_REG0);
    val1 = cpld_i2c_read(data, QSFP_INITMOD_REG1);
    dev_info(&client->dev, "[SWPLD2]QSFP_LPMODE_REG: 0x%02x, 0x%02x\n", val0, val1);
    
    val0 = cpld_i2c_read(data, QSFP_MODSEL_REG0);
    val1 = cpld_i2c_read(data, QSFP_MODSEL_REG1);
    dev_info(&client->dev, "[SWPLD2]QSFP_MODSEL_REG: 0x%02x, 0x%02x\n", val0, val1);
    
    val0 = cpld_i2c_read(data, QSFP_MODPRS_REG0);    
    val1 = cpld_i2c_read(data, QSFP_MODPRS_REG1);
    dev_info(&client->dev, "[SWPLD2]QSFP_MODPRES_REG: 0x%02x, 0x%02x\n", val0, val1);

}

static ssize_t show_code_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_REV_REG);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_sync(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, SYNC_REG);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_sync(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, SYNC_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SYNC_REG, (reg_val | usr_val));

    return count;
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
static SENSOR_DEVICE_ATTR(sync_clk_sel0, S_IRUGO | S_IWUSR, show_sync, set_sync, SYNC_REG_CLK_SEL0);
static SENSOR_DEVICE_ATTR(sync_clk_sel1, S_IRUGO | S_IWUSR, show_sync, set_sync, SYNC_REG_CLK_SEL1);
static SENSOR_DEVICE_ATTR(sync_cpld, S_IRUGO, show_sync, NULL, SYNC_REG_SYNCE_CPLD);
static SENSOR_DEVICE_ATTR(sync_presence, S_IRUGO, show_sync, NULL, SYNC_REG_PRESENCE);
static SENSOR_DEVICE_ATTR(led_test_amb, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_AMB);
static SENSOR_DEVICE_ATTR(led_test_grn, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_GRN);
static SENSOR_DEVICE_ATTR(led_test_blink, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_BLINK);
static SENSOR_DEVICE_ATTR(led_test_src_sel, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_SRC_SEL);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);
static SENSOR_DEVICE_ATTR(rst_pld_soft, S_IRUGO | S_IWUSR, show_rst, set_rst, RST_REG_PLD_SOFT_RST);
static SENSOR_DEVICE_ATTR(qsfp1_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst0, set_qsfp_rst0, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_rstn, S_IRUGO | S_IWUSR, show_qsfp_rst1, set_qsfp_rst1, QSFP16_INDEX);
static SENSOR_DEVICE_ATTR(qsfp1_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod0, set_qsfp_initmod0, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_lpmod, S_IRUGO | S_IWUSR, show_qsfp_initmod1, set_qsfp_initmod1, QSFP16_INDEX);
static SENSOR_DEVICE_ATTR(qsfp1_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel0, set_qsfp_modsel0, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_modsel, S_IRUGO | S_IWUSR, show_qsfp_modsel1, set_qsfp_modsel1, QSFP16_INDEX);
static SENSOR_DEVICE_ATTR(hitless_en, S_IRUGO, show_hitless, NULL, HITLESS_REG_EN);
static SENSOR_DEVICE_ATTR(qsfp1_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_prs, S_IRUGO, show_qsfp_modprs0, NULL, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_prs, S_IRUGO, show_qsfp_modprs1, NULL, QSFP16_INDEX);
static SENSOR_DEVICE_ATTR(modprs_reg1, S_IRUGO, show_modprs_reg, NULL, 1);
static SENSOR_DEVICE_ATTR(modprs_reg2, S_IRUGO, show_modprs_reg, NULL, 2);
static SENSOR_DEVICE_ATTR(qsfp1_int, S_IRUGO, show_qsfp_int0, NULL, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_int, S_IRUGO, show_qsfp_int0, NULL, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_int, S_IRUGO, show_qsfp_int0, NULL, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_int, S_IRUGO, show_qsfp_int0, NULL, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_int, S_IRUGO, show_qsfp_int0, NULL, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_int, S_IRUGO, show_qsfp_int0, NULL, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_int, S_IRUGO, show_qsfp_int0, NULL, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_int, S_IRUGO, show_qsfp_int0, NULL, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_int, S_IRUGO, show_qsfp_int1, NULL, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_int, S_IRUGO, show_qsfp_int1, NULL, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_int, S_IRUGO, show_qsfp_int1, NULL, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_int, S_IRUGO, show_qsfp_int1, NULL, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_int, S_IRUGO, show_qsfp_int1, NULL, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_int, S_IRUGO, show_qsfp_int1, NULL, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_int, S_IRUGO, show_qsfp_int1, NULL, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_int, S_IRUGO, show_qsfp_int1, NULL, QSFP16_INDEX);
static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp1_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 0);
static SENSOR_DEVICE_ATTR(qsfp2_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 1);
static SENSOR_DEVICE_ATTR(qsfp3_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 2);
static SENSOR_DEVICE_ATTR(qsfp4_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 3);
static SENSOR_DEVICE_ATTR(qsfp5_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 4);
static SENSOR_DEVICE_ATTR(qsfp6_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 5);
static SENSOR_DEVICE_ATTR(qsfp7_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 6);
static SENSOR_DEVICE_ATTR(qsfp8_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 7);
static SENSOR_DEVICE_ATTR(qsfp9_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 8);
static SENSOR_DEVICE_ATTR(qsfp10_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 9);
static SENSOR_DEVICE_ATTR(qsfp11_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 10);
static SENSOR_DEVICE_ATTR(qsfp12_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 11);
static SENSOR_DEVICE_ATTR(qsfp13_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 12);
static SENSOR_DEVICE_ATTR(qsfp14_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 13);
static SENSOR_DEVICE_ATTR(qsfp15_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 14);
static SENSOR_DEVICE_ATTR(qsfp16_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 15);

static SENSOR_DEVICE_ATTR(qsfp1_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 0);
static SENSOR_DEVICE_ATTR(qsfp2_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 1);
static SENSOR_DEVICE_ATTR(qsfp3_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 2);
static SENSOR_DEVICE_ATTR(qsfp4_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 3);
static SENSOR_DEVICE_ATTR(qsfp5_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 4);
static SENSOR_DEVICE_ATTR(qsfp6_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 5);
static SENSOR_DEVICE_ATTR(qsfp7_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 6);
static SENSOR_DEVICE_ATTR(qsfp8_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 7);
static SENSOR_DEVICE_ATTR(qsfp9_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 8);
static SENSOR_DEVICE_ATTR(qsfp10_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 9);
static SENSOR_DEVICE_ATTR(qsfp11_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 10);
static SENSOR_DEVICE_ATTR(qsfp12_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 11);
static SENSOR_DEVICE_ATTR(qsfp13_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 12);
static SENSOR_DEVICE_ATTR(qsfp14_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 13);
static SENSOR_DEVICE_ATTR(qsfp15_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 14);
static SENSOR_DEVICE_ATTR(qsfp16_led, S_IRUGO | S_IWUSR, show_qsfp_led, set_qsfp_led, 15);

static struct attribute *h4_32d_swpld2_attributes[] = {
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_sync_clk_sel0.dev_attr.attr,
    &sensor_dev_attr_sync_clk_sel1.dev_attr.attr,
    &sensor_dev_attr_sync_cpld.dev_attr.attr,
    &sensor_dev_attr_sync_presence.dev_attr.attr,
    &sensor_dev_attr_led_test_amb.dev_attr.attr,
    &sensor_dev_attr_led_test_grn.dev_attr.attr,
    &sensor_dev_attr_led_test_blink.dev_attr.attr,
    &sensor_dev_attr_led_test_src_sel.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_rst_pld_soft.dev_attr.attr,
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
    &sensor_dev_attr_qsfp1_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp2_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp3_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp4_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp5_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp6_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp7_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp8_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp9_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp10_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp11_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp12_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp13_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp14_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp15_modsel.dev_attr.attr,
    &sensor_dev_attr_qsfp16_modsel.dev_attr.attr,
    &sensor_dev_attr_hitless_en.dev_attr.attr,
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
    &sensor_dev_attr_modprs_reg1.dev_attr.attr,
    &sensor_dev_attr_modprs_reg2.dev_attr.attr,
    &sensor_dev_attr_qsfp1_int.dev_attr.attr,
    &sensor_dev_attr_qsfp2_int.dev_attr.attr,
    &sensor_dev_attr_qsfp3_int.dev_attr.attr,
    &sensor_dev_attr_qsfp4_int.dev_attr.attr,
    &sensor_dev_attr_qsfp5_int.dev_attr.attr,
    &sensor_dev_attr_qsfp6_int.dev_attr.attr,
    &sensor_dev_attr_qsfp7_int.dev_attr.attr,
    &sensor_dev_attr_qsfp8_int.dev_attr.attr,
    &sensor_dev_attr_qsfp9_int.dev_attr.attr,
    &sensor_dev_attr_qsfp10_int.dev_attr.attr,
    &sensor_dev_attr_qsfp11_int.dev_attr.attr,
    &sensor_dev_attr_qsfp12_int.dev_attr.attr,
    &sensor_dev_attr_qsfp13_int.dev_attr.attr,
    &sensor_dev_attr_qsfp14_int.dev_attr.attr,
    &sensor_dev_attr_qsfp15_int.dev_attr.attr,
    &sensor_dev_attr_qsfp16_int.dev_attr.attr,
    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,
    &sensor_dev_attr_qsfp1_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp2_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp3_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp4_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp5_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp6_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp7_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp8_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp9_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp10_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp11_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp12_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp13_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp14_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp15_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp16_reset.dev_attr.attr,

    &sensor_dev_attr_qsfp1_led.dev_attr.attr,
    &sensor_dev_attr_qsfp2_led.dev_attr.attr,
    &sensor_dev_attr_qsfp3_led.dev_attr.attr,
    &sensor_dev_attr_qsfp4_led.dev_attr.attr,
    &sensor_dev_attr_qsfp5_led.dev_attr.attr,
    &sensor_dev_attr_qsfp6_led.dev_attr.attr,
    &sensor_dev_attr_qsfp7_led.dev_attr.attr,
    &sensor_dev_attr_qsfp8_led.dev_attr.attr,
    &sensor_dev_attr_qsfp9_led.dev_attr.attr,
    &sensor_dev_attr_qsfp10_led.dev_attr.attr,
    &sensor_dev_attr_qsfp11_led.dev_attr.attr,
    &sensor_dev_attr_qsfp12_led.dev_attr.attr,
    &sensor_dev_attr_qsfp13_led.dev_attr.attr,
    &sensor_dev_attr_qsfp14_led.dev_attr.attr,
    &sensor_dev_attr_qsfp15_led.dev_attr.attr,
    &sensor_dev_attr_qsfp16_led.dev_attr.attr,
    NULL
};

static const struct attribute_group h4_32d_swpld2_group = {
    .attrs = h4_32d_swpld2_attributes,
};

static int h4_32d_swpld2_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;
    int i;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H4-32D SWPLD2 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &h4_32d_swpld2_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    dump_reg(data);
    dev_info(&client->dev, "[SWPLD2]Reseting QSFPs and SWPLD Registers...\n");
    cpld_i2c_write(data, QSFP_RST_REG0, 0x0);
    cpld_i2c_write(data, QSFP_RST_REG1, 0x0);
    cpld_i2c_write(data, QSFP_INITMOD_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_INITMOD_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG0, 0x0);
    cpld_i2c_write(data, QSFP_MODSEL_REG1, 0x0);
    msleep(2000);
    cpld_i2c_write(data, QSFP_RST_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG1, 0xFF);
    dev_info(&client->dev, "[SWPLD2]QSFPs and SWPLD Registers reset done.\n");
    dump_reg(data);
    for (i=0;i<16;i++) data->reset_list[i] = 0;

    return 0;

exit:
    return status;
}

static void h4_32d_swpld2_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &h4_32d_swpld2_group);
    kfree(data);
}

static const struct of_device_id h4_32d_swpld2_of_ids[] = {
    {
        .compatible = "nokia,h4_32d_swpld2",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, h4_32d_swpld2_of_ids);

static const struct i2c_device_id h4_32d_swpld2_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, h4_32d_swpld2_ids);

static struct i2c_driver h4_32d_swpld2_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(h4_32d_swpld2_of_ids),
    },
    .probe        = h4_32d_swpld2_probe,
    .remove       = h4_32d_swpld2_remove,
    .id_table     = h4_32d_swpld2_ids,
    .address_list = cpld_address_list,
};

static int __init h4_32d_swpld2_init(void)
{
    return i2c_add_driver(&h4_32d_swpld2_driver);
}

static void __exit h4_32d_swpld2_exit(void)
{
    i2c_del_driver(&h4_32d_swpld2_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H4-32D CPLD driver");
MODULE_LICENSE("GPL");

module_init(h4_32d_swpld2_init);
module_exit(h4_32d_swpld2_exit);
