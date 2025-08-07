//  * CPLD driver for Nokia-7220-IXR-D4
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

#define DRIVER_NAME "d4_cpld1"

// REGISTERS ADDRESS MAP
#define BOARD_INFO_REG          0x00
#define CPLD1_VER_REG           0x01
#define POWER_STATUS_REG1       0x02
#define POWER_STATUS_REG2       0x03

#define SYSTEM_RST_REG1         0x04
#define SYSTEM_RST_REG2         0x05
#define SYSTEM_RST_REG3         0x06
#define SYSTEM_RST_REG4         0x07
#define SYSTEM_RST_REG5         0x08
#define SYSTEM_RST_REG6         0x09

#define MOD_PRSNT_REG1          0x1F
#define MOD_PRSNT_REG2          0x20
#define MOD_PRSNT_REG3          0x21
#define MOD_PRSNT_REG4          0x22
#define MOD_PRSNT_REG5          0x23
#define MISC2_REG               0x27
#define SYSTEM_LED_REG1         0x59
#define SYSTEM_LED_REG2         0x60

// REG BIT FIELD POSITION or MASK
#define BOARD_INFO_REG_PCB_VER_MSK            0x3

#define PS1_PRESENT            0x0
#define PS2_PRESENT            0x1
#define PS1_PW_OK              0x2
#define PS2_PW_OK              0x3
#define PS1_ACDC_I_OK          0x4
#define PS2_ACDC_I_OK          0x5

#define PS1_POWER_ON           0x0
#define PS2_POWER_ON           0x1

#define I2C_SW1_RST            0x0
#define I2C_SW2_RST            0x1
#define I2C_SW3_RST            0x2
#define I2C_SW4_RST            0x3
#define I2C_SW5_RST            0x4
#define I2C_SW6_RST            0x5
#define I2C_SW7_RST            0x6

#define PSU1_LED_MASK          0x0
#define PSU2_LED_MASK          0x2
#define FAN_LED_MASK           0x4
#define SYSTEM_LED_MASK        0x4

#define I2C_MUX1_S             0x0
#define I2C_MUX2_S             0x1
#define JTAG_BUS_SEL           0x2
#define JTAG_SW_SEL            0x3
#define JTAG_SW_OE             0x4
#define EPROM_WP               0x5
#define BCM81356_SPI_WP        0x6

static const unsigned short cpld1_address_list[] = {0x60, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int reset_list[36];
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

/* ---------------- */

static void dump_reg(struct cpld_data *data)
{
    struct i2c_client *client = data->client;
    u8 val0 = 0;
    u8 val1 = 0;
    u8 val2 = 0;
    u8 val3 = 0;
    u8 val4 = 0;

    val0 = cpld_i2c_read(data, SYSTEM_RST_REG1);
    val1 = cpld_i2c_read(data, SYSTEM_RST_REG2);
    val2 = cpld_i2c_read(data, SYSTEM_RST_REG3);
    val3 = cpld_i2c_read(data, SYSTEM_RST_REG4);
    val4 = cpld_i2c_read(data, SYSTEM_RST_REG5);
    dev_info(&client->dev, "[CPLD1]QSFP_RESET_REG: 0x%02x, 0x%02x, 0x%02x,"
                           "0x%02x, 0x%02x\n",
                           val0, val1, val2, val3, val4);
}

/* ---------------- */

static ssize_t show_pcb_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *str_ver = NULL;
    u8 val = 0;
    val = cpld_i2c_read(data, BOARD_INFO_REG) & BOARD_INFO_REG_PCB_VER_MSK;

    switch (val) {
    case 0x0:
        str_ver = "R0A";
        break;
    case 0x1:
        str_ver = "R0B";
        break;
    case 0x2:
        str_ver = "R01";
        break;
    default:
        str_ver = "Reserved";
        break;
    }

    return sprintf(buf, "0x%x %s\n", val, str_ver);
}

/* ---------------- */

static ssize_t show_cpld_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, CPLD1_VER_REG);
    return sprintf(buf, "0x%02x\n", val);
}

/* ---------------- */

