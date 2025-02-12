// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia X3B FAN eeprom decoder
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
#include <linux/delay.h>

#define EEPROM_NAME      "fan_eeprom"
#define EEPROM_LEN       128
#define FIELD_LEN_MAX    16

#define kEeCleiCode      0x1a
#define kMfgAssemblyNum  0x1b
#define kMfgDate         0x17
#define kMfgSerialNum    0x16
#define kMfgPartNum      0x15
#define kHwDirectives    0x05
#define kPlatforms       0x03
#define kHwType          0x01
#define kCSumRec         0x00

static const unsigned short normal_i2c[] = { 0x54, I2C_CLIENT_END };

static unsigned int debug = 0;
module_param_named(debug, debug, uint, 0);
MODULE_PARM_DESC(debug, "Debug enable(default to 0)");

struct menuee_data {
	struct mutex lock;
	struct i2c_client *client;
	char eeprom[EEPROM_LEN];
	char part_number[FIELD_LEN_MAX];
	char mfg_date[FIELD_LEN_MAX];
	char serial_number[FIELD_LEN_MAX];
	char clei[FIELD_LEN_MAX];
	char assembly_num[FIELD_LEN_MAX];
	u32 hw_directives;
	u8 platforms;
	u8 hw_type;
	u8 checksum;
};

int cache_eeprom(struct i2c_client *client)
{
	struct menuee_data *ee_data = i2c_get_clientdata(client);
	int status;

	status = i2c_smbus_write_word_data(client, 0, 0);
	msleep(1);
	for (int i = 0; i < EEPROM_LEN; i++) {
		ee_data->eeprom[i] = i2c_smbus_read_byte(client);
	}

	if (debug)
		print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, ee_data->eeprom, EEPROM_LEN, true);

	return 0;
}

int decode_eeprom(struct i2c_client *client)
{
	struct menuee_data *ee_data = i2c_get_clientdata(client);
	int i = 0;
	u8 len;

	while (i < EEPROM_LEN) {
		switch (ee_data->eeprom[i]) {
		case kEeCleiCode:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->clei[0], &ee_data->eeprom[i], len);
			ee_data->clei[len] = 0;
			i += len;
			break;
		case kMfgDate:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->mfg_date[0], &ee_data->eeprom[i], len);
			ee_data->mfg_date[len] = 0;
			i += len;
			break;
		case kMfgSerialNum:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->serial_number[0], &ee_data->eeprom[i], len);
			ee_data->serial_number[len] = 0;
			i += len;
			break;
		case kMfgPartNum:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->part_number[0], &ee_data->eeprom[i], len);
			ee_data->part_number[len] = 0;
			i += len;
			break;
		case kMfgAssemblyNum:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->assembly_num[0], &ee_data->eeprom[i], len);
			ee_data->assembly_num[len] = 0;
			i += len;
			break;
		case kHwDirectives:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->hw_directives, &ee_data->eeprom[i], len);
			ee_data->hw_directives = swab32(ee_data->hw_directives);
			i += len;
			break;
		case kHwType:
			i++;
			memcpy(&ee_data->hw_type, &ee_data->eeprom[i], 1);
			i++;
			break;
		case kPlatforms:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->platforms, &ee_data->eeprom[i], len);
			i += len;
			break;
		case kCSumRec:
			i++;
			len = ee_data->eeprom[i++];
			memcpy(&ee_data->checksum, &ee_data->eeprom[i], len);
			i += len;
			break;
		default:
			return 0;
		}
	}

	return 0;
}

static ssize_t eeprom_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->eeprom);
}

static ssize_t part_number_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->part_number);
}

static ssize_t serial_number_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->serial_number);
}

static ssize_t mfg_date_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->mfg_date);
}

static ssize_t clei_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->clei);
}

static ssize_t hw_directives_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "0x%x\n", data->hw_directives);
}

static ssize_t hw_type_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "0x%x\n", data->hw_type);
}

static ssize_t platforms_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "0x%x\n", data->platforms);
}

static ssize_t assembly_num_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct menuee_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->assembly_num);
}

static DEVICE_ATTR_RO(eeprom);
static DEVICE_ATTR_RO(part_number);
static DEVICE_ATTR_RO(serial_number);
static DEVICE_ATTR_RO(mfg_date);
static DEVICE_ATTR_RO(clei);
static DEVICE_ATTR_RO(hw_directives);
static DEVICE_ATTR_RO(hw_type);
static DEVICE_ATTR_RO(platforms);
static DEVICE_ATTR_RO(assembly_num);

static struct attribute *eeprom_attributes[] = {
	&dev_attr_eeprom.attr,
	&dev_attr_part_number.attr,
	&dev_attr_serial_number.attr,
	&dev_attr_mfg_date.attr,
	&dev_attr_clei.attr,
	&dev_attr_hw_directives.attr,
	&dev_attr_hw_type.attr,
	&dev_attr_platforms.attr,
	&dev_attr_assembly_num.attr,
	NULL
};

static const struct attribute_group eeprom_group = {
	.attrs = eeprom_attributes,
};

static int eeprom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct menuee_data *data;
	int status;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
									I2C_FUNC_SMBUS_WORD_DATA )) {
		dev_info(&client->dev, "i2c_check_functionality failed!\n");
		return -EIO;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	//data->client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->lock);

	status = sysfs_create_group(&client->dev.kobj, &eeprom_group);
	if (status) {
		dev_err(&client->dev, "Cannot create sysfs\n");
		kfree(data);
		return status;
	}
	data->client = client;

	cache_eeprom(client);
	decode_eeprom(client);
	
	return 0;
}

static void eeprom_remove(struct i2c_client *client)
{
	struct menuee_data *data = i2c_get_clientdata(client);
	sysfs_remove_group(&client->dev.kobj, &eeprom_group);
	kfree(data);
	return;
}

static const struct i2c_device_id eeprom_id[] = {
	{ EEPROM_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, eeprom_id);

static struct i2c_driver eeprom_driver = {
		.driver = {
					.name = EEPROM_NAME,
				},
		.probe            = eeprom_probe,
		.remove           = eeprom_remove,
		.id_table         = eeprom_id,
		.address_list     = normal_i2c,
};

static int __init fan_eeprom_init(void)
{
	return i2c_add_driver(&eeprom_driver);
}

static void __exit fan_eeprom_exit(void)
{
	i2c_del_driver(&eeprom_driver);
}

MODULE_DESCRIPTION("FAN eeprom sysfs driver");
MODULE_AUTHOR("Nokia");
MODULE_LICENSE("GPL");

module_init(fan_eeprom_init);
module_exit(fan_eeprom_exit);