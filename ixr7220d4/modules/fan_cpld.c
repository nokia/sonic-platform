//  * FAN CPLD
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

#define DRIVER_NAME "fan_cpld"

// REGISTERS ADDRESS MAP
#define FAN_BOARD_INFO_REG          0x00
#define FAN_CPLD_VER_REG            0x01
#define FAN_CPLD_RESET_REG          0x04
#define FAN_PRESENCE_REG            0x0F
#define FAN_DIR_REG                 0x10
#define FAN_PWM_REG                 0x11
#define FAN6_SPEED_REG              0x12
#define FAN5_SPEED_REG              0x13
#define FAN4_SPEED_REG              0x14
#define FAN3_SPEED_REG              0x15
#define FAN2_SPEED_REG              0x16
#define FAN1_SPEED_REG              0x17
#define LED1_DISPLAY_REG            0x1C
#define LED2_DISPLAY_REG            0x1D
#define FAN6_R_SPEED_REG            0x22
#define FAN5_R_SPEED_REG            0x23
#define FAN4_R_SPEED_REG            0x24
#define FAN3_R_SPEED_REG            0x25
#define FAN2_R_SPEED_REG            0x26
#define FAN1_R_SPEED_REG            0x27

// REG BIT FIELD POSITION or MASK
#define BOARD_INFO_REG_TYPE_MSK     0x7
#define FAN_PWM_MSK                 0xF
#define FAN_CPLD_RESET_BIT          0x7

#define FAN6_PRES                   0x0
#define FAN5_PRES                   0x1
#define FAN4_PRES                   0x2
#define FAN3_PRES                   0x3
#define FAN2_PRES                   0x4
#define FAN1_PRES                   0x5

#define FAN1_ID                     0x0
#define FAN2_ID                     0x1
#define FAN3_ID                     0x2
#define FAN4_ID                     0x3
#define FAN5_ID                     0x4
#define FAN6_ID                     0x5
#define FAN7_ID                     0x6
#define FAN8_ID                     0x7
#define FAN9_ID                     0x8
#define FAN10_ID                    0x9
#define FAN11_ID                    0xA
#define FAN12_ID                    0xB

#define FAN1_LED_REG                0x0
#define FAN2_LED_REG                0x2
#define FAN3_LED_REG                0x0
#define FAN4_LED_REG                0x2
#define FAN5_LED_REG                0x4
#define FAN6_LED_REG                0x6

static const unsigned short cpld_address_list[] = {0x66, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
};

enum fan_led_mode {
    FAN_LED_BASE,
    FAN_LED_GREEN,
    FAN_LED_RED,
    FAN_LED_OFF
};

char *fan_led_mode_str[] = {"base", "green", "red", "off"};

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

/* ---------- */

static ssize_t show_board_type(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = (cpld_i2c_read(data, FAN_BOARD_INFO_REG) >> 2 ) & BOARD_INFO_REG_TYPE_MSK;
    char *brd_type = NULL;

    switch (val) {
    case 0:
        brd_type = "R0A";
        break;
    case 1:
        brd_type = "R0B";
        break;
    case 2:
        brd_type = "R01";
        break;
    default:
        brd_type = "RESERVED";
        break;
    }

    return sprintf(buf, "0x%x %s\n", val, brd_type);
}

/* ---------- */

static ssize_t show_fan_cpld_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, FAN_CPLD_VER_REG);
    return sprintf(buf, "0x%02x\n", val);
}

/* ---------- */

static ssize_t show_fan_cpld_reset(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);

    u8 val = cpld_i2c_read(data, FAN_CPLD_RESET_REG);

    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 1:0);
}

/* ---------- */

static ssize_t show_fan_present(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = cpld_i2c_read(data, FAN_PRESENCE_REG);

    /* If the bit is set, fan is not present. So, we are toggling intentionally */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 0:1);
}

/* ---------- */

static ssize_t show_fan_direction(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = cpld_i2c_read(data, FAN_DIR_REG);

    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 1:0);
}