static ssize_t show_power_module1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, POWER_STATUS_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_power_module2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, POWER_STATUS_REG2);

    /* If the bit is set, power supply is disabled. */
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_power_module2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, POWER_STATUS_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, POWER_STATUS_REG2, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_rst1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_RST_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_system_rst1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_RST_REG1, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_rst2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_RST_REG2);

    /* If the bit is set, power supply is disabled. */
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_system_rst2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_RST_REG2, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_rst3(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_RST_REG3);

    /* If the bit is set, power supply is disabled. */
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_system_rst3(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG3);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_RST_REG3, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_rst4(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_RST_REG4);

    /* If the bit is set, power supply is disabled. */
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_system_rst4(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG4);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_RST_REG4, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_rst5(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_RST_REG5);

    /* If the bit is set, power supply is disabled. */
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_system_rst5(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG5);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_RST_REG5, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_rst6(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_RST_REG6);

    /* If the bit is set, power supply is disabled. */
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_system_rst6(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG6);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_RST_REG6, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_module_prsnt1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, MOD_PRSNT_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_module_prsnt2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, MOD_PRSNT_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_module_prsnt3(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, MOD_PRSNT_REG3);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_module_prsnt4(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, MOD_PRSNT_REG4);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_module_prsnt5(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, MOD_PRSNT_REG5);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_misc2(struct device *dev, struct device_attribute *devattr, char *buf)
{
     struct cpld_data *data = dev_get_drvdata(dev);
     struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
     u8 val = 0;
     val = cpld_i2c_read(data, MISC2_REG);

     return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_misc2(struct device *dev, struct device_attribute *devattr,  const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    mask = (~(0x1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, MISC2_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, MISC2_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_led1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_LED_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x3);
}

static ssize_t set_system_led1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 3) {
        return -EINVAL;
    }

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_LED_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_LED_REG1, (reg_val|usr_val));

    return count;
}

static ssize_t show_system_led2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_LED_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0xF);
}

static ssize_t set_system_led2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, usr_val=0, mask;
    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 15) {
        return -EINVAL;
    }

    mask = (~(0xF << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYSTEM_LED_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, SYSTEM_LED_REG2, (reg_val|usr_val));

    return count;
}

/* ---------------- */

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

/* ---------------- */

static ssize_t set_bulk_qsfp_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 usr_val = 0, reg_val = 0;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG4);
    switch(usr_val){
        case 0:
            cpld_i2c_write(data, SYSTEM_RST_REG1, 0x00);
            cpld_i2c_write(data, SYSTEM_RST_REG2, 0x00);
            cpld_i2c_write(data, SYSTEM_RST_REG3, 0x00);
            cpld_i2c_write(data, SYSTEM_RST_REG4, reg_val & 0xF0);
            cpld_i2c_write(data, SYSTEM_RST_REG5, 0x00);
            break;
        case 1:
            cpld_i2c_write(data, SYSTEM_RST_REG1, 0xFF);
            cpld_i2c_write(data, SYSTEM_RST_REG2, 0xFF);
            cpld_i2c_write(data, SYSTEM_RST_REG3, 0xFF);
            cpld_i2c_write(data, SYSTEM_RST_REG4, reg_val | 0x0F);
            cpld_i2c_write(data, SYSTEM_RST_REG5, 0xFF);
            for (int i=0;i<36;i++) data->reset_list[i] = 0;
    }
    dump_reg(data);
    return count;
}

