//  * SCM CPLD driver
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

#define DRIVER_NAME "scm_cpld"

// REGISTERS ADDRESS MAP
#define MINOR_REV_REG               0x00
#define MAJOR_REV_REG               0x01
#define HW_REV_REG                  0x02
#define SCRATCH_REG                 0x04
#define RST_CAUSE_CTRL_REG          0x2F
#define RST_CAUSE_REG               0x52

// REG BIT FIELD POSITION or MASK
#define HW_REV_REG_BRDID_MSK        0xF
#define HW_REV_REG_PCB_VER          0x4

static const unsigned short cpld_address_list[] = {0x35, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int reset_cause;
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

static ssize_t show_board_id(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev); 
    u8 val = 0;

    val = cpld_i2c_read(data, HW_REV_REG) & HW_REV_REG_BRDID_MSK;   
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_pcb_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;

    val = cpld_i2c_read(data, HW_REV_REG) >> HW_REV_REG_PCB_VER;
    return sprintf(buf, "0x%02x\n", val);
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

static ssize_t show_rst_cause(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%02x\n", data->reset_cause);
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(board_id, S_IRUGO, show_board_id, NULL, 0);
static SENSOR_DEVICE_ATTR(pcb_ver, S_IRUGO, show_pcb_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);
static SENSOR_DEVICE_ATTR(reset_cause, S_IRUGO, show_rst_cause, NULL, 0);

static struct attribute *scm_cpld_attributes[] = {
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_id.dev_attr.attr,
    &sensor_dev_attr_pcb_ver.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_reset_cause.dev_attr.attr,
    NULL
};

static const struct attribute_group scm_cpld_group = {
    .attrs = scm_cpld_attributes,
};

static int scm_cpld_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia SCM CPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &scm_cpld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->reset_cause = cpld_i2c_read(data, RST_CAUSE_REG);
    cpld_i2c_write(data, RST_CAUSE_CTRL_REG, 0x1);

    return 0;

exit:
    return status;
}

static void scm_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &scm_cpld_group);
    kfree(data);
}

static const struct of_device_id scm_cpld_of_ids[] = {
    {
        .compatible = "nokia,scm_cpld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, scm_cpld_of_ids);

static const struct i2c_device_id scm_cpld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, scm_cpld_ids);

static struct i2c_driver scm_cpld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(scm_cpld_of_ids),
    },
    .probe        = scm_cpld_probe,
    .remove       = scm_cpld_remove,
    .id_table     = scm_cpld_ids,
    .address_list = cpld_address_list,
};

static int __init scm_cpld_init(void)
{
    return i2c_add_driver(&scm_cpld_driver);
}

static void __exit scm_cpld_exit(void)
{
    i2c_del_driver(&scm_cpld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA SCM CPLD driver");
MODULE_LICENSE("GPL");

module_init(scm_cpld_init);
module_exit(scm_cpld_exit);