/* ---------- */

static ssize_t show_fan_pwm(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);

    u8 val = cpld_i2c_read(data, FAN_PWM_REG) & FAN_PWM_MSK;

    return sprintf(buf,"%d\n", val);
}

static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);

    u8 reg_val=0, usr_val=0, mask;

    int ret=kstrtou8(buf,10, &usr_val);
    if (ret != 0) {
        return ret;
    }
    if (usr_val > 0xF) {
        return -EINVAL;
    }

    mask = (~FAN_PWM_MSK) & 0xFF;
    reg_val = cpld_i2c_read(data, FAN_PWM_REG);
    reg_val = reg_val & mask;

    cpld_i2c_write(data, FAN_PWM_REG, (reg_val|usr_val));

    return count;

}

/* ---------- */

static ssize_t show_fan_speed(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 speed_reg = 0;
    int val_rpm = 0;
    switch (sda->index) {
    case 0:
        speed_reg = FAN1_SPEED_REG;
        break;
    case 1:
        speed_reg = FAN1_R_SPEED_REG;
        break;
    case 2:
        speed_reg = FAN2_SPEED_REG;
        break;
    case 3:
        speed_reg = FAN2_R_SPEED_REG;
        break;
    case 4:
        speed_reg = FAN3_SPEED_REG;
        break;
    case 5:
        speed_reg = FAN3_R_SPEED_REG;
        break;
    case 6:
        speed_reg = FAN4_SPEED_REG;
        break;
    case 7:
        speed_reg = FAN4_R_SPEED_REG;
        break;
    case 8:
        speed_reg = FAN5_SPEED_REG;
        break;
    case 9:
        speed_reg = FAN5_R_SPEED_REG;
        break;
    case 10:
        speed_reg = FAN6_SPEED_REG;
        break;
    case 11:
        speed_reg = FAN6_R_SPEED_REG;
        break;
    }

    val = cpld_i2c_read(data, speed_reg);
    val_rpm = (val*100);

    return sprintf(buf, "%d\n", val_rpm);
}

/* ------------- */

static ssize_t show_fan_led2_status(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = cpld_i2c_read(data, LED2_DISPLAY_REG);
    val = (val >> sda->index) & 0x3;

    return sprintf(buf,"%s\n",fan_led_mode_str[val]);
}

static ssize_t set_fan_led2_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, mask=0, usr_val=0;
    int i;

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, LED2_DISPLAY_REG);
    reg_val = reg_val & mask;

    for(i=FAN_LED_BASE ; i<=FAN_LED_OFF ; i++) {
        if(strncmp(buf, fan_led_mode_str[i],strlen(fan_led_mode_str[i]))==0) {
            usr_val = i << sda->index;
            cpld_i2c_write(data, LED2_DISPLAY_REG, reg_val|usr_val);
            break;
        }
    }

    return count;
}

/* ------------- */

static ssize_t show_fan_led1_status(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = cpld_i2c_read(data, LED1_DISPLAY_REG);
    val = (val >> sda->index) & 0x3;

    return sprintf(buf,"%s\n",fan_led_mode_str[val]);
}

static ssize_t set_fan_led1_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, mask=0, usr_val=0;
    int i;

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, LED1_DISPLAY_REG);
    reg_val = reg_val & mask;

    for(i=FAN_LED_BASE; i<= FAN_LED_OFF; i++) {
        if(strncmp(buf, fan_led_mode_str[i],strlen(fan_led_mode_str[i]))==0) {
            usr_val = i << sda->index;
            cpld_i2c_write(data, LED1_DISPLAY_REG, reg_val|usr_val);
            break;
        }
    }

    return count;
}


