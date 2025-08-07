//  * CPLD driver for Nokia-7220-IXR-H3 Router
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

#define DRIVER_NAME "nokia_7220h3_cpupld"

// REG ADDRESS-MAP
#define SYS_CPLD_REV_REG              0x00
#define SYS_CPLD_TEST_REG             0x01
#define BOARD_REV_TYPE_REG            0x02
#define WATCHDOG_REG                  0x06
#define CPU_SYS_RST_REG               0x08
#define PWR_STATUS_REG                0x0A
#define CPU_CPLD_UPGRADE_REG          0x0B

// REG BIT FIELD POSITION or MASK
#define SYS_CPLD_REV_REG_MNR_MSK      0xF
#define SYS_CPLD_REV_REG_MJR          0x4

#define BOARD_REV_TYPE_REG_TYPE_MSK   0xF
#define BOARD_REV_TYPE_REG_REV        0x4

#define WATCHDOG_REG_WD_PUNCH         0x0
#define WATCHDOG_REG_WD_EN            0x3
#define WATCHDOG_REG_WD_TIMER         0x4

#define CPU_SYS_RST_REG_CPU_PWR_ERR     0x0
#define CPU_SYS_RST_REG_BOOT_FAIL       0x2
#define CPU_SYS_RST_REG_BIOS_SWITCHOVER 0x3
#define CPU_SYS_RST_REG_WD_FAIL         0x4
#define CPU_SYS_RST_REG_WARM_RST        0x6
#define CPU_SYS_RST_REG_COLD_RST        0x7

#define POWER_STATUS_REG_V1P35         0x0
#define POWER_STATUS_REG_V1P8          0x1
#define POWER_STATUS_REG_V3P3          0x2
#define POWER_STATUS_REG_V1P0          0x3
#define POWER_STATUS_REG_V1P1          0x4
#define POWER_STATUS_REG_PWR_CORE      0x5
#define POWER_STATUS_REG_PWR_VDDR      0x6
#define POWER_STATUS_REG_DDR_VTT       0x7

static const unsigned short cpld_address_list[] = {0x31, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int cpld_major_version;
    int cpld_minor_version;
    int board_revision;
    int board_type;
};

static int nokia_7220_h3_cpupld_read(struct cpld_data *data, u8 reg)
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

static void nokia_7220_h3_cpupld_write(struct cpld_data *data, u8 reg, u8 value)
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

static ssize_t show_cpld_major_version(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "0x%02x\n", data->cpld_major_version);
}

static ssize_t show_cpld_minor_version(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "0x%02x\n", data->cpld_minor_version);
}

static ssize_t show_board_revision(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *revision = NULL;
    
    switch (data->board_revision) {
    case 0:
        revision = "X00";
        break;
    case 1:
        revision = "X01";
        break;
    case 2:
        revision = "X02";
        break;
    case 3:
        revision = "X03";
        break;    
    default:
        revision = "RSVD";
        break;
    }

    return sprintf(buf, "0x%02x %s\n", data->board_revision, revision);
}

static ssize_t show_board_type(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *brd_type = NULL;
    
    switch (data->board_type) {
    case 0:
        brd_type = "BROADWELL-DE CPU Platform";
        break;
    case 1:
        brd_type = "Deverton CPU Platform";
        break;
    default:
        brd_type = "RESERVED";
        break;
    }

    return sprintf(buf, "0x%02x %s\n", data->board_type, brd_type);
}

static ssize_t show_scratch(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = nokia_7220_h3_cpupld_read(data, SYS_CPLD_TEST_REG);

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

    nokia_7220_h3_cpupld_write(data, SYS_CPLD_TEST_REG, usr_val);

    return count;
}

