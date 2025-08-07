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

#define DRIVER_NAME "d4_cpld3"

// REGISTERS ADDRESS MAP
#define CPLD3_VER_REG                   0x01
#define QSFP28_P18_11_LOOP_REG          0x40
#define QSFP28_P10_03_LOOP_REG          0x41
#define QSFP28_P02_01_LOOP_REG          0x42
#define QSFP28_P18_11_PRES_REG          0x43
#define QSFP28_P10_03_PRES_REG          0x44
#define QSFP28_P02_01_PRES_REG          0x45
#define QSFP28_P18_11_PORT_STATUS_REG   0x46
#define QSFP28_P10_03_PORT_STATUS_REG   0x47
#define QSFP28_P02_01_PORT_STATUS_REG   0x48
#define QSFP28_P08_01_LP_MODE_REG       0x60
#define QSFP28_P16_09_LP_MODE_REG       0x61
#define QSFP28_P24_17_LP_MODE_REG       0x62
#define QSFP28_P28_25_LP_MODE_REG       0x63
#define QSFPDD_P08_01_LP_MODE_REG       0x64

// REG BIT FIELD POSITION or MASK

#define QSFP28_P18       0x7
#define QSFP28_P17       0x6
#define QSFP28_P16       0x5
#define QSFP28_P15       0x4
#define QSFP28_P14       0x3
#define QSFP28_P13       0x2
#define QSFP28_P12       0x1
#define QSFP28_P11       0x0
#define QSFP28_P10       0x7
#define QSFP28_P09       0x6
#define QSFP28_P08       0x5
#define QSFP28_P07       0x4
#define QSFP28_P06       0x3
#define QSFP28_P05       0x2
#define QSFP28_P04       0x1
#define QSFP28_P03       0x0
#define QSFP28_P02       0x1
#define QSFP28_P01       0x0


static const unsigned short cpld3_address_list[] = {0x64, I2C_CLIENT_END};

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

/* ---------------- */

static void dump_reg(struct cpld_data *data)
{
    struct i2c_client *client = data->client;
    u8 val0 = 0;
    u8 val1 = 0;
    u8 val2 = 0;
    u8 val3 = 0;
    u8 val4 = 0;

    val0 = cpld_i2c_read(data, QSFP28_P08_01_LP_MODE_REG);
    val1 = cpld_i2c_read(data, QSFP28_P16_09_LP_MODE_REG);
    val2 = cpld_i2c_read(data, QSFP28_P24_17_LP_MODE_REG);
    val3 = cpld_i2c_read(data, QSFP28_P28_25_LP_MODE_REG);
    val4 = cpld_i2c_read(data, QSFPDD_P08_01_LP_MODE_REG);
    dev_info(&client->dev, "[CPLD3]QSFP_LPMODE_REG: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", val0, val1, val2, val3, val4);

}

/* ---------------- */

static ssize_t show_cpld_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, CPLD3_VER_REG);
    return sprintf(buf, "0x%02x\n", val);
}

/* ---------------- */