// sysfs attributes
static SENSOR_DEVICE_ATTR(board_type, S_IRUGO, show_board_type, NULL, 0);
static SENSOR_DEVICE_ATTR(fan_version, S_IRUGO, show_fan_cpld_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(fan_reset, S_IRUGO, show_fan_cpld_reset, NULL, FAN_CPLD_RESET_BIT);
static SENSOR_DEVICE_ATTR(fan1_present, S_IRUGO, show_fan_present, NULL, FAN1_PRES);
static SENSOR_DEVICE_ATTR(fan2_present, S_IRUGO, show_fan_present, NULL, FAN2_PRES);
static SENSOR_DEVICE_ATTR(fan3_present, S_IRUGO, show_fan_present, NULL, FAN3_PRES);
static SENSOR_DEVICE_ATTR(fan4_present, S_IRUGO, show_fan_present, NULL, FAN4_PRES);
static SENSOR_DEVICE_ATTR(fan5_present, S_IRUGO, show_fan_present, NULL, FAN5_PRES);
static SENSOR_DEVICE_ATTR(fan6_present, S_IRUGO, show_fan_present, NULL, FAN6_PRES);
static SENSOR_DEVICE_ATTR(fan1_direction, S_IRUGO, show_fan_direction, NULL, FAN1_PRES);
static SENSOR_DEVICE_ATTR(fan2_direction, S_IRUGO, show_fan_direction, NULL, FAN2_PRES);
static SENSOR_DEVICE_ATTR(fan3_direction, S_IRUGO, show_fan_direction, NULL, FAN3_PRES);
static SENSOR_DEVICE_ATTR(fan4_direction, S_IRUGO, show_fan_direction, NULL, FAN4_PRES);
static SENSOR_DEVICE_ATTR(fan5_direction, S_IRUGO, show_fan_direction, NULL, FAN5_PRES);
static SENSOR_DEVICE_ATTR(fan6_direction, S_IRUGO, show_fan_direction, NULL, FAN6_PRES);
static SENSOR_DEVICE_ATTR(fans_pwm, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, 0);
static SENSOR_DEVICE_ATTR(fan1_speed, S_IRUGO, show_fan_speed, NULL, FAN1_ID);
static SENSOR_DEVICE_ATTR(fan2_speed, S_IRUGO, show_fan_speed, NULL, FAN2_ID);
static SENSOR_DEVICE_ATTR(fan3_speed, S_IRUGO, show_fan_speed, NULL, FAN3_ID);
static SENSOR_DEVICE_ATTR(fan4_speed, S_IRUGO, show_fan_speed, NULL, FAN4_ID);
static SENSOR_DEVICE_ATTR(fan5_speed, S_IRUGO, show_fan_speed, NULL, FAN5_ID);
static SENSOR_DEVICE_ATTR(fan6_speed, S_IRUGO, show_fan_speed, NULL, FAN6_ID);
static SENSOR_DEVICE_ATTR(fan7_speed, S_IRUGO, show_fan_speed, NULL, FAN7_ID);
static SENSOR_DEVICE_ATTR(fan8_speed, S_IRUGO, show_fan_speed, NULL, FAN8_ID);
static SENSOR_DEVICE_ATTR(fan9_speed, S_IRUGO, show_fan_speed, NULL, FAN9_ID);
static SENSOR_DEVICE_ATTR(fan10_speed, S_IRUGO, show_fan_speed, NULL, FAN10_ID);
static SENSOR_DEVICE_ATTR(fan11_speed, S_IRUGO, show_fan_speed, NULL, FAN11_ID);
static SENSOR_DEVICE_ATTR(fan12_speed, S_IRUGO, show_fan_speed, NULL, FAN12_ID);
static SENSOR_DEVICE_ATTR(fan1_led, S_IRUGO | S_IWUSR, show_fan_led2_status, set_fan_led2_status, FAN1_LED_REG);
static SENSOR_DEVICE_ATTR(fan2_led, S_IRUGO | S_IWUSR, show_fan_led2_status, set_fan_led2_status, FAN2_LED_REG);
static SENSOR_DEVICE_ATTR(fan3_led, S_IRUGO | S_IWUSR, show_fan_led1_status, set_fan_led1_status, FAN3_LED_REG);
static SENSOR_DEVICE_ATTR(fan4_led, S_IRUGO | S_IWUSR, show_fan_led1_status, set_fan_led1_status, FAN4_LED_REG);
static SENSOR_DEVICE_ATTR(fan5_led, S_IRUGO | S_IWUSR, show_fan_led1_status, set_fan_led1_status, FAN5_LED_REG);
static SENSOR_DEVICE_ATTR(fan6_led, S_IRUGO | S_IWUSR, show_fan_led1_status, set_fan_led1_status, FAN6_LED_REG);


static struct attribute *fan_cpld_attributes[] = {
    &sensor_dev_attr_board_type.dev_attr.attr,
    &sensor_dev_attr_fan_version.dev_attr.attr,
    &sensor_dev_attr_fan_reset.dev_attr.attr,
    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan4_present.dev_attr.attr,
    &sensor_dev_attr_fan5_present.dev_attr.attr,
    &sensor_dev_attr_fan6_present.dev_attr.attr,
    &sensor_dev_attr_fan1_direction.dev_attr.attr,
    &sensor_dev_attr_fan2_direction.dev_attr.attr,
    &sensor_dev_attr_fan3_direction.dev_attr.attr,
    &sensor_dev_attr_fan4_direction.dev_attr.attr,
    &sensor_dev_attr_fan5_direction.dev_attr.attr,
    &sensor_dev_attr_fan6_direction.dev_attr.attr,
    &sensor_dev_attr_fans_pwm.dev_attr.attr,
    &sensor_dev_attr_fan1_speed.dev_attr.attr,
    &sensor_dev_attr_fan2_speed.dev_attr.attr,
    &sensor_dev_attr_fan3_speed.dev_attr.attr,
    &sensor_dev_attr_fan4_speed.dev_attr.attr,
    &sensor_dev_attr_fan5_speed.dev_attr.attr,
    &sensor_dev_attr_fan6_speed.dev_attr.attr,
    &sensor_dev_attr_fan7_speed.dev_attr.attr,
    &sensor_dev_attr_fan8_speed.dev_attr.attr,
    &sensor_dev_attr_fan9_speed.dev_attr.attr,
    &sensor_dev_attr_fan10_speed.dev_attr.attr,
    &sensor_dev_attr_fan11_speed.dev_attr.attr,
    &sensor_dev_attr_fan12_speed.dev_attr.attr,
    &sensor_dev_attr_fan1_led.dev_attr.attr,
    &sensor_dev_attr_fan2_led.dev_attr.attr,
    &sensor_dev_attr_fan3_led.dev_attr.attr,
    &sensor_dev_attr_fan4_led.dev_attr.attr,
    &sensor_dev_attr_fan5_led.dev_attr.attr,
    &sensor_dev_attr_fan6_led.dev_attr.attr,
    NULL
};

static const struct attribute_group fan_cpld_group = {
    .attrs = fan_cpld_attributes,
};

static int fan_cpld_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia FAN CPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &fan_cpld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    return 0;

exit:
    return status;
}

static void fan_cpld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &fan_cpld_group);
    kfree(data);
}

static const struct of_device_id fan_cpld_of_ids[] = {
    {
        .compatible = "nokia,fan_cpld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, fan_cpld_of_ids);

static const struct i2c_device_id fan_cpld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, fan_cpld_ids);

static struct i2c_driver fan_cpld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(fan_cpld_of_ids),
    },
    .probe        = fan_cpld_probe,
    .remove       = fan_cpld_remove,
    .id_table     = fan_cpld_ids,
    .address_list = cpld_address_list,
};

static int __init fan_cpld_init(void)
{
    return i2c_add_driver(&fan_cpld_driver);
}

static void __exit fan_cpld_exit(void)
{
    i2c_del_driver(&fan_cpld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA FAN CPLD driver");
MODULE_LICENSE("GPL");

module_init(fan_cpld_init);
module_exit(fan_cpld_exit);
