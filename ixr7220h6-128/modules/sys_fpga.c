//  * CPLD driver for Nokia-7220-IXR-H6-128 Router
//  *
//  * Copyright (C) 2026 Nokia Corporation.
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

#define DRIVER_NAME "sys_fpga"

// REGISTERS ADDRESS MAP
#define HW_BOARD_VER_REG                 0x00
#define VER_MAJOR_REG                    0x01
#define VER_MINOR_REG                    0x02
#define BMC_TIMING_FCM_PSU_PRESENT_REG   0x07
#define SSD_PRESENT_REG                  0x08
#define PSU_POWERGOOD_REG                0x51

// REG BIT FIELD POSITION or MASK

static const unsigned short cpld_address_list[] = {0x60, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int reset_cause;
};

static int cpld_i2c_read(struct cpld_data *data, u8 reg)
{
    int val = 0;
    struct i2c_client *client = data->client;

    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
         dev_warn(&client->dev, "SYS_FPGA READ ERROR: reg(0x%02x) err %d\n", reg, val);
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
        dev_warn(&client->dev, "SYS_FPGA WRITE ERROR: reg(0x%02x) err %d\n", reg, res);
    }
    mutex_unlock(&data->update_lock);
}

static ssize_t show_hw_board_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, HW_BOARD_VER_REG);
    return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_fpga_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 reg_major = 0;
    u8 reg_minor = 0;

    reg_major = cpld_i2c_read(data, VER_MAJOR_REG);
    reg_minor = cpld_i2c_read(data, VER_MINOR_REG);

    return sprintf(buf, "%d.%d\n", reg_major, reg_minor);
}

static ssize_t show_bmc_timing_fcm_psu_present(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, BMC_TIMING_FCM_PSU_PRESENT_REG);
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_psu_ok(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, PSU_POWERGOOD_REG);
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_ssd_present(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;

    val = cpld_i2c_read(data, SSD_PRESENT_REG);
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(hw_board_version, S_IRUGO, show_hw_board_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(fpga_version, S_IRUGO, show_fpga_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(psu1_ok, S_IRUGO, show_psu_ok, NULL, 0);
static SENSOR_DEVICE_ATTR(psu2_ok, S_IRUGO, show_psu_ok, NULL, 1);
static SENSOR_DEVICE_ATTR(psu3_ok, S_IRUGO, show_psu_ok, NULL, 2);
static SENSOR_DEVICE_ATTR(psu4_ok, S_IRUGO, show_psu_ok, NULL, 3);
static SENSOR_DEVICE_ATTR(psu1_pres, S_IRUGO, show_bmc_timing_fcm_psu_present, NULL, 3);
static SENSOR_DEVICE_ATTR(psu2_pres, S_IRUGO, show_bmc_timing_fcm_psu_present, NULL, 2);
static SENSOR_DEVICE_ATTR(psu3_pres, S_IRUGO, show_bmc_timing_fcm_psu_present, NULL, 1);
static SENSOR_DEVICE_ATTR(psu4_pres, S_IRUGO, show_bmc_timing_fcm_psu_present, NULL, 0);
static SENSOR_DEVICE_ATTR(ssd1_pres, S_IRUGO, show_ssd_present, NULL, 1);
static SENSOR_DEVICE_ATTR(ssd2_pres, S_IRUGO, show_ssd_present, NULL, 0);

static struct attribute *sys_fpga_attributes[] = {
    &sensor_dev_attr_hw_board_version.dev_attr.attr,
    &sensor_dev_attr_fpga_version.dev_attr.attr,
    &sensor_dev_attr_psu1_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_ok.dev_attr.attr,
    &sensor_dev_attr_psu3_ok.dev_attr.attr,
    &sensor_dev_attr_psu4_ok.dev_attr.attr,
    &sensor_dev_attr_psu1_pres.dev_attr.attr,
    &sensor_dev_attr_psu2_pres.dev_attr.attr,
    &sensor_dev_attr_psu3_pres.dev_attr.attr,
    &sensor_dev_attr_psu4_pres.dev_attr.attr,
    &sensor_dev_attr_ssd1_pres.dev_attr.attr,
    &sensor_dev_attr_ssd2_pres.dev_attr.attr,
    NULL
};

static const struct attribute_group sys_fpga_group = {
    .attrs = sys_fpga_attributes,
};

static int sys_fpga_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia SYS_FPGA chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &sys_fpga_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    return 0;

exit:
    return status;
}

static void sys_fpga_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &sys_fpga_group);
    kfree(data);
}

static const struct of_device_id sys_fpga_of_ids[] = {
    {
        .compatible = "nokia,sys_fpga",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, sys_fpga_of_ids);

static const struct i2c_device_id sys_fpga_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sys_fpga_ids);

static struct i2c_driver sys_fpga_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(sys_fpga_of_ids),
    },
    .probe        = sys_fpga_probe,
    .remove       = sys_fpga_remove,
    .id_table     = sys_fpga_ids,
    .address_list = cpld_address_list,
};

static int __init sys_fpga_init(void)
{
    return i2c_add_driver(&sys_fpga_driver);
}

static void __exit sys_fpga_exit(void)
{
    i2c_del_driver(&sys_fpga_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA H6-128 SYS_FPGA driver");
MODULE_LICENSE("GPL");

module_init(sys_fpga_init);
module_exit(sys_fpga_exit);