static ssize_t show_bulk_qsfp_reset(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);

    u8 val0 = 0;
    u8 val1 = 0;
    u8 val2 = 0;
    u8 val3 = 0;
    u8 val4 = 0;

    val0 = cpld_i2c_read(data, SYSTEM_RST_REG1);
    val1 = cpld_i2c_read(data, SYSTEM_RST_REG2);
    val2 = cpld_i2c_read(data, SYSTEM_RST_REG3);
    val3 = cpld_i2c_read(data, SYSTEM_RST_REG4);
    val4 = cpld_i2c_read(data, SYSTEM_RST_REG5);

    return sprintf(buf, "%d %d %d %d %d\n", val0, val1, val2, val3, val4);
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(pcb_ver, S_IRUGO, show_pcb_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_ver, S_IRUGO, show_cpld_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(psu1_present, S_IRUGO, show_power_module1, NULL, PS1_PRESENT);
static SENSOR_DEVICE_ATTR(psu2_present, S_IRUGO, show_power_module1, NULL, PS2_PRESENT);
static SENSOR_DEVICE_ATTR(psu1_pwr_ok, S_IRUGO, show_power_module1, NULL, PS1_PW_OK);
static SENSOR_DEVICE_ATTR(psu2_pwr_ok, S_IRUGO, show_power_module1, NULL, PS2_PW_OK);
static SENSOR_DEVICE_ATTR(psu1_input_ok, S_IRUGO, show_power_module1, NULL, PS1_ACDC_I_OK);
static SENSOR_DEVICE_ATTR(psu2_input_ok, S_IRUGO, show_power_module1, NULL, PS2_ACDC_I_OK);
static SENSOR_DEVICE_ATTR(psu1_power_ok, S_IRUGO | S_IWUSR, show_power_module2, set_power_module2, PS1_POWER_ON);
static SENSOR_DEVICE_ATTR(psu2_power_ok, S_IRUGO | S_IWUSR, show_power_module2, set_power_module2, PS2_POWER_ON);
static SENSOR_DEVICE_ATTR(qsfp1_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 0);
static SENSOR_DEVICE_ATTR(qsfp2_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 1);
static SENSOR_DEVICE_ATTR(qsfp3_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 2);
static SENSOR_DEVICE_ATTR(qsfp4_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 3);
static SENSOR_DEVICE_ATTR(qsfp5_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 4);
static SENSOR_DEVICE_ATTR(qsfp6_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 5);
static SENSOR_DEVICE_ATTR(qsfp7_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 6);
static SENSOR_DEVICE_ATTR(qsfp8_rstn,  S_IRUGO | S_IWUSR, show_system_rst1, set_system_rst1, 7);
static SENSOR_DEVICE_ATTR(qsfp9_rstn,  S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 0);
static SENSOR_DEVICE_ATTR(qsfp10_rstn, S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 1);
static SENSOR_DEVICE_ATTR(qsfp11_rstn, S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 2);
static SENSOR_DEVICE_ATTR(qsfp12_rstn, S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 3);
static SENSOR_DEVICE_ATTR(qsfp13_rstn, S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 4);
static SENSOR_DEVICE_ATTR(qsfp14_rstn, S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 5);
static SENSOR_DEVICE_ATTR(qsfp15_rstn, S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 6);
static SENSOR_DEVICE_ATTR(qsfp16_rstn, S_IRUGO | S_IWUSR, show_system_rst2, set_system_rst2, 7);
static SENSOR_DEVICE_ATTR(qsfp17_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 0);
static SENSOR_DEVICE_ATTR(qsfp18_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 1);
static SENSOR_DEVICE_ATTR(qsfp19_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 2);
static SENSOR_DEVICE_ATTR(qsfp20_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 3);
static SENSOR_DEVICE_ATTR(qsfp21_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 4);
static SENSOR_DEVICE_ATTR(qsfp22_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 5);
static SENSOR_DEVICE_ATTR(qsfp23_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 6);
static SENSOR_DEVICE_ATTR(qsfp24_rstn, S_IRUGO | S_IWUSR, show_system_rst3, set_system_rst3, 7);
static SENSOR_DEVICE_ATTR(qsfp25_rstn, S_IRUGO | S_IWUSR, show_system_rst4, set_system_rst4, 0);
static SENSOR_DEVICE_ATTR(qsfp26_rstn, S_IRUGO | S_IWUSR, show_system_rst4, set_system_rst4, 1);
static SENSOR_DEVICE_ATTR(qsfp27_rstn, S_IRUGO | S_IWUSR, show_system_rst4, set_system_rst4, 2);
static SENSOR_DEVICE_ATTR(qsfp28_rstn, S_IRUGO | S_IWUSR, show_system_rst4, set_system_rst4, 3);
static SENSOR_DEVICE_ATTR(qsfp29_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 0);
static SENSOR_DEVICE_ATTR(qsfp30_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 1);
static SENSOR_DEVICE_ATTR(qsfp31_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 2);
static SENSOR_DEVICE_ATTR(qsfp32_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 3);
static SENSOR_DEVICE_ATTR(qsfp33_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 4);
static SENSOR_DEVICE_ATTR(qsfp34_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 5);
static SENSOR_DEVICE_ATTR(qsfp35_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 6);
static SENSOR_DEVICE_ATTR(qsfp36_rstn, S_IRUGO | S_IWUSR, show_system_rst5, set_system_rst5, 7);
static SENSOR_DEVICE_ATTR(i2c_sw1_rstn, S_IRUGO | S_IWUSR, show_system_rst6, set_system_rst6, I2C_SW1_RST);
static SENSOR_DEVICE_ATTR(i2c_sw2_rstn, S_IRUGO | S_IWUSR, show_system_rst6, set_system_rst6, I2C_SW2_RST);
static SENSOR_DEVICE_ATTR(i2c_sw3_rstn, S_IRUGO | S_IWUSR, show_system_rst6, set_system_rst6, I2C_SW3_RST);
static SENSOR_DEVICE_ATTR(i2c_sw4_rstn, S_IRUGO | S_IWUSR, show_system_rst6, set_system_rst6, I2C_SW4_RST);
static SENSOR_DEVICE_ATTR(i2c_sw5_rstn, S_IRUGO | S_IWUSR, show_system_rst6, set_system_rst6, I2C_SW5_RST);
static SENSOR_DEVICE_ATTR(i2c_sw6_rstn, S_IRUGO | S_IWUSR, show_system_rst6, set_system_rst6, I2C_SW6_RST);
static SENSOR_DEVICE_ATTR(i2c_sw7_rstn, S_IRUGO | S_IWUSR, show_system_rst6, set_system_rst6, I2C_SW7_RST);
static SENSOR_DEVICE_ATTR(qsfp1_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp2_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 1);
static SENSOR_DEVICE_ATTR(qsfp3_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 2);
static SENSOR_DEVICE_ATTR(qsfp4_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 3);
static SENSOR_DEVICE_ATTR(qsfp5_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 4);
static SENSOR_DEVICE_ATTR(qsfp6_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 5);
static SENSOR_DEVICE_ATTR(qsfp7_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 6);
static SENSOR_DEVICE_ATTR(qsfp8_mod_prsnt, S_IRUGO, show_module_prsnt2, NULL, 7);
static SENSOR_DEVICE_ATTR(qsfp9_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp10_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 1);
static SENSOR_DEVICE_ATTR(qsfp11_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 2);
static SENSOR_DEVICE_ATTR(qsfp12_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 3);
static SENSOR_DEVICE_ATTR(qsfp13_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 4);
static SENSOR_DEVICE_ATTR(qsfp14_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 5);
static SENSOR_DEVICE_ATTR(qsfp15_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 6);
static SENSOR_DEVICE_ATTR(qsfp16_mod_prsnt, S_IRUGO, show_module_prsnt3, NULL, 7);
static SENSOR_DEVICE_ATTR(qsfp17_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp18_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 1);
static SENSOR_DEVICE_ATTR(qsfp19_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 2);
static SENSOR_DEVICE_ATTR(qsfp20_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 3);
static SENSOR_DEVICE_ATTR(qsfp21_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 4);
static SENSOR_DEVICE_ATTR(qsfp22_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 5);
static SENSOR_DEVICE_ATTR(qsfp23_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 6);
static SENSOR_DEVICE_ATTR(qsfp24_mod_prsnt, S_IRUGO, show_module_prsnt4, NULL, 7);
static SENSOR_DEVICE_ATTR(qsfp25_mod_prsnt, S_IRUGO, show_module_prsnt5, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp26_mod_prsnt, S_IRUGO, show_module_prsnt5, NULL, 1);
static SENSOR_DEVICE_ATTR(qsfp27_mod_prsnt, S_IRUGO, show_module_prsnt5, NULL, 2);
static SENSOR_DEVICE_ATTR(qsfp28_mod_prsnt, S_IRUGO, show_module_prsnt5, NULL, 3);
static SENSOR_DEVICE_ATTR(qsfp29_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp30_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 1);
static SENSOR_DEVICE_ATTR(qsfp31_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 2);
static SENSOR_DEVICE_ATTR(qsfp32_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 3);
static SENSOR_DEVICE_ATTR(qsfp33_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 4);
static SENSOR_DEVICE_ATTR(qsfp34_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 5);
static SENSOR_DEVICE_ATTR(qsfp35_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 6);
static SENSOR_DEVICE_ATTR(qsfp36_mod_prsnt, S_IRUGO, show_module_prsnt1, NULL, 7);
static SENSOR_DEVICE_ATTR(i2c_mux1_sel, S_IRUGO | S_IWUSR, show_misc2, set_misc2, I2C_MUX1_S);
static SENSOR_DEVICE_ATTR(i2c_mux2_sel, S_IRUGO | S_IWUSR, show_misc2, set_misc2, I2C_MUX2_S);
static SENSOR_DEVICE_ATTR(jtag_bus_sel, S_IRUGO | S_IWUSR, show_misc2, set_misc2, JTAG_BUS_SEL);
static SENSOR_DEVICE_ATTR(jtag_sw_sel, S_IRUGO | S_IWUSR, show_misc2, set_misc2, JTAG_SW_SEL);
static SENSOR_DEVICE_ATTR(jtag_sw_oe, S_IRUGO | S_IWUSR, show_misc2, set_misc2, JTAG_SW_OE);
static SENSOR_DEVICE_ATTR(eeprom_wp, S_IRUGO | S_IWUSR, show_misc2, set_misc2, EPROM_WP);
static SENSOR_DEVICE_ATTR(bcm_spi_wp, S_IRUGO | S_IWUSR, show_misc2, set_misc2, BCM81356_SPI_WP);
static SENSOR_DEVICE_ATTR(psu1_led, S_IRUGO | S_IWUSR, show_system_led1, set_system_led1, PSU1_LED_MASK);
static SENSOR_DEVICE_ATTR(psu2_led, S_IRUGO | S_IWUSR, show_system_led1, set_system_led1, PSU2_LED_MASK);
static SENSOR_DEVICE_ATTR(fan_led, S_IRUGO | S_IWUSR, show_system_led1, set_system_led1, FAN_LED_MASK);
static SENSOR_DEVICE_ATTR(sys_led, S_IRUGO | S_IWUSR, show_system_led2, set_system_led2, SYSTEM_LED_MASK);
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
static SENSOR_DEVICE_ATTR(qsfp17_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 16);
static SENSOR_DEVICE_ATTR(qsfp18_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 17);
static SENSOR_DEVICE_ATTR(qsfp19_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 18);
static SENSOR_DEVICE_ATTR(qsfp20_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 19);
static SENSOR_DEVICE_ATTR(qsfp21_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 20);
static SENSOR_DEVICE_ATTR(qsfp22_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 21);
static SENSOR_DEVICE_ATTR(qsfp23_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 22);
static SENSOR_DEVICE_ATTR(qsfp24_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 23);
static SENSOR_DEVICE_ATTR(qsfp25_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 24);
static SENSOR_DEVICE_ATTR(qsfp26_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 25);
static SENSOR_DEVICE_ATTR(qsfp27_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 26);
static SENSOR_DEVICE_ATTR(qsfp28_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 27);
static SENSOR_DEVICE_ATTR(qsfp29_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 28);
static SENSOR_DEVICE_ATTR(qsfp30_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 29);
static SENSOR_DEVICE_ATTR(qsfp31_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 30);
static SENSOR_DEVICE_ATTR(qsfp32_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 31);
static SENSOR_DEVICE_ATTR(qsfp33_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 32);
static SENSOR_DEVICE_ATTR(qsfp34_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 33);
static SENSOR_DEVICE_ATTR(qsfp35_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 34);
static SENSOR_DEVICE_ATTR(qsfp36_reset, S_IRUGO | S_IWUSR, show_qsfp_reset, set_qsfp_reset, 35);
static SENSOR_DEVICE_ATTR(bulk_qsfp_reset, S_IRUGO | S_IWUSR, show_bulk_qsfp_reset, set_bulk_qsfp_reset, 0);

static struct attribute *d4_cpld1_attributes[] = {
    &sensor_dev_attr_pcb_ver.dev_attr.attr,
    &sensor_dev_attr_cpld_ver.dev_attr.attr,
    &sensor_dev_attr_psu1_present.dev_attr.attr,
    &sensor_dev_attr_psu2_present.dev_attr.attr,
    &sensor_dev_attr_psu1_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_psu1_input_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_input_ok.dev_attr.attr,
    &sensor_dev_attr_psu1_power_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_power_ok.dev_attr.attr,
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
    &sensor_dev_attr_qsfp33_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp34_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp35_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp36_rstn.dev_attr.attr,
    &sensor_dev_attr_i2c_sw1_rstn.dev_attr.attr,
    &sensor_dev_attr_i2c_sw2_rstn.dev_attr.attr,
    &sensor_dev_attr_i2c_sw3_rstn.dev_attr.attr,
    &sensor_dev_attr_i2c_sw4_rstn.dev_attr.attr,
    &sensor_dev_attr_i2c_sw5_rstn.dev_attr.attr,
    &sensor_dev_attr_i2c_sw6_rstn.dev_attr.attr,
    &sensor_dev_attr_i2c_sw7_rstn.dev_attr.attr,
    &sensor_dev_attr_qsfp1_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp2_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp3_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp4_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp5_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp6_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp7_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp8_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp9_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp10_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp11_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp12_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp13_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp14_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp15_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp16_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp17_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp18_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp19_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp20_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp21_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp22_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp23_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp24_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp25_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp26_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp27_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp28_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp29_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp30_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp31_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp32_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp33_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp34_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp35_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_qsfp36_mod_prsnt.dev_attr.attr,
    &sensor_dev_attr_i2c_mux1_sel.dev_attr.attr,
    &sensor_dev_attr_i2c_mux2_sel.dev_attr.attr,
    &sensor_dev_attr_jtag_bus_sel.dev_attr.attr,
    &sensor_dev_attr_jtag_sw_sel.dev_attr.attr,
    &sensor_dev_attr_jtag_sw_oe.dev_attr.attr,
    &sensor_dev_attr_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_bcm_spi_wp.dev_attr.attr,
    &sensor_dev_attr_psu1_led.dev_attr.attr,
    &sensor_dev_attr_psu2_led.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
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
    &sensor_dev_attr_qsfp33_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp34_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp35_reset.dev_attr.attr,
    &sensor_dev_attr_qsfp36_reset.dev_attr.attr,
    &sensor_dev_attr_bulk_qsfp_reset.dev_attr.attr,
    NULL
};

static const struct attribute_group d4_cpld1_group = {
    .attrs = d4_cpld1_attributes,
};

static int d4_cpld1_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-D4 CPLD1 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD1 PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &d4_cpld1_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    return 0;

exit:
    return status;
}

static void d4_cpld1_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &d4_cpld1_group);
    kfree(data);
}

static const struct of_device_id d4_cpld1_of_ids[] = {
    {
        .compatible = "nokia,d4_cpld1",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, d4_cpld1_of_ids);

static const struct i2c_device_id d4_cpld1_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, d4_cpld1_ids);

static struct i2c_driver d4_cpld1_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(d4_cpld1_of_ids),
    },
    .probe        = d4_cpld1_probe,
    .remove       = d4_cpld1_remove,
    .id_table     = d4_cpld1_ids,
    .address_list = cpld1_address_list,
};

static int __init d4_cpld1_init(void)
{
    return i2c_add_driver(&d4_cpld1_driver);
}

static void __exit d4_cpld1_exit(void)
{
    i2c_del_driver(&d4_cpld1_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-D4 CPLD1 driver");
MODULE_LICENSE("GPL");

module_init(d4_cpld1_init);
module_exit(d4_cpld1_exit);
