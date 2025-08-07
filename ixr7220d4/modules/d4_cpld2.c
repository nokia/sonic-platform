
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

#define DRIVER_NAME "d4_cpld2"

#define CPLD2_VER_REG                     0x01
#define CPLD2_INTERRUPT_REG               0x02

#define QSPFPDD_P08_01_LOOP_REG           0x40
#define QSPFP28_P28_21_LOOP_REG           0x41
#define QSPFP28_P20_19_LOOP_REG           0x42
#define QSPFPDD_P08_01_PRES_REG           0x43
#define QSPFP28_P28_21_PRES_REG           0x44
#define QSPFP28_P20_19_PRES_REG           0x45
#define QSPFPDD_P08_01_PORT_STATUS_REG    0x46
#define QSPFP28_P28_21_PORT_STATUS_REG    0x47
#define QSPFP28_P20_19_PORT_STATUS_REG    0x48

#define QSPFP28_P20      0x1
#define QSPFP28_P19      0x0

#define QSPFP28_P28      0x7
#define QSPFP28_P27      0x6
#define QSPFP28_P26      0x5
#define QSPFP28_P25      0x4
#define QSPFP28_P24      0x3
#define QSPFP28_P23      0x2
#define QSPFP28_P22      0x1
#define QSPFP28_P21      0x0

#define QSFPDD_P8       0x7
#define QSFPDD_P7       0x6
#define QSFPDD_P6       0x5
#define QSFPDD_P5       0x4
#define QSFPDD_P4       0x3
#define QSFPDD_P3       0x2
#define QSFPDD_P2       0x1
#define QSFPDD_P1       0x0


static const unsigned short cpld2_address_list[] = {0x62, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex update_lock;
};

static int cpld_i2c_read(struct cpld_data *data, u8 reg)
{
    int val = 0;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
         dev_err(&client->dev, "CPLD2 READ ERROR: reg(0x%02x) err %d\n", reg, val);
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
        dev_err(&client->dev, "CPLD2 WRITE ERROR: reg(0x%02x) err %d\n", reg, res);
    }
    mutex_unlock(&data->update_lock);
}

static ssize_t show_cpld_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, CPLD2_VER_REG);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_module_lo0(struct device *dev, struct device_attribute *devattr, char *buf)
{
     struct cpld_data *data = dev_get_drvdata(dev);
     struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
     u8 val = 0;
     val = cpld_i2c_read(data,QSPFPDD_P08_01_LOOP_REG );

     return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_lo0(struct device *dev, struct device_attribute *devattr,  const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFPDD_P08_01_LOOP_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFPDD_P08_01_LOOP_REG, (reg_val|usr_val));

    return count;
}

static ssize_t show_module_lo1(struct device *dev, struct device_attribute *devattr, char *buf)
{
     struct cpld_data *data = dev_get_drvdata(dev);
     struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
     u8 val = 0;
     val = cpld_i2c_read(data,QSPFP28_P28_21_LOOP_REG );

     return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_lo1(struct device *dev, struct device_attribute *devattr,  const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFP28_P28_21_LOOP_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFP28_P28_21_LOOP_REG, (reg_val|usr_val));

    return count;
}

static ssize_t show_module_lo2(struct device *dev, struct device_attribute *devattr, char *buf)
{
     struct cpld_data *data = dev_get_drvdata(dev);
     struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
     u8 val = 0;
     val = cpld_i2c_read(data, QSPFP28_P20_19_LOOP_REG);

     return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_lo2(struct device *dev, struct device_attribute *devattr,  const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFP28_P20_19_LOOP_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFP28_P20_19_LOOP_REG, (reg_val|usr_val));

    return count;
}

static ssize_t show_module_present0(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSPFPDD_P08_01_PRES_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_present0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFPDD_P08_01_PRES_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFPDD_P08_01_PRES_REG, (reg_val|usr_val));

    return count;
}

static ssize_t show_module_present1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSPFP28_P28_21_PRES_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_present1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFP28_P28_21_PRES_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFP28_P28_21_PRES_REG, (reg_val|usr_val));

    return count;
}

static ssize_t show_module_present2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSPFP28_P20_19_PRES_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_present2(struct device *dev, struct device_attribute *devattr,const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFP28_P20_19_PRES_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFP28_P20_19_PRES_REG, (reg_val|usr_val));

    return count;
}

