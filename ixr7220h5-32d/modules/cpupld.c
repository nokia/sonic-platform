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

#define DRIVER_NAME "cpupld"

// REGISTERS ADDRESS MAP
#define SCRATCH_REG                 0x00
#define CODE_REV_REG                0x01
#define BOARD_INFO_REG              0x02
#define BIOS_CTRL_REG1              0x05
#define BIOS_CTRL_REG2              0x06
#define WATCHDOG_REG                0x07
#define PERIF_WP_REG                0x09
#define PWR_CTRL_REG1               0x0A
#define PWR_CTRL_REG2               0x0B
#define PWR_CTRL_REG3               0x0C
#define PWR_STATUS_REG1             0x0D
#define PWR_STATUS_REG2             0x0E
#define PWR_STATUS_REG3             0x0F
#define BOARD_STATUS_REG            0x10
#define BOARD_CTRL_REG1             0x18
#define BOARD_CTRL_REG2             0x19
#define RST_PLD_REG                 0x20
#define RST_CTRLMSK_REG1            0x21
#define RST_CTRL_REG1               0x22
#define RST_CTRLMSK_REG2            0x23
#define RST_CTRL_REG2               0x24
#define RST_CTRLMSK_REG3            0x25
#define RST_CTRL_REG3               0x26
#define RST_CAUSE_REG               0x28
#define CPU_INT_CLR_REG             0x30
#define CPU_INT_MSK_REG             0x31
#define CPU_INT_REG                 0x38
#define HITLESS_REG                 0x40
#define PWR_SEQ_REG                 0x80
#define CODE_DAY_REG                0xF0
#define CODE_MONTH_REG              0xF1
#define CODE_YEAR_REG               0xF2
#define TEST_CODE_REV_REG           0xF3


// REG BIT FIELD POSITION or MASK
#define BOARD_INFO_REG_VER_MSK      0x7

#define WATCHDOG_REG_WD_PUNCH       0x0
#define WATCHDOG_REG_WD_EN          0x3
#define WATCHDOG_REG_WD_TIMER       0x4

#define RST_CAUSE_REG_MB_PWR_ERR    0x0
#define RST_CAUSE_REG_BOOT_FAIL     0x2
#define RST_CAUSE_REG_BIOS_SW       0x3
#define RST_CAUSE_REG_WD_FAIL       0x4
#define RST_CAUSE_REG_WARM_RST      0x6
#define RST_CAUSE_REG_COLD_RST      0x7

#define HITLESS_REG_EN              0x0

static const unsigned short cpld_address_list[] = {0x40, I2C_CLIENT_END};

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
      
    val = cpld_i2c_read(data, BOARD_INFO_REG) & BOARD_INFO_REG_VER_MSK;
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_watchdog(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 reg_val;
    int ret;
    val = cpld_i2c_read(data, WATCHDOG_REG);    

    switch (sda->index) {
    case WATCHDOG_REG_WD_PUNCH:
    case WATCHDOG_REG_WD_EN:
        return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
    case WATCHDOG_REG_WD_TIMER:
        reg_val = (val>>sda->index) & 0x7;
        switch (reg_val) {
        case 0b000:
            ret = 5;
            break;
        case 0b001:
            ret = 10;
            break;
        case 0b010:
            ret = 30;
            break;
        case 0b011:
            ret = 60;
            break;
        case 0b100:
            ret = 180;
            break;
        case 0b101:
            ret = 240;
            break;
        case 0b110:
            ret = 360;
            break;
        case 0b111:
            ret = 480;
            break; 
        default:
            ret = 0;
            break;
        } 
        return sprintf(buf, "%d: %d seconds\n", reg_val, ret);  
    default:
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    }

}

static ssize_t set_watchdog(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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

    switch (sda->index) {
    case WATCHDOG_REG_WD_PUNCH:
    case WATCHDOG_REG_WD_EN:
        if (usr_val > 1) {
            return -EINVAL;
        }
        mask = (~(1 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, WATCHDOG_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, WATCHDOG_REG, (reg_val | usr_val));
        break;
    case WATCHDOG_REG_WD_TIMER:
        if (usr_val > 7) {
            return -EINVAL;
        }
        mask = (~(7 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, WATCHDOG_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, WATCHDOG_REG, (reg_val | usr_val));
        break;
    default:        
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);     
    } 

    return count;
}

static ssize_t show_rst_cause(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%02x\n", data->reset_cause);
}

static ssize_t show_hitless(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, HITLESS_REG);

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

// sysfs attributes 
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(board_ver, S_IRUGO, show_board_ver, NULL, 0);

static SENSOR_DEVICE_ATTR(wd_punch, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_PUNCH);
static SENSOR_DEVICE_ATTR(wd_enable, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_EN);
static SENSOR_DEVICE_ATTR(wd_timer, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_TIMER);
static SENSOR_DEVICE_ATTR(reset_cause, S_IRUGO, show_rst_cause, NULL, 0);

static SENSOR_DEVICE_ATTR(hitless_en, S_IRUGO, show_hitless, NULL, HITLESS_REG_EN);
static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);

static struct attribute *cpupld_attributes[] = {
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,    
    
    &sensor_dev_attr_wd_punch.dev_attr.attr,
    &sensor_dev_attr_wd_enable.dev_attr.attr,
    &sensor_dev_attr_wd_timer.dev_attr.attr,
    &sensor_dev_attr_reset_cause.dev_attr.attr,

    &sensor_dev_attr_hitless_en.dev_attr.attr,
    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,
    NULL
};

static const struct attribute_group cpupld_group = {
    .attrs = cpupld_attributes,
};

static int cpupld_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia CPUCPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &cpupld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->reset_cause = cpld_i2c_read(data, RST_CAUSE_REG);
    cpld_i2c_write(data, RST_CAUSE_REG, 0);

    return 0;

exit:
    return status;
}

static void cpupld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &cpupld_group);
    kfree(data);
}

static const struct of_device_id cpupld_of_ids[] = {
    {
        .compatible = "nokia,cpupld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, cpupld_of_ids);

static const struct i2c_device_id cpupld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, cpupld_ids);

static struct i2c_driver cpupld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(cpupld_of_ids),
    },
    .probe        = cpupld_probe,
    .remove       = cpupld_remove,
    .id_table     = cpupld_ids,
    .address_list = cpld_address_list,
};

static int __init cpupld_init(void)
{
    return i2c_add_driver(&cpupld_driver);
}

static void __exit cpupld_exit(void)
{
    i2c_del_driver(&cpupld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA CPLD driver");
MODULE_LICENSE("GPL");

module_init(cpupld_init);
module_exit(cpupld_exit);

