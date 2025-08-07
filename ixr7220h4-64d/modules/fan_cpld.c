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
#define MINOR_REV_REG               0x00
#define MAJOR_REV_REG               0x01
#define PCB_VERSION_REG             0x02
#define SCRATCH_REG                 0x04
#define TEMP_SENSOR_REG             0x08
#define LED_STATUS_REG              0x09
#define FAN_PRESENCE_REG            0x10
#define EEPROM_PROTECT_REG          0x18
#define FAN_ENABLE_REG              0x1A
#define FAN_ENABLE_PROTECT_REG      0x1B
#define WATCHDOG_ENABLE_REG         0x20
#define WATCHDOG_TIMER_REG          0x21
#define FAN1_PWM_REG                0x30
#define FAN3_PWM_REG                0x31
#define FAN5_PWM_REG                0x32
#define FAN7_PWM_REG                0x33
#define FAN1_SPEED_REG              0x40
#define FAN2_SPEED_REG              0x50
#define FAN3_SPEED_REG              0x41
#define FAN4_SPEED_REG              0x51
#define FAN5_SPEED_REG              0x42
#define FAN6_SPEED_REG              0x52
#define FAN7_SPEED_REG              0x43
#define FAN8_SPEED_REG              0x53
#define FAN_HITLESS_REG             0x60
#define FAN_MISC_REG                0x61

// REG BIT FIELD POSITION or MASK
#define BOARD_INFO_REG_TYPE_MSK     0xF

#define FAN1_LED_REG                0x0
#define FAN2_LED_REG                0x2
#define FAN3_LED_REG                0x4
#define FAN4_LED_REG                0x6

#define FAN1_PRESENCE_REG           0x0
#define FAN2_PRESENCE_REG           0x1
#define FAN3_PRESENCE_REG           0x2
#define FAN4_PRESENCE_REG           0x3

#define FAN1_POWER_REG              0x0
#define FAN2_POWER_REG              0x1
#define FAN3_POWER_REG              0x2
#define FAN4_POWER_REG              0x3

#define FAN1_REG                    0x0
#define FAN2_REG                    0x1
#define FAN3_REG                    0x2
#define FAN4_REG                    0x3
#define FAN5_REG                    0x4
#define FAN6_REG                    0x5
#define FAN7_REG                    0x6
#define FAN8_REG                    0x7

static const unsigned short cpld_address_list[] = {0x33, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
};

enum fan_led_mode {
    FAN_LED_OFF,
    FAN_LED_GREEN,
    FAN_LED_RED,
    FAN_LED_BASE
};
char *fan_led_mode_str[]={"off", "green", "red", "base"};

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

static ssize_t show_board_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *str_ver = NULL;
    u8 val = 0;

    val = (cpld_i2c_read(data, PCB_VERSION_REG))& BOARD_INFO_REG_TYPE_MSK;    
    switch (val) {
    case 0:
        str_ver = "R0A";
        break; 
    case 1:
        str_ver = "R0B";
        break; 
    case 2:
        str_ver = "R0C";
        break; 
    case 4:
        str_ver = "R0D";
        break;
    case 5:
        str_ver = "R01";
        break;     
    default:
        str_ver = "Unknown";
        break;
    }

    return sprintf(buf, "0x%x %s\n", val, str_ver);
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

static ssize_t show_fan_led_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = cpld_i2c_read(data, LED_STATUS_REG);
    val = (val >> sda->index) & 0x3;

    return sprintf(buf,"%s\n",fan_led_mode_str[val]);
}

static ssize_t set_fan_led_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val=0, mask=0, usr_val=0;
    int i;

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, LED_STATUS_REG);
    reg_val = reg_val & mask;

    for(i=FAN_LED_OFF; i<FAN_LED_BASE; i++) {
        if(strncmp(buf, fan_led_mode_str[i],strlen(fan_led_mode_str[i]))==0) {
            usr_val = i << sda->index;
            cpld_i2c_write(data, LED_STATUS_REG, reg_val|usr_val);
            break;
        }
    }

    return count;
}

static ssize_t show_fan_present(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = cpld_i2c_read(data, FAN_PRESENCE_REG);
    
    /* If the bit is set, fan is not present. So, we are toggling intentionally */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 0:1);
}

