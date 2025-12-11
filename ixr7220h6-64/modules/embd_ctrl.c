//  * Embedded Controller driver for Nokia Router
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

#define DRIVER_NAME "embd_ctrl"

// REGISTERS ADDRESS MAP
#define CPU_TEMP_REG            0x10
#define MEM0_TEMP_REG           0x12
#define MEM1_TEMP_REG           0x13

static const unsigned short ec_address_list[] = {0x21, I2C_CLIENT_END};

struct ec_data {
    struct i2c_client *client;
    struct mutex  update_lock;
};

static int ec_i2c_read(struct ec_data *data, u8 reg)
{
    int val = 0;
    struct i2c_client *client = data->client;

    val = i2c_smbus_read_byte_data(client, reg);
    if (val < 0) {
         dev_warn(&client->dev, "EC READ WARN: reg(0x%02x) err %d\n", reg, val);
    }

    return val;
}

#ifdef EC_WRITE
static void ec_i2c_write(struct ec_data *data, u8 reg, u8 value)
{
    int res = 0;
    struct i2c_client *client = data->client;

    mutex_lock(&data->update_lock);
    res = i2c_smbus_write_byte_data(client, reg, value);
    if (res < 0) {
        dev_warn(&client->dev, "EC WRITE WARN: reg(0x%02x) err %d\n", reg, res);
    }
    mutex_unlock(&data->update_lock);
}
#endif

static ssize_t show_cpu_temperature(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct ec_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = ec_i2c_read(data, CPU_TEMP_REG);
    return sprintf(buf, "%d\n", (s8)val * 1000);
}

static ssize_t show_mem0_temperature(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct ec_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = ec_i2c_read(data, MEM0_TEMP_REG);
    return sprintf(buf, "%d\n", (s8)val * 1000);
}

static ssize_t show_mem1_temperature(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct ec_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = ec_i2c_read(data, MEM1_TEMP_REG);
    return sprintf(buf, "%d\n", (s8)val * 1000);
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(cpu_temperature, S_IRUGO, show_cpu_temperature, NULL, 0);
static SENSOR_DEVICE_ATTR(mem0_temperature, S_IRUGO, show_mem0_temperature, NULL, 0);
static SENSOR_DEVICE_ATTR(mem1_temperature, S_IRUGO, show_mem1_temperature, NULL, 0);

static struct attribute *embd_ctrl_attributes[] = {
    &sensor_dev_attr_cpu_temperature.dev_attr.attr,
    &sensor_dev_attr_mem0_temperature.dev_attr.attr,
    &sensor_dev_attr_mem1_temperature.dev_attr.attr,
    NULL
};

static const struct attribute_group embd_ctrl_group = {
    .attrs = embd_ctrl_attributes,
};

static int embd_ctrl_probe(struct i2c_client *client)
{
    int status = 0;
    struct ec_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_warn(&client->dev, "EC PROBE WARN: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia embeded controller chip found.\n");
    data = kzalloc(sizeof(struct ec_data), GFP_KERNEL);

    if (!data) {
        dev_warn(&client->dev, "EC PROBE WARN: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &embd_ctrl_group);
    if (status) {
        dev_warn(&client->dev, "EC INIT WARN: Cannot create sysfs\n");
        goto exit;
    }
    
    return 0;

exit:
    return status;
}

static void embd_ctrl_remove(struct i2c_client *client)
{
    struct ec_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &embd_ctrl_group);
    kfree(data);
}

static const struct of_device_id embd_ctrl_of_ids[] = {
    {
        .compatible = "Nokia,embd_ctrl",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, embd_ctrl_of_ids);

static const struct i2c_device_id embd_ctrl_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, embd_ctrl_ids);

static struct i2c_driver embd_ctrl_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(embd_ctrl_of_ids),
    },
    .probe        = embd_ctrl_probe,
    .remove       = embd_ctrl_remove,
    .id_table     = embd_ctrl_ids,
    .address_list = ec_address_list,
};

static int __init embd_ctrl_init(void)
{
    return i2c_add_driver(&embd_ctrl_driver);
}

static void __exit embd_ctrl_exit(void)
{
    i2c_del_driver(&embd_ctrl_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA Embedded Controller driver");
MODULE_LICENSE("GPL");

module_init(embd_ctrl_init);
module_exit(embd_ctrl_exit);
