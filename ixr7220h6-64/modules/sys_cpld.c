//  * CPLD driver for Nokia-7220-IXR-H6-64 Router
//  *
//  * Copyright (C) 2025 Nokia Corporation.
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

#define DRIVER_NAME "sys_cpld"

// REGISTERS ADDRESS MAP
#define VER_MAJOR_REG           0x00
#define VER_MINOR_REG           0x01
#define SCRATCH_REG             0x04
#define PSU_GOOD_REG            0x0B
#define PSU_PRES_REG            0x0C
#define RST_GROUP1_REG          0x11
#define OSFP_EFUSE_REG0         0x70
#define SYS_LED_REG0            0x80
#define SYS_LED_REG1            0x81

// REG BIT FIELD POSITION or MASK

static const unsigned short cpld_address_list[] = {0x61, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int osfp_efuse;
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

static ssize_t show_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 reg_major = 0;
    u8 reg_minor = 0;

    reg_major = cpld_i2c_read(data, VER_MAJOR_REG);
    reg_minor = cpld_i2c_read(data, VER_MINOR_REG);

    return sprintf(buf, "%d.%d\n", reg_major, reg_minor);
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

static ssize_t show_psu_ok(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PSU_GOOD_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_psu_pres(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PSU_PRES_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_led0(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 mask = 0xF;

    val = cpld_i2c_read(data, SYS_LED_REG0);
    if (sda->index == 0) mask = 0xF;
    else mask = 0x3;
    return sprintf(buf, "0x%x\n", (val>>sda->index) & mask);
}

static ssize_t set_led0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 reg_mask = 0xFF;
    u8 mask = 0xF;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (sda->index == 0) mask = 0xF;
    else mask = 0x3;
    if (usr_val > mask) {
        return -EINVAL;
    }
    reg_mask = (~(mask << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYS_LED_REG0);
    reg_val = reg_val & reg_mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SYS_LED_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_led1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 mask = 0xF;

    val = cpld_i2c_read(data, SYS_LED_REG1);
    if (sda->index == 0) mask = 0xF;
    else mask = 0x3;
    return sprintf(buf, "0x%x\n", (val>>sda->index) & mask);
}

static ssize_t set_led1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 reg_mask = 0xFF;
    u8 mask = 0xF;

    int ret = kstrtou8(buf, 16, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (sda->index == 0) mask = 0xF;
    else mask = 0x3;
    if (usr_val > mask) {
        return -EINVAL;
    }
    reg_mask = (~(mask << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, SYS_LED_REG1);
    reg_val = reg_val & reg_mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SYS_LED_REG1, (reg_val | usr_val));

    return count;
}

static ssize_t show_rst1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, RST_GROUP1_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_rst1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, RST_GROUP1_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, RST_GROUP1_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_osfp_efuse(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    
    return sprintf(buf, "%s\n", (data->osfp_efuse) ? "Enabled":"Disabled");
}

static ssize_t set_osfp_efuse(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    int i;
    const char *str_en = "Enable\n";
    const char *str_dis = "Disable\n";
    
    if (strcmp(buf, str_en) == 0) {
        for (i=0;i<8;i++) cpld_i2c_write(data, OSFP_EFUSE_REG0+i, 0xFF);
        data->osfp_efuse = 1;
    }
    else if (strcmp(buf, str_dis) == 0) {
        for (i=0;i<8;i++) cpld_i2c_write(data, OSFP_EFUSE_REG0+i, 0x0);
        data->osfp_efuse = 0;
    }
    else
        return -EINVAL;

    return count;
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);

static SENSOR_DEVICE_ATTR(psu1_ok, S_IRUGO, show_psu_ok, NULL, 0);
static SENSOR_DEVICE_ATTR(psu2_ok, S_IRUGO, show_psu_ok, NULL, 1);
static SENSOR_DEVICE_ATTR(psu3_ok, S_IRUGO, show_psu_ok, NULL, 2);
static SENSOR_DEVICE_ATTR(psu4_ok, S_IRUGO, show_psu_ok, NULL, 3);
static SENSOR_DEVICE_ATTR(psu1_pres, S_IRUGO, show_psu_pres, NULL, 4);
static SENSOR_DEVICE_ATTR(psu2_pres, S_IRUGO, show_psu_pres, NULL, 5);
static SENSOR_DEVICE_ATTR(psu3_pres, S_IRUGO, show_psu_pres, NULL, 6);
static SENSOR_DEVICE_ATTR(psu4_pres, S_IRUGO, show_psu_pres, NULL, 7);

static SENSOR_DEVICE_ATTR(led_sys, S_IRUGO | S_IWUSR, show_led0, set_led0, 0);
static SENSOR_DEVICE_ATTR(led_psu, S_IRUGO, show_led0, NULL, 4);
static SENSOR_DEVICE_ATTR(led_loc, S_IRUGO | S_IWUSR, show_led1, set_led1, 0);
static SENSOR_DEVICE_ATTR(led_fan, S_IRUGO | S_IWUSR, show_led1, set_led1, 4);

static SENSOR_DEVICE_ATTR(mac_pcie_rst, S_IRUGO | S_IWUSR, show_rst1, set_rst1, 3);
static SENSOR_DEVICE_ATTR(osfp_efuse, S_IRUGO | S_IWUSR, show_osfp_efuse, set_osfp_efuse, 0);

static struct attribute *sys_cpld_attributes[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,

    &sensor_dev_attr_psu1_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_ok.dev_attr.attr,
    &sensor_dev_attr_psu3_ok.dev_attr.attr,
    &sensor_dev_attr_psu4_ok.dev_attr.attr,
    &sensor_dev_attr_psu1_pres.dev_attr.attr,
    &sensor_dev_attr_psu2_pres.dev_attr.attr,
    &sensor_dev_attr_psu3_pres.dev_attr.attr,
    &sensor_dev_attr_psu4_pres.dev_attr.attr,

    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_psu.dev_attr.attr,
    &sensor_dev_attr_led_loc.dev_attr.attr,
    &sensor_dev_attr_led_fan.dev_attr.attr,

    &sensor_dev_attr_mac_pcie_rst.dev_attr.attr,
    &sensor_dev_attr_osfp_efuse.dev_attr.attr,

    NULL
};

static const struct attribute_group sys_cpld_group = {
    .attrs = sys_cpld_attributes,
};

static int sys_cpld_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia SYS_CPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &sys_cpld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    int i;
    for (i=0;i<8;i++) cpld_i2c_write(data, OSFP_EFUSE_REG0+i, 0xFF);
    data->osfp_efuse = 1;

    return 0;

exit:
    return status;
}

static void sys_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &sys_cpld_group);
    kfree(data);
}

static const struct of_device_id sys_cpld_of_ids[] = {
    {
        .compatible = "nokia,sys_cpld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, sys_cpld_of_ids);

static const struct i2c_device_id sys_cpld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sys_cpld_ids);

static struct i2c_driver sys_cpld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(sys_cpld_of_ids),
    },
    .probe        = sys_cpld_probe,
    .remove       = sys_cpld_remove,
    .id_table     = sys_cpld_ids,
    .address_list = cpld_address_list,
};

static int __init sys_cpld_init(void)
{
    return i2c_add_driver(&sys_cpld_driver);
}

static void __exit sys_cpld_exit(void)
{
    i2c_del_driver(&sys_cpld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA H6-64 SYS_CPLD driver");
MODULE_LICENSE("GPL");

module_init(sys_cpld_init);
module_exit(sys_cpld_exit);
