//  * PDB_L CPLD driver 
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
// 

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

#define DRIVER_NAME "pdbl_cpld"

// REGISTERS ADDRESS MAP
#define MINOR_REV_REG               0x00
#define MAJOR_REV_REG               0x01

static const unsigned short cpld_address_list[] = {0x60, I2C_CLIENT_END};

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

static ssize_t show_code_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 major_ver = 0;
    u8 minor_ver = 0;

    major_ver = cpld_i2c_read(data, MAJOR_REV_REG);
    minor_ver = cpld_i2c_read(data, MINOR_REV_REG);
    return sprintf(buf, "%d.%d\n", major_ver, minor_ver);
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);

static struct attribute *pdbl_cpld_attributes[] = {
    &sensor_dev_attr_code_ver.dev_attr.attr,    
    NULL
};

static const struct attribute_group pdbl_cpld_group = {
    .attrs = pdbl_cpld_attributes,
};

static int pdbl_cpld_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia PDB_L CPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &pdbl_cpld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    return 0;

exit:
    return status;
}

static void pdbl_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &pdbl_cpld_group);
    kfree(data);
}

static const struct of_device_id pdbl_cpld_of_ids[] = {
    {
        .compatible = "nokia,pdbl_cpld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, pdbl_cpld_of_ids);

static const struct i2c_device_id pdbl_cpld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, pdbl_cpld_ids);

static struct i2c_driver pdbl_cpld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(pdbl_cpld_of_ids),
    },
    .probe        = pdbl_cpld_probe,
    .remove       = pdbl_cpld_remove,
    .id_table     = pdbl_cpld_ids,
    .address_list = cpld_address_list,
};

static int __init pdbl_cpld_init(void)
{
    return i2c_add_driver(&pdbl_cpld_driver);
}

static void __exit pdbl_cpld_exit(void)
{
    i2c_del_driver(&pdbl_cpld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA PDB_L CPLD driver");
MODULE_LICENSE("GPL");

module_init(pdbl_cpld_init);
module_exit(pdbl_cpld_exit);
