// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia 7250-IXR X Fan LED Driver
*
*  Copyright (C) 2024 Nokia
*
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define DRIVER_NAME "fan_led"

// REGISTERS ADDRESS MAP
#define REG_MODE1       0x0
#define REG_MODE2       0x1
#define REG_PWM0        0x2
#define REG_PWM1        0x3
#define REG_PWM2        0x4
#define REG_PWM3        0x5
#define REG_GRPPWM      0x6
#define REG_GRPFREQ     0x7
#define REG_LEDOUT      0x8

#define MODE1_VALUE     0x0
#define MODE2_VALUE     0x34
#define LED_OFF         0x40
#define LED_ON          0x6a
#define LED_BLINK       0x7f

static const unsigned short led_address_list[] = {0x60, I2C_CLIENT_END};

struct fan_led_data {
	struct i2c_client *client;
	struct mutex  update_lock;
	int fan_led;
};

static void smbus_i2c_write(struct fan_led_data *data, u8 reg, u8 value)
{
	int res = 0;
	struct i2c_client *client = data->client;

	mutex_lock(&data->update_lock);
	res = i2c_smbus_write_byte_data(client, reg, value);
	if (res < 0) {
		dev_err(&client->dev, "I2C WRITE ERROR: reg(0x%02x) err %d\n", reg, res);
	}
	mutex_unlock(&data->update_lock);
}

static ssize_t fan_led_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct fan_led_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->fan_led);
}

static void set_mode(struct fan_led_data *data)
{
	smbus_i2c_write(data, REG_MODE1, MODE1_VALUE);
	smbus_i2c_write(data, REG_MODE2, MODE2_VALUE);
}

static ssize_t fan_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
	struct fan_led_data *data = dev_get_drvdata(dev);
	u8 usr_val = 0;

	int ret = kstrtou8(buf, 10, &usr_val);
	if (ret != 0) {
		return ret; 
	}
	if (usr_val > 2) {
		return -EINVAL;
	}

	set_mode(data);
	switch (usr_val) {
	case 0:
		smbus_i2c_write(data, REG_LEDOUT, LED_OFF);
		data->fan_led = 0;
		break;		
	case 1:
		smbus_i2c_write(data, REG_PWM0, 0x00);
		smbus_i2c_write(data, REG_PWM1, 0xff);
		smbus_i2c_write(data, REG_PWM2, 0x00);
		smbus_i2c_write(data, REG_LEDOUT, LED_ON);
		data->fan_led = 1;
		break;
	case 2:
		smbus_i2c_write(data, REG_PWM0, 0xff);
		smbus_i2c_write(data, REG_PWM1, 0x3f);
		smbus_i2c_write(data, REG_PWM2, 0x00);
		smbus_i2c_write(data, REG_LEDOUT, LED_ON);
		data->fan_led = 2;
		break;
	default:
		smbus_i2c_write(data, REG_LEDOUT, LED_OFF);
	}

	return count;
}

// sysfs attributes 
static DEVICE_ATTR_RW(fan_led);

static struct attribute *fan_led_attributes[] = {
	&dev_attr_fan_led.attr,
	NULL
};

static const struct attribute_group fan_led_group = {
	.attrs = fan_led_attributes,
};

static int fan_led_probe(struct i2c_client *client)
{
	int status;
	struct fan_led_data *data = NULL;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "Fan_LED PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
		status = -EIO;
	goto exit;
	}

	dev_info(&client->dev, "Nokia Fan_LED driver found.\n");
	data = kzalloc(sizeof(struct fan_led_data), GFP_KERNEL);

	if (!data) {
		dev_err(&client->dev, "Fan_LED PROBE ERROR: Can't allocate memory\n");
		status = -ENOMEM;
		goto exit;
	}

	data->client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	status = sysfs_create_group(&client->dev.kobj, &fan_led_group);
	if (status) {
		dev_err(&client->dev, "Fan_LED INIT ERROR: Cannot create sysfs\n");
		goto exit;
	}

	set_mode(data);
	smbus_i2c_write(data, REG_PWM0, 0x00);
	smbus_i2c_write(data, REG_PWM1, 0xff);
	smbus_i2c_write(data, REG_PWM2, 0x00);
	smbus_i2c_write(data, REG_GRPPWM, 0x80);
	smbus_i2c_write(data, REG_GRPFREQ, 0x19);
	smbus_i2c_write(data, REG_LEDOUT, LED_BLINK);
	data->fan_led = 3;

	return 0;

exit:
	return status;
}

static void fan_led_remove(struct i2c_client *client)
{
	struct fan_led_data *data = i2c_get_clientdata(client);
	sysfs_remove_group(&client->dev.kobj, &fan_led_group);
	kfree(data);
}

static const struct i2c_device_id fan_led_id[] = {
	{ DRIVER_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, fan_led_id);

static struct i2c_driver fan_led_driver = {
	.driver = {
		.name    = DRIVER_NAME,
	},
	.probe        = fan_led_probe,
	.remove       = fan_led_remove,
	.id_table     = fan_led_id,
	.address_list = led_address_list,
};

static int __init fan_led_init(void)
{
	return i2c_add_driver(&fan_led_driver);
}

static void __exit fan_led_exit(void)
{
	i2c_del_driver(&fan_led_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA Fan LED driver");
MODULE_LICENSE("GPL");

module_init(fan_led_init);
module_exit(fan_led_exit);