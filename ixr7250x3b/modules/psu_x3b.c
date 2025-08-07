// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia X3B PSU Driver
 *
 *  Copyright (C) 2024 Nokia
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/of.h>

#define MAX_FAN_DUTY_CYCLE        100
#define PMBUS_CODE_STATUS_WORD    0x79
#define PSU_DRIVER_NAME           "psu_x3b"

static const unsigned short normal_i2c[] = { 0x5b, I2C_CLIENT_END };

struct x3b_psu_data {
	struct device	*hwmon_dev;
	struct mutex	update_lock;
	char		valid;
	unsigned long	last_updated;	/* In jiffies */

	/* Registers value */
	u8	vout_mode;
	u16	in1_input;
	u16	in2_input;
	u16	curr1_input;
	u16	curr2_input;
	u16	power1_input;
	u16	power2_input;
	u16	temp_input[2];
	u8	fan_target;
	u16	fan_duty_cycle_input[2];
	u16	fan_speed_input[2];
};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask);
static ssize_t set_fan_duty_cycle_input(struct device *dev, struct device_attribute \
										*dev_attr, const char *buf, size_t count);
static ssize_t for_linear_data(struct device *dev, struct device_attribute \
								*dev_attr, char *buf);
static ssize_t for_fan_target(struct device *dev, struct device_attribute \
								*dev_attr, char *buf);
static ssize_t for_vout_data(struct device *dev, struct device_attribute \
							*dev_attr, char *buf);
static int x3b_psu_read_byte(struct i2c_client *client, u8 reg);
static int x3b_psu_read_word(struct i2c_client *client, u8 reg);
static int x3b_psu_write_word(struct i2c_client *client, u8 reg, \
								u16 value);
static struct x3b_psu_data *x3b_psu_update_device(struct device *dev);