static ssize_t show_port_status0(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSPFPDD_P08_01_PORT_STATUS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_port_status0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFPDD_P08_01_PORT_STATUS_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFPDD_P08_01_PORT_STATUS_REG, (reg_val|usr_val));

    return count;
}

static ssize_t show_port_status1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSPFP28_P28_21_PORT_STATUS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_port_status1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFP28_P28_21_PORT_STATUS_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFP28_P28_21_PORT_STATUS_REG, (reg_val|usr_val));

    return count;
}


static ssize_t show_port_status2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSPFP28_P20_19_PORT_STATUS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_port_status2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSPFP28_P20_19_PORT_STATUS_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSPFP28_P20_19_PORT_STATUS_REG, (reg_val|usr_val));

    return count;
}


static SENSOR_DEVICE_ATTR(cpld_ver, S_IRUGO, show_cpld_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp36_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P8);
static SENSOR_DEVICE_ATTR(qsfp35_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P7);
static SENSOR_DEVICE_ATTR(qsfp34_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P6);
static SENSOR_DEVICE_ATTR(qsfp33_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P5);
static SENSOR_DEVICE_ATTR(qsfp32_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P4);
static SENSOR_DEVICE_ATTR(qsfp31_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P3);
static SENSOR_DEVICE_ATTR(qsfp30_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P2);
static SENSOR_DEVICE_ATTR(qsfp29_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFPDD_P1);
static SENSOR_DEVICE_ATTR(qsfp28_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P28);
static SENSOR_DEVICE_ATTR(qsfp27_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P27);
static SENSOR_DEVICE_ATTR(qsfp26_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P26);
static SENSOR_DEVICE_ATTR(qsfp25_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P25);
static SENSOR_DEVICE_ATTR(qsfp24_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P24);
static SENSOR_DEVICE_ATTR(qsfp23_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P23);
static SENSOR_DEVICE_ATTR(qsfp22_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P22);
static SENSOR_DEVICE_ATTR(qsfp21_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSPFP28_P21);
static SENSOR_DEVICE_ATTR(qsfp20_lo, S_IRUGO | S_IWUSR, show_module_lo2, set_module_lo2, QSPFP28_P20);
static SENSOR_DEVICE_ATTR(qsfp19_lo, S_IRUGO | S_IWUSR, show_module_lo2, set_module_lo2, QSPFP28_P19);
static SENSOR_DEVICE_ATTR(qsfp36_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P8);
static SENSOR_DEVICE_ATTR(qsfp35_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P7);
static SENSOR_DEVICE_ATTR(qsfp34_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P6);
static SENSOR_DEVICE_ATTR(qsfp33_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P5);
static SENSOR_DEVICE_ATTR(qsfp32_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P4);
static SENSOR_DEVICE_ATTR(qsfp31_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P3);
static SENSOR_DEVICE_ATTR(qsfp30_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P2);
static SENSOR_DEVICE_ATTR(qsfp29_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFPDD_P1);
static SENSOR_DEVICE_ATTR(qsfp28_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P28);
static SENSOR_DEVICE_ATTR(qsfp27_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P27);
static SENSOR_DEVICE_ATTR(qsfp26_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P26);
static SENSOR_DEVICE_ATTR(qsfp25_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P25);
static SENSOR_DEVICE_ATTR(qsfp24_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P24);
static SENSOR_DEVICE_ATTR(qsfp23_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P23);
static SENSOR_DEVICE_ATTR(qsfp22_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P22);
static SENSOR_DEVICE_ATTR(qsfp21_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSPFP28_P21);
static SENSOR_DEVICE_ATTR(qsfp20_prs, S_IRUGO | S_IWUSR, show_module_present2, set_module_present2, QSPFP28_P20);
static SENSOR_DEVICE_ATTR(qsfp19_prs, S_IRUGO | S_IWUSR, show_module_present2, set_module_present2, QSPFP28_P19);
static SENSOR_DEVICE_ATTR(qsfp36_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P8);
static SENSOR_DEVICE_ATTR(qsfp35_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P7);
static SENSOR_DEVICE_ATTR(qsfp34_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P6);
static SENSOR_DEVICE_ATTR(qsfp33_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P5);
static SENSOR_DEVICE_ATTR(qsfp32_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P4);
static SENSOR_DEVICE_ATTR(qsfp31_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P3);
static SENSOR_DEVICE_ATTR(qsfp30_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P2);
static SENSOR_DEVICE_ATTR(qsfp29_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFPDD_P1);
static SENSOR_DEVICE_ATTR(qsfp28_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P28);
static SENSOR_DEVICE_ATTR(qsfp27_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P27);
static SENSOR_DEVICE_ATTR(qsfp26_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P26);
static SENSOR_DEVICE_ATTR(qsfp25_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P25);
static SENSOR_DEVICE_ATTR(qsfp24_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P24);
static SENSOR_DEVICE_ATTR(qsfp23_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P23);
static SENSOR_DEVICE_ATTR(qsfp22_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P22);
static SENSOR_DEVICE_ATTR(qsfp21_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSPFP28_P21);
static SENSOR_DEVICE_ATTR(qsfp20_port, S_IRUGO | S_IWUSR, show_port_status2, set_port_status2, QSPFP28_P20);
static SENSOR_DEVICE_ATTR(qsfp19_port, S_IRUGO | S_IWUSR, show_port_status2, set_port_status2, QSPFP28_P19);