static ssize_t show_module_lo0(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P18_11_LOOP_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_lo0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P18_11_LOOP_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P18_11_LOOP_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_module_lo1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P10_03_LOOP_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_lo1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P10_03_LOOP_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P10_03_LOOP_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_module_lo2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P02_01_LOOP_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_lo2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P02_01_LOOP_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P02_01_LOOP_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_module_present0(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P18_11_PRES_REG);

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
    reg_val = cpld_i2c_read(data, QSFP28_P18_11_PRES_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P18_11_PRES_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_module_present1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P10_03_PRES_REG);

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
    reg_val = cpld_i2c_read(data, QSFP28_P10_03_PRES_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P10_03_PRES_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_module_present2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P02_01_PRES_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_module_present2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P02_01_PRES_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P02_01_PRES_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_port_status0(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P18_11_PORT_STATUS_REG);

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
    reg_val = cpld_i2c_read(data, QSFP28_P18_11_PORT_STATUS_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P18_11_PORT_STATUS_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_port_status1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P10_03_PORT_STATUS_REG);

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
    reg_val = cpld_i2c_read(data, QSFP28_P10_03_PORT_STATUS_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P10_03_PORT_STATUS_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_port_status2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P02_01_PORT_STATUS_REG);

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
    reg_val = cpld_i2c_read(data, QSFP28_P02_01_PORT_STATUS_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P02_01_PORT_STATUS_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_lp_mode0(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P08_01_LP_MODE_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_lp_mode0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P08_01_LP_MODE_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P08_01_LP_MODE_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_lp_mode1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P16_09_LP_MODE_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_lp_mode1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P16_09_LP_MODE_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P16_09_LP_MODE_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_lp_mode2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P24_17_LP_MODE_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_lp_mode2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P24_17_LP_MODE_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P24_17_LP_MODE_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_lp_mode3(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFP28_P28_25_LP_MODE_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_lp_mode3(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFP28_P28_25_LP_MODE_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFP28_P28_25_LP_MODE_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_lp_mode4(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, QSFPDD_P08_01_LP_MODE_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_lp_mode4(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, QSFPDD_P08_01_LP_MODE_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;

    cpld_i2c_write(data, QSFPDD_P08_01_LP_MODE_REG, (reg_val|usr_val));

    return count;
}

/* ---------------- */

static ssize_t set_bulk_qsfp28_lpmod(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 reg_val=0, usr_val=0;

    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    reg_val = cpld_i2c_read(data, QSFP28_P28_25_LP_MODE_REG);
    switch(usr_val){
        case 0:
            cpld_i2c_write(data, QSFP28_P08_01_LP_MODE_REG, 0x00);
            cpld_i2c_write(data, QSFP28_P16_09_LP_MODE_REG, 0x00);
            cpld_i2c_write(data, QSFP28_P24_17_LP_MODE_REG, 0x00);
            cpld_i2c_write(data, QSFP28_P28_25_LP_MODE_REG, reg_val & 0xF0);
            break;
        case 1:
            cpld_i2c_write(data, QSFP28_P08_01_LP_MODE_REG, 0xFF);
            cpld_i2c_write(data, QSFP28_P16_09_LP_MODE_REG, 0xFF);
            cpld_i2c_write(data, QSFP28_P24_17_LP_MODE_REG, 0xFF);
            cpld_i2c_write(data, QSFP28_P28_25_LP_MODE_REG, reg_val | 0x0F);
    }
    dump_reg(data);
    return count;
}

static ssize_t show_bulk_qsfp28_lpmod(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);

    u8 val0 = 0;
    u8 val1 = 0;
    u8 val2 = 0;
    u8 val3 = 0;

    val0 = cpld_i2c_read(data, QSFP28_P08_01_LP_MODE_REG);
    val1 = cpld_i2c_read(data, QSFP28_P16_09_LP_MODE_REG);
    val2 = cpld_i2c_read(data, QSFP28_P24_17_LP_MODE_REG);
    val3 = cpld_i2c_read(data, QSFP28_P28_25_LP_MODE_REG);

    return sprintf(buf, "%d %d %d %d\n", val0, val1, val2, val3);
}

static ssize_t set_bulk_qsfpdd_lpmod(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 reg_val=0, usr_val=0;

    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 1) {
        return -EINVAL;
    }

    switch(usr_val){
        case 0:
            cpld_i2c_write(data, QSFPDD_P08_01_LP_MODE_REG, 0x00);
            break;
        case 1:
            cpld_i2c_write(data, QSFPDD_P08_01_LP_MODE_REG, 0xFF);
    }
    dump_reg(data);
    return count;
}

static ssize_t show_bulk_qsfpdd_lpmod(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);

    u8 val0 = 0;
    val0 = cpld_i2c_read(data, QSFPDD_P08_01_LP_MODE_REG);

    return sprintf(buf, "%d\n", val0);
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(cpld_ver, S_IRUGO, show_cpld_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(qsfp18_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P18);
static SENSOR_DEVICE_ATTR(qsfp17_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P17);
static SENSOR_DEVICE_ATTR(qsfp16_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P16);
static SENSOR_DEVICE_ATTR(qsfp15_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P15);
static SENSOR_DEVICE_ATTR(qsfp14_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P14);
static SENSOR_DEVICE_ATTR(qsfp13_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P13);
static SENSOR_DEVICE_ATTR(qsfp12_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P12);
static SENSOR_DEVICE_ATTR(qsfp11_lo, S_IRUGO | S_IWUSR, show_module_lo0, set_module_lo0, QSFP28_P11);
static SENSOR_DEVICE_ATTR(qsfp10_lo, S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P10);
static SENSOR_DEVICE_ATTR(qsfp9_lo,  S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P09);
static SENSOR_DEVICE_ATTR(qsfp8_lo,  S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P08);
static SENSOR_DEVICE_ATTR(qsfp7_lo,  S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P07);
static SENSOR_DEVICE_ATTR(qsfp6_lo,  S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P06);
static SENSOR_DEVICE_ATTR(qsfp5_lo,  S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P05);
static SENSOR_DEVICE_ATTR(qsfp4_lo,  S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P04);
static SENSOR_DEVICE_ATTR(qsfp3_lo,  S_IRUGO | S_IWUSR, show_module_lo1, set_module_lo1, QSFP28_P03);
static SENSOR_DEVICE_ATTR(qsfp2_lo,  S_IRUGO | S_IWUSR, show_module_lo2, set_module_lo2, QSFP28_P02);
static SENSOR_DEVICE_ATTR(qsfp1_lo,  S_IRUGO | S_IWUSR, show_module_lo2, set_module_lo2, QSFP28_P01);
static SENSOR_DEVICE_ATTR(qsfp18_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P18);
static SENSOR_DEVICE_ATTR(qsfp17_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P17);
static SENSOR_DEVICE_ATTR(qsfp16_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P16);
static SENSOR_DEVICE_ATTR(qsfp15_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P15);
static SENSOR_DEVICE_ATTR(qsfp14_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P14);
static SENSOR_DEVICE_ATTR(qsfp13_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P13);
static SENSOR_DEVICE_ATTR(qsfp12_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P12);
static SENSOR_DEVICE_ATTR(qsfp11_prs, S_IRUGO | S_IWUSR, show_module_present0, set_module_present0, QSFP28_P11);
static SENSOR_DEVICE_ATTR(qsfp10_prs, S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P10);
static SENSOR_DEVICE_ATTR(qsfp9_prs,  S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P09);
static SENSOR_DEVICE_ATTR(qsfp8_prs,  S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P08);
static SENSOR_DEVICE_ATTR(qsfp7_prs,  S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P07);
static SENSOR_DEVICE_ATTR(qsfp6_prs,  S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P06);
static SENSOR_DEVICE_ATTR(qsfp5_prs,  S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P05);
static SENSOR_DEVICE_ATTR(qsfp4_prs,  S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P04);
static SENSOR_DEVICE_ATTR(qsfp3_prs,  S_IRUGO | S_IWUSR, show_module_present1, set_module_present1, QSFP28_P03);
static SENSOR_DEVICE_ATTR(qsfp2_prs,  S_IRUGO | S_IWUSR, show_module_present2, set_module_present2, QSFP28_P02);
static SENSOR_DEVICE_ATTR(qsfp1_prs,  S_IRUGO | S_IWUSR, show_module_present2, set_module_present2, QSFP28_P01);
static SENSOR_DEVICE_ATTR(qsfp18_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P18);
static SENSOR_DEVICE_ATTR(qsfp17_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P17);
static SENSOR_DEVICE_ATTR(qsfp16_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P16);
static SENSOR_DEVICE_ATTR(qsfp15_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P15);
static SENSOR_DEVICE_ATTR(qsfp14_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P14);
static SENSOR_DEVICE_ATTR(qsfp13_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P13);
static SENSOR_DEVICE_ATTR(qsfp12_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P12);
static SENSOR_DEVICE_ATTR(qsfp11_port, S_IRUGO | S_IWUSR, show_port_status0, set_port_status0, QSFP28_P11);
static SENSOR_DEVICE_ATTR(qsfp10_port, S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P10);
static SENSOR_DEVICE_ATTR(qsfp9_port,  S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P09);
static SENSOR_DEVICE_ATTR(qsfp8_port,  S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P08);
static SENSOR_DEVICE_ATTR(qsfp7_port,  S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P07);
static SENSOR_DEVICE_ATTR(qsfp6_port,  S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P06);
static SENSOR_DEVICE_ATTR(qsfp5_port,  S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P05);
static SENSOR_DEVICE_ATTR(qsfp4_port,  S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P04);
static SENSOR_DEVICE_ATTR(qsfp3_port,  S_IRUGO | S_IWUSR, show_port_status1, set_port_status1, QSFP28_P03);
static SENSOR_DEVICE_ATTR(qsfp2_port,  S_IRUGO | S_IWUSR, show_port_status2, set_port_status2, QSFP28_P02);
static SENSOR_DEVICE_ATTR(qsfp1_port,  S_IRUGO | S_IWUSR, show_port_status2, set_port_status2, QSFP28_P01);
static SENSOR_DEVICE_ATTR(qsfp1_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 0);
static SENSOR_DEVICE_ATTR(qsfp2_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 1);
static SENSOR_DEVICE_ATTR(qsfp3_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 2);
static SENSOR_DEVICE_ATTR(qsfp4_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 3);
static SENSOR_DEVICE_ATTR(qsfp5_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 4);
static SENSOR_DEVICE_ATTR(qsfp6_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 5);
static SENSOR_DEVICE_ATTR(qsfp7_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 6);
static SENSOR_DEVICE_ATTR(qsfp8_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode0, set_lp_mode0, 7);
static SENSOR_DEVICE_ATTR(qsfp9_lpmod,  S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 0);
static SENSOR_DEVICE_ATTR(qsfp10_lpmod, S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 1);
static SENSOR_DEVICE_ATTR(qsfp11_lpmod, S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 2);
static SENSOR_DEVICE_ATTR(qsfp12_lpmod, S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 3);
static SENSOR_DEVICE_ATTR(qsfp13_lpmod, S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 4);
static SENSOR_DEVICE_ATTR(qsfp14_lpmod, S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 5);
static SENSOR_DEVICE_ATTR(qsfp15_lpmod, S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 6);
static SENSOR_DEVICE_ATTR(qsfp16_lpmod, S_IRUGO | S_IWUSR, show_lp_mode1, set_lp_mode1, 7);
static SENSOR_DEVICE_ATTR(qsfp17_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 0);
static SENSOR_DEVICE_ATTR(qsfp18_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 1);
static SENSOR_DEVICE_ATTR(qsfp19_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 2);
static SENSOR_DEVICE_ATTR(qsfp20_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 3);
static SENSOR_DEVICE_ATTR(qsfp21_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 4);
static SENSOR_DEVICE_ATTR(qsfp22_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 5);
static SENSOR_DEVICE_ATTR(qsfp23_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 6);
static SENSOR_DEVICE_ATTR(qsfp24_lpmod, S_IRUGO | S_IWUSR, show_lp_mode2, set_lp_mode2, 7);
static SENSOR_DEVICE_ATTR(qsfp25_lpmod, S_IRUGO | S_IWUSR, show_lp_mode3, set_lp_mode3, 0);
static SENSOR_DEVICE_ATTR(qsfp26_lpmod, S_IRUGO | S_IWUSR, show_lp_mode3, set_lp_mode3, 1);
static SENSOR_DEVICE_ATTR(qsfp27_lpmod, S_IRUGO | S_IWUSR, show_lp_mode3, set_lp_mode3, 2);
static SENSOR_DEVICE_ATTR(qsfp28_lpmod, S_IRUGO | S_IWUSR, show_lp_mode3, set_lp_mode3, 3);
static SENSOR_DEVICE_ATTR(qsfp29_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 0);
static SENSOR_DEVICE_ATTR(qsfp30_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 1);
static SENSOR_DEVICE_ATTR(qsfp31_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 2);
static SENSOR_DEVICE_ATTR(qsfp32_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 3);
static SENSOR_DEVICE_ATTR(qsfp33_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 4);
static SENSOR_DEVICE_ATTR(qsfp34_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 5);
static SENSOR_DEVICE_ATTR(qsfp35_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 6);
static SENSOR_DEVICE_ATTR(qsfp36_lpmod, S_IRUGO | S_IWUSR, show_lp_mode4, set_lp_mode4, 7);
static SENSOR_DEVICE_ATTR(bulk_qsfp28_lpmod, S_IRUGO | S_IWUSR, show_bulk_qsfp28_lpmod, set_bulk_qsfp28_lpmod, 0);
static SENSOR_DEVICE_ATTR(bulk_qsfpdd_lpmod, S_IRUGO | S_IWUSR, show_bulk_qsfpdd_lpmod, set_bulk_qsfpdd_lpmod, 0);

static struct attribute *d4_cpld3_attributes[] = {
    &sensor_dev_attr_cpld_ver.dev_attr.attr,
    &sensor_dev_attr_qsfp18_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp17_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp16_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp15_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp14_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp13_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp12_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp11_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp10_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp9_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp8_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp7_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp6_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp5_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp4_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp3_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp2_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp1_lo.dev_attr.attr,
    &sensor_dev_attr_qsfp18_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp17_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp16_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp15_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp14_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp13_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp12_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp11_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp10_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp9_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp8_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp7_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp6_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp5_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp4_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp3_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp2_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp1_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp18_port.dev_attr.attr,
    &sensor_dev_attr_qsfp17_port.dev_attr.attr,
    &sensor_dev_attr_qsfp16_port.dev_attr.attr,
    &sensor_dev_attr_qsfp15_port.dev_attr.attr,
    &sensor_dev_attr_qsfp14_port.dev_attr.attr,
    &sensor_dev_attr_qsfp13_port.dev_attr.attr,
    &sensor_dev_attr_qsfp12_port.dev_attr.attr,
    &sensor_dev_attr_qsfp11_port.dev_attr.attr,
    &sensor_dev_attr_qsfp10_port.dev_attr.attr,
    &sensor_dev_attr_qsfp9_port.dev_attr.attr,
    &sensor_dev_attr_qsfp8_port.dev_attr.attr,
    &sensor_dev_attr_qsfp7_port.dev_attr.attr,
    &sensor_dev_attr_qsfp6_port.dev_attr.attr,
    &sensor_dev_attr_qsfp5_port.dev_attr.attr,
    &sensor_dev_attr_qsfp4_port.dev_attr.attr,
    &sensor_dev_attr_qsfp3_port.dev_attr.attr,
    &sensor_dev_attr_qsfp2_port.dev_attr.attr,
    &sensor_dev_attr_qsfp1_port.dev_attr.attr,
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
    &sensor_dev_attr_qsfp33_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp34_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp35_lpmod.dev_attr.attr,
    &sensor_dev_attr_qsfp36_lpmod.dev_attr.attr,
    &sensor_dev_attr_bulk_qsfp28_lpmod.dev_attr.attr,
    &sensor_dev_attr_bulk_qsfpdd_lpmod.dev_attr.attr,
    NULL
};

static const struct attribute_group d4_cpld3_group = {
    .attrs = d4_cpld3_attributes,
};

static int d4_cpld3_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-D4 CPLD3 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD3 PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &d4_cpld3_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    return 0;

exit:
    return status;
}

static void d4_cpld3_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &d4_cpld3_group);
    kfree(data);
}

static const struct of_device_id d4_cpld3_of_ids[] = {
    {
        .compatible = "nokia,d4_cpld3",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, d4_cpld3_of_ids);

static const struct i2c_device_id d4_cpld3_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, d4_cpld3_ids);

static struct i2c_driver d4_cpld3_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(d4_cpld3_of_ids),
    },
    .probe        = d4_cpld3_probe,
    .remove       = d4_cpld3_remove,
    .id_table     = d4_cpld3_ids,
    .address_list = cpld3_address_list,
};

static int __init d4_cpld3_init(void)
{
    return i2c_add_driver(&d4_cpld3_driver);
}

static void __exit d4_cpld3_exit(void)
{
    i2c_del_driver(&d4_cpld3_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-D4 CPLD3 driver");
MODULE_LICENSE("GPL");

module_init(d4_cpld3_init);
module_exit(d4_cpld3_exit);