enum x3b_psu_sysfs_attributes {
	PSU_V_IN,
	PSU_V_OUT,
	PSU_I_IN,
	PSU_I_OUT,
	PSU_P_IN,
	PSU_P_OUT,
	PSU_TEMP1_INPUT,
	PSU_FAN1_FAULT,
	PSU_FAN1_DUTY_CYCLE,
	PSU_FAN1_SPEED,
};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
	u16  valid_data  = data & mask;
	bool is_negative = valid_data >> (valid_bit - 1);

	return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t set_fan_duty_cycle_input(struct device *dev, struct device_attribute \
				*dev_attr, const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
	struct i2c_client *client = to_i2c_client(dev);
	struct x3b_psu_data *data = i2c_get_clientdata(client);
	int nr = (attr->index == PSU_FAN1_DUTY_CYCLE) ? 0 : 1;
	long speed;
	int error;
	
	error = kstrtol(buf, 10, &speed);
	if (error)
		return error;

	if (speed < 0 || speed > MAX_FAN_DUTY_CYCLE)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->fan_duty_cycle_input[nr] = speed;
	x3b_psu_write_word(client, 0x3B + nr, data->fan_duty_cycle_input[nr]);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t for_linear_data(struct device *dev, struct device_attribute \
							*dev_attr, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
	struct x3b_psu_data *data = x3b_psu_update_device(dev);

	u16 value = 0;
	int exponent, mantissa;
	int multiplier = 1000;
	
	switch (attr->index) {
	case PSU_V_IN:
		value = data->in1_input;
		break;
	case PSU_V_OUT:
		value = data->in2_input;
		break;
	case PSU_I_IN:
		value = data->curr1_input;
		break;
	case PSU_I_OUT:
		value = data->curr2_input;
		break;
	case PSU_P_IN:
		value = data->power1_input;
		multiplier = 1000*1000;
		break;
	case PSU_P_OUT:
		value = data->power2_input;
		multiplier = 1000*1000;
		break;
	case PSU_TEMP1_INPUT:
		value = data->temp_input[0];
		break;
	case PSU_FAN1_DUTY_CYCLE:
		multiplier = 1;
		value = data->fan_duty_cycle_input[0];
		break;
	case PSU_FAN1_SPEED:
		multiplier = 1;
		value = data->fan_speed_input[0];
		break;
	default:
		break;
	}

	exponent = two_complement_to_int(value >> 11, 5, 0x1f);
	mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);

	return (exponent >= 0) ? sprintf(buf, "%d\n",	\
		(mantissa << exponent) * multiplier) :	\
		sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static ssize_t for_fan_target(struct device *dev, struct device_attribute \
							*dev_attr, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
	struct x3b_psu_data *data = x3b_psu_update_device(dev);

	u8 shift = (attr->index == PSU_FAN1_FAULT) ? 7 : 6;

	return sprintf(buf, "%d\n", data->fan_target >> shift);
}

static ssize_t for_vout_data(struct device *dev, struct device_attribute \
		 					*dev_attr, char *buf)
{
	struct x3b_psu_data *data = x3b_psu_update_device(dev);
	int exponent = 0;
	int mantissa = 0;
	int multiplier = 1000;

	exponent = two_complement_to_int(data->vout_mode, 5, 0x1f);
	mantissa = data->in2_input;
 
	return (exponent > 0) ? sprintf(buf, "%d\n", \
		mantissa * (1 << exponent)) : \
		sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static int x3b_psu_read_byte(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static int x3b_psu_read_word(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_word_data(client, reg);
}

static int x3b_psu_write_word(struct i2c_client *client, u8 reg, \
								u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	return i2c_smbus_xfer(client->adapter, client->addr, 
				client->flags |= I2C_CLIENT_PEC,
								I2C_SMBUS_WRITE, reg,
								I2C_SMBUS_WORD_DATA, &data);
}

struct reg_data_byte {
	u8 reg;
	u8 *value;
};

struct reg_data_word {
	u8 reg;
	u16 *value;
};

static struct x3b_psu_data *x3b_psu_update_device( \
							struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct x3b_psu_data *data = i2c_get_clientdata(client);
	
	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated)) {
		int i, status;
		struct reg_data_byte regs_byte[] = {
			{0x20, &data->vout_mode},
			{0x81, &data->fan_target}
		};
		struct reg_data_word regs_word[] = {
			{0x88, &data->in1_input},
			{0x8b, &data->in2_input},
			{0x89, &data->curr1_input},
			{0x8c, &data->curr2_input},
			{0x96, &data->power2_input},
			{0x97, &data->power1_input},
			{0x8d, &(data->temp_input[0])},
			{0x8e, &(data->temp_input[1])},
			{0x3b, &(data->fan_duty_cycle_input[0])},
			{0x90, &(data->fan_speed_input[0])},
		};

		dev_dbg(&client->dev, "start data update\n");

		/* one milliseconds from now */
		data->last_updated = jiffies + HZ / 1000;
		
		for (i = 0; i < ARRAY_SIZE(regs_byte); i++) {
			status = x3b_psu_read_byte(client, 
							regs_byte[i].reg);
			if (status < 0) {
				dev_dbg(&client->dev, "reg %d, err %d\n",
					regs_byte[i].reg, status);
				*(regs_byte[i].value) = 0;
			} else {
				*(regs_byte[i].value) = status;
			}
		}

		for (i = 0; i < ARRAY_SIZE(regs_word); i++) {
			status = x3b_psu_read_word(client,
							regs_word[i].reg);
			if (status < 0) {
				dev_dbg(&client->dev, "reg %d, err %d\n",
					regs_word[i].reg, status);
				*(regs_word[i].value) = 0;
			} else {
				*(regs_word[i].value) = status;
			}
		}
		
		data->valid = 1;
	}
	
	mutex_unlock(&data->update_lock);
	
	return data;
}

static ssize_t psu_status_show(struct device *dev, struct device_attribute \
		 					*dev_attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int retval;

	retval = x3b_psu_read_word(client, PMBUS_CODE_STATUS_WORD);
	
	return sprintf(buf, "%d\n", retval);
}

/* sysfs attributes for hwmon */
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, for_linear_data, NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(in2_input, S_IRUGO, for_vout_data,   NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(curr1_input, S_IRUGO, for_linear_data, NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, for_linear_data, NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, for_linear_data, NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, for_linear_data, NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, for_linear_data, NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_target, S_IRUGO, for_fan_target,	NULL, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(fan1_set_percentage, S_IWUSR | S_IRUGO, \
							for_linear_data, set_fan_duty_cycle_input, PSU_FAN1_DUTY_CYCLE);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, for_linear_data,	NULL, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_status, S_IRUGO, psu_status_show,	NULL, 0);

static struct attribute *x3b_psu_attributes[] = {
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_curr1_input.dev_attr.attr,
	&sensor_dev_attr_curr2_input.dev_attr.attr,
	&sensor_dev_attr_power1_input.dev_attr.attr,
	&sensor_dev_attr_power2_input.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_fan1_target.dev_attr.attr,
	&sensor_dev_attr_fan1_set_percentage.dev_attr.attr,
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_psu_status.dev_attr.attr,
	NULL
};

static const struct attribute_group x3b_psu_group = {
	.attrs = x3b_psu_attributes,
};

static int x3b_psu_probe(struct i2c_client *client)
{
	struct x3b_psu_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter, 
		I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA)) {
		status = -EIO;
		dev_info(&client->dev, "i2c_check_functionality failed\n");
		goto exit;
	}
	
	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		status = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	client->flags |= I2C_CLIENT_SCCB;
	data->valid = 0;
	mutex_init(&data->update_lock);
	
	status = sysfs_create_group(&client->dev.kobj, &x3b_psu_group);
	if (status) 
		goto exit_sysfs_create_group;

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, PSU_DRIVER_NAME, NULL, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_hwmon_device_register;
	}

	return 0;
	
exit_hwmon_device_register:
	sysfs_remove_group(&client->dev.kobj, &x3b_psu_group);
exit_sysfs_create_group:
	kfree(data);
exit:
	return status;
}

static void x3b_psu_remove(struct i2c_client *client)
{
	struct x3b_psu_data *data = i2c_get_clientdata(client);
	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &x3b_psu_group);
	kfree(data);
}

static const struct i2c_device_id x3b_psu_id[] = {
	{ PSU_DRIVER_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, x3b_psu_id);

static struct i2c_driver x3b_psu_driver = {
	.class          = I2C_CLASS_HWMON,
	.driver = {
		.name   = PSU_DRIVER_NAME,
	},
	.probe          = x3b_psu_probe,
	.remove         = x3b_psu_remove,
	.id_table       = x3b_psu_id,
	.address_list   = normal_i2c,
};

static int __init x3b_psu_init(void)
{
	return i2c_add_driver(&x3b_psu_driver);
}

static void __exit x3b_psu_exit(void)
{
	i2c_del_driver(&x3b_psu_driver);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PSU Driver");
MODULE_AUTHOR("Nokia");

module_init(x3b_psu_init);
module_exit(x3b_psu_exit);