static struct attribute *d4_cpld2_attributes[] = {
    &sensor_dev_attr_cpld_ver.dev_attr.attr,
    &sensor_dev_attr_qsfp36_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp35_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp34_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp33_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp32_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp31_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp30_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp29_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp28_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp27_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp26_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp25_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp24_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp23_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp22_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp21_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp20_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp19_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp36_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp35_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp34_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp33_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp32_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp31_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp30_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp29_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp28_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp27_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp26_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp25_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp24_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp23_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp22_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp21_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp20_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp19_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp36_port.dev_attr.attr,
    &sensor_dev_attr_qsfp35_port.dev_attr.attr,
    &sensor_dev_attr_qsfp34_port.dev_attr.attr,
    &sensor_dev_attr_qsfp33_port.dev_attr.attr,
    &sensor_dev_attr_qsfp32_port.dev_attr.attr,
    &sensor_dev_attr_qsfp31_port.dev_attr.attr,
    &sensor_dev_attr_qsfp30_port.dev_attr.attr,
    &sensor_dev_attr_qsfp29_port.dev_attr.attr,
    &sensor_dev_attr_qsfp28_port.dev_attr.attr,
    &sensor_dev_attr_qsfp27_port.dev_attr.attr,
    &sensor_dev_attr_qsfp26_port.dev_attr.attr,
    &sensor_dev_attr_qsfp25_port.dev_attr.attr,
    &sensor_dev_attr_qsfp24_port.dev_attr.attr,
    &sensor_dev_attr_qsfp23_port.dev_attr.attr,
    &sensor_dev_attr_qsfp22_port.dev_attr.attr,
    &sensor_dev_attr_qsfp21_port.dev_attr.attr,
    &sensor_dev_attr_qsfp20_port.dev_attr.attr,
    &sensor_dev_attr_qsfp19_port.dev_attr.attr,
    NULL

};

static const struct attribute_group d4_cpld2_group = {
    .attrs = d4_cpld2_attributes,
};

static int d4_cpld2_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD2 PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-D4 CPLD2 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD2 PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &d4_cpld2_group);
    if (status) {
        dev_err(&client->dev, "CPLD2 INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    return 0;

exit:
    return status;
}

static void d4_cpld2_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &d4_cpld2_group);
    kfree(data);
}

static const struct of_device_id d4_cpld2_of_ids[] = {
    {
        .compatible = "nokia,d4_cpld2",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, d4_cpld2_of_ids);

static const struct i2c_device_id d4_cpld2_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, d4_cpld2_ids);


static struct i2c_driver d4_cpld2_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(d4_cpld2_of_ids),
    },
    .probe        = d4_cpld2_probe,
    .remove       = d4_cpld2_remove,
    .id_table     = d4_cpld2_ids,
    .address_list = cpld2_address_list,
};

static int __init d4_cpld2_init(void)
{
    return i2c_add_driver(&d4_cpld2_driver);
}

static void __exit d4_cpld2_exit(void)
{
    i2c_del_driver(&d4_cpld2_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-D4 CPLD2 driver");
MODULE_LICENSE("GPL");

module_init(d4_cpld2_init);
module_exit(d4_cpld2_exit);