static ssize_t show_watchdog(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 reg_val;
    int ret;
    val = nokia_7220_h3_cpupld_read(data, WATCHDOG_REG);    

    switch (sda->index) {
    case WATCHDOG_REG_WD_PUNCH:
    case WATCHDOG_REG_WD_EN:
        return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
    case WATCHDOG_REG_WD_TIMER:
        reg_val = (val>>sda->index) & 0x7;
        switch (reg_val) {
        case 0b000:
            ret = 15;
            break;
        case 0b001:
            ret = 20;
            break;
        case 0b010:
            ret = 30;
            break;
        case 0b011:
            ret = 40;
            break;
        case 0b100:
            ret = 50;
            break;
        case 0b101:
            ret = 60;
            break;
        case 0b110:
            ret = 65;
            break;
        case 0b111:
            ret = 70;
            break; 
        default:
            ret = 0;
            break;
        } 
        return sprintf(buf, "0x%02x %dsec\n", reg_val, ret);  
    default:
        return sprintf(buf, "Error: Reserved register!\n");
    }

}

static ssize_t set_watchdog(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;

    int ret = kstrtou8(buf, 16, &usr_val);
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
        reg_val = nokia_7220_h3_cpupld_read(data, WATCHDOG_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        nokia_7220_h3_cpupld_write(data, WATCHDOG_REG, (reg_val | usr_val));
        break;
    case WATCHDOG_REG_WD_TIMER:
        if (usr_val > 7) {
            return -EINVAL;
        }
        mask = (~(7 << sda->index)) & 0xFF;
        reg_val = nokia_7220_h3_cpupld_read(data, WATCHDOG_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        nokia_7220_h3_cpupld_write(data, WATCHDOG_REG, (reg_val | usr_val));
        break;
    default:        
        break;       
    } 

    return count;
}

static ssize_t show_sys_rst_cause(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = nokia_7220_h3_cpupld_read(data, CPU_SYS_RST_REG);
    
    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_cpu_pwr_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = nokia_7220_h3_cpupld_read(data, PWR_STATUS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_cpu_cpld_upgrade(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = nokia_7220_h3_cpupld_read(data, CPU_CPLD_UPGRADE_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_cpu_cpld_upgrade(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_cpupld_read(data, CPU_CPLD_UPGRADE_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_cpupld_write(data, CPU_CPLD_UPGRADE_REG, (reg_val | usr_val));

    return count;
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(cpld_major_version, S_IRUGO, show_cpld_major_version, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_minor_version, S_IRUGO, show_cpld_minor_version, NULL, SYS_CPLD_REV_REG_MJR);
static SENSOR_DEVICE_ATTR(board_revision, S_IRUGO, show_board_revision, NULL, 0);
static SENSOR_DEVICE_ATTR(board_type, S_IRUGO, show_board_type, NULL, BOARD_REV_TYPE_REG_REV);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);
static SENSOR_DEVICE_ATTR(wd_punch, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_PUNCH);
static SENSOR_DEVICE_ATTR(wd_enable, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_EN);
static SENSOR_DEVICE_ATTR(wd_timer, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_TIMER);
static SENSOR_DEVICE_ATTR(cpu_pwr_err, S_IRUGO, show_sys_rst_cause, NULL, CPU_SYS_RST_REG_CPU_PWR_ERR);
static SENSOR_DEVICE_ATTR(boot_fail, S_IRUGO, show_sys_rst_cause, NULL, CPU_SYS_RST_REG_BOOT_FAIL);
static SENSOR_DEVICE_ATTR(bios_switchover, S_IRUGO, show_sys_rst_cause, NULL, CPU_SYS_RST_REG_BIOS_SWITCHOVER);
static SENSOR_DEVICE_ATTR(wd_reset, S_IRUGO, show_sys_rst_cause, NULL, CPU_SYS_RST_REG_WD_FAIL);
static SENSOR_DEVICE_ATTR(warm_reset, S_IRUGO, show_sys_rst_cause, NULL, CPU_SYS_RST_REG_WARM_RST);
static SENSOR_DEVICE_ATTR(cold_reset, S_IRUGO, show_sys_rst_cause, NULL, CPU_SYS_RST_REG_COLD_RST);
static SENSOR_DEVICE_ATTR(cpu_pwr_1v35, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_V1P35);
static SENSOR_DEVICE_ATTR(cpu_pwr_1v8, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_V1P8);
static SENSOR_DEVICE_ATTR(cpu_pwr_3v3, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_V3P3);
static SENSOR_DEVICE_ATTR(cpu_pwr_1v0, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_V1P0);
static SENSOR_DEVICE_ATTR(cpu_pwr_1v1, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_V1P1);
static SENSOR_DEVICE_ATTR(cpu_pwr_core, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_PWR_CORE);
static SENSOR_DEVICE_ATTR(cpu_pwr_vddr, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_PWR_VDDR);
static SENSOR_DEVICE_ATTR(cpu_pwr_1v5, S_IRUGO, show_cpu_pwr_status, NULL, POWER_STATUS_REG_DDR_VTT);
static SENSOR_DEVICE_ATTR(cpu_cpld_upgrade, S_IRUGO | S_IWUSR, show_cpu_cpld_upgrade, set_cpu_cpld_upgrade, 0);

static struct attribute *nokia_7220_h3_cpupld_attributes[] = {
    &sensor_dev_attr_cpld_major_version.dev_attr.attr,
    &sensor_dev_attr_cpld_minor_version.dev_attr.attr,
    &sensor_dev_attr_board_revision.dev_attr.attr,
    &sensor_dev_attr_board_type.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,    
    &sensor_dev_attr_wd_punch.dev_attr.attr,
    &sensor_dev_attr_wd_enable.dev_attr.attr,
    &sensor_dev_attr_wd_timer.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_err.dev_attr.attr,
    &sensor_dev_attr_boot_fail.dev_attr.attr,
    &sensor_dev_attr_bios_switchover.dev_attr.attr,
    &sensor_dev_attr_wd_reset.dev_attr.attr,
    &sensor_dev_attr_warm_reset.dev_attr.attr,
    &sensor_dev_attr_cold_reset.dev_attr.attr,    
    &sensor_dev_attr_cpu_pwr_1v35.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_1v8.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_3v3.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_1v0.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_1v1.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_core.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_vddr.dev_attr.attr,
    &sensor_dev_attr_cpu_pwr_1v5.dev_attr.attr,
    &sensor_dev_attr_cpu_cpld_upgrade.dev_attr.attr,       
    NULL
};

static const struct attribute_group nokia_7220_h3_cpupld_group = {
    .attrs = nokia_7220_h3_cpupld_attributes,
};

static int nokia_7220_h3_cpupld_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H3 CPUCPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &nokia_7220_h3_cpupld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->cpld_major_version = nokia_7220_h3_cpupld_read(data, SYS_CPLD_REV_REG) & SYS_CPLD_REV_REG_MNR_MSK;
    data->cpld_minor_version = nokia_7220_h3_cpupld_read(data, SYS_CPLD_REV_REG) >> SYS_CPLD_REV_REG_MJR;
    data->board_revision = nokia_7220_h3_cpupld_read(data, BOARD_REV_TYPE_REG) & BOARD_REV_TYPE_REG_TYPE_MSK;
    data->board_type = nokia_7220_h3_cpupld_read(data, BOARD_REV_TYPE_REG) >> BOARD_REV_TYPE_REG_REV;  
    
    return 0;

exit:
    return status;
}

static void nokia_7220_h3_cpupld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &nokia_7220_h3_cpupld_group);
    kfree(data);
}

static const struct of_device_id nokia_7220_h3_cpupld_of_ids[] = {
    {
        .compatible = "nokia,7220_h3_cpupld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, nokia_7220_h3_cpupld_of_ids);

static const struct i2c_device_id nokia_7220_h3_cpupld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, nokia_7220_h3_cpupld_ids);

static struct i2c_driver nokia_7220_h3_cpupld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(nokia_7220_h3_cpupld_of_ids),
    },
    .probe        = nokia_7220_h3_cpupld_probe,
    .remove       = nokia_7220_h3_cpupld_remove,
    .id_table     = nokia_7220_h3_cpupld_ids,
    .address_list = cpld_address_list,
};

static int __init nokia_7220_h3_cpupld_init(void)
{
    return i2c_add_driver(&nokia_7220_h3_cpupld_driver);
}

static void __exit nokia_7220_h3_cpupld_exit(void)
{
    i2c_del_driver(&nokia_7220_h3_cpupld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H3 CPLD driver");
MODULE_LICENSE("GPL");

module_init(nokia_7220_h3_cpupld_init);
module_exit(nokia_7220_h3_cpupld_exit);