static ssize_t show_fan_power_status(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = cpld_i2c_read(data, FAN_ENABLE_REG);
    
    /* If the bit is set, fan is disabled */
    return sprintf(buf,"%d\n",(val>>sda->index) & 0x1 ? 0:1);
}

static ssize_t set_fan_power_status(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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

    mask = (~(1 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, FAN_ENABLE_REG);
    reg_val = reg_val & mask;

    usr_val = (!usr_val) << sda->index;

    cpld_i2c_write(data, FAN_ENABLE_REG, (reg_val|usr_val));

    return count;

}

static ssize_t show_fan_pwm(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 pwm_reg = 0;
    switch (sda->index) {
    case 0: case 1:
        pwm_reg = FAN1_PWM_REG;
        break; 
    case 2: case 3:
        pwm_reg = FAN3_PWM_REG;
        break; 
    case 4: case 5:
        pwm_reg = FAN5_PWM_REG;
        break; 
    case 6: case 7:
        pwm_reg = FAN7_PWM_REG;
        break;
    }

    val = cpld_i2c_read(data, pwm_reg);

    return sprintf(buf, "%d\n", val);
}

static ssize_t set_fan_pwm(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 usr_val = 0;
    u8 pwm_reg = 0;

    int ret = kstrtou8(buf, 0, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (usr_val > 0xFF) {
        return -EINVAL;
    }
 
    switch (sda->index) {
    case 0: case 1:
        pwm_reg = FAN1_PWM_REG;
        break; 
    case 2: case 3:
        pwm_reg = FAN3_PWM_REG;
        break; 
    case 4: case 5:
        pwm_reg = FAN5_PWM_REG;
        break; 
    case 6: case 7:
        pwm_reg = FAN7_PWM_REG;
        break;
    }

    cpld_i2c_write(data, pwm_reg, usr_val);

    return count;
}

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
        speed_reg = FAN2_SPEED_REG;
        break;
    case 2:
        speed_reg = FAN3_SPEED_REG;
        break;
    case 3:
        speed_reg = FAN4_SPEED_REG;
        break;
    case 4:
        speed_reg = FAN5_SPEED_REG;
        break;
    case 5:
        speed_reg = FAN6_SPEED_REG;
        break;
    case 6:
        speed_reg = FAN7_SPEED_REG;
        break;
    case 7:
        speed_reg = FAN8_SPEED_REG;
        break;

    }

    val = cpld_i2c_read(data, speed_reg);
    val_rpm = (val*60000)/1048; 

    return sprintf(buf, "%d\n", val_rpm);
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(board_ver, S_IRUGO, show_board_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);
static SENSOR_DEVICE_ATTR(fan1_led, S_IRUGO | S_IWUSR, show_fan_led_status, set_fan_led_status, FAN1_LED_REG);
static SENSOR_DEVICE_ATTR(fan2_led, S_IRUGO | S_IWUSR, show_fan_led_status, set_fan_led_status, FAN2_LED_REG);
static SENSOR_DEVICE_ATTR(fan3_led, S_IRUGO | S_IWUSR, show_fan_led_status, set_fan_led_status, FAN3_LED_REG);
static SENSOR_DEVICE_ATTR(fan4_led, S_IRUGO | S_IWUSR, show_fan_led_status, set_fan_led_status, FAN4_LED_REG);
static SENSOR_DEVICE_ATTR(fan1_present, S_IRUGO, show_fan_present, NULL, FAN1_PRESENCE_REG);
static SENSOR_DEVICE_ATTR(fan2_present, S_IRUGO, show_fan_present, NULL, FAN2_PRESENCE_REG);
static SENSOR_DEVICE_ATTR(fan3_present, S_IRUGO, show_fan_present, NULL, FAN3_PRESENCE_REG);
static SENSOR_DEVICE_ATTR(fan4_present, S_IRUGO, show_fan_present, NULL, FAN4_PRESENCE_REG);
static SENSOR_DEVICE_ATTR(fan1_power, S_IRUGO | S_IWUSR, show_fan_power_status, set_fan_power_status, FAN1_POWER_REG);
static SENSOR_DEVICE_ATTR(fan2_power, S_IRUGO | S_IWUSR, show_fan_power_status, set_fan_power_status, FAN2_POWER_REG);
static SENSOR_DEVICE_ATTR(fan3_power, S_IRUGO | S_IWUSR, show_fan_power_status, set_fan_power_status, FAN3_POWER_REG);
static SENSOR_DEVICE_ATTR(fan4_power, S_IRUGO | S_IWUSR, show_fan_power_status, set_fan_power_status, FAN4_POWER_REG);
static SENSOR_DEVICE_ATTR(pwm1, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN1_REG);
static SENSOR_DEVICE_ATTR(pwm2, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN2_REG);
static SENSOR_DEVICE_ATTR(pwm3, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN3_REG);
static SENSOR_DEVICE_ATTR(pwm4, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN4_REG);
static SENSOR_DEVICE_ATTR(pwm5, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN5_REG);
static SENSOR_DEVICE_ATTR(pwm6, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN6_REG);
static SENSOR_DEVICE_ATTR(pwm7, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN7_REG);
static SENSOR_DEVICE_ATTR(pwm8, S_IRUGO | S_IWUSR, show_fan_pwm, set_fan_pwm, FAN8_REG);
static SENSOR_DEVICE_ATTR(fan1_speed, S_IRUGO, show_fan_speed, NULL, FAN1_REG);
static SENSOR_DEVICE_ATTR(fan2_speed, S_IRUGO, show_fan_speed, NULL, FAN2_REG);
static SENSOR_DEVICE_ATTR(fan3_speed, S_IRUGO, show_fan_speed, NULL, FAN3_REG);
static SENSOR_DEVICE_ATTR(fan4_speed, S_IRUGO, show_fan_speed, NULL, FAN4_REG);
static SENSOR_DEVICE_ATTR(fan5_speed, S_IRUGO, show_fan_speed, NULL, FAN5_REG);
static SENSOR_DEVICE_ATTR(fan6_speed, S_IRUGO, show_fan_speed, NULL, FAN6_REG);
static SENSOR_DEVICE_ATTR(fan7_speed, S_IRUGO, show_fan_speed, NULL, FAN7_REG);
static SENSOR_DEVICE_ATTR(fan8_speed, S_IRUGO, show_fan_speed, NULL, FAN8_REG);

static struct attribute *fan_cpld_attributes[] = {
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_fan1_led.dev_attr.attr,
    &sensor_dev_attr_fan2_led.dev_attr.attr,
    &sensor_dev_attr_fan3_led.dev_attr.attr,
    &sensor_dev_attr_fan4_led.dev_attr.attr,
    &sensor_dev_attr_fan1_present.dev_attr.attr,
    &sensor_dev_attr_fan2_present.dev_attr.attr,
    &sensor_dev_attr_fan3_present.dev_attr.attr,
    &sensor_dev_attr_fan4_present.dev_attr.attr,
    &sensor_dev_attr_fan1_power.dev_attr.attr,
    &sensor_dev_attr_fan2_power.dev_attr.attr,
    &sensor_dev_attr_fan3_power.dev_attr.attr,
    &sensor_dev_attr_fan4_power.dev_attr.attr,
    &sensor_dev_attr_pwm1.dev_attr.attr,
    &sensor_dev_attr_pwm2.dev_attr.attr,
    &sensor_dev_attr_pwm3.dev_attr.attr,
    &sensor_dev_attr_pwm4.dev_attr.attr,
    &sensor_dev_attr_pwm5.dev_attr.attr,
    &sensor_dev_attr_pwm6.dev_attr.attr,
    &sensor_dev_attr_pwm7.dev_attr.attr,
    &sensor_dev_attr_pwm8.dev_attr.attr,
    &sensor_dev_attr_fan1_speed.dev_attr.attr,
    &sensor_dev_attr_fan2_speed.dev_attr.attr,
    &sensor_dev_attr_fan3_speed.dev_attr.attr,
    &sensor_dev_attr_fan4_speed.dev_attr.attr,
    &sensor_dev_attr_fan5_speed.dev_attr.attr,
    &sensor_dev_attr_fan6_speed.dev_attr.attr,
    &sensor_dev_attr_fan7_speed.dev_attr.attr,
    &sensor_dev_attr_fan8_speed.dev_attr.attr,
  
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
