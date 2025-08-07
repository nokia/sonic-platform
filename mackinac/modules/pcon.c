// SPDX-License-Identifier: GPL-2.0-only
/*
* Copyright (C) 2025 Nokia
*
* Based on ad7418.c
* Copyright (C) 2006-07 Tower Technologies
*/

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include "pcon.h"

#define DRV_VERSION "0.1"

enum chips { pcon, pconm };

struct pcon_data {
	struct i2c_client	*client;
	const struct attribute_group **groups;
	enum chips			type;
	struct mutex		lock;
	u16                 version;
	u16                 revision;
	int					num_channels;
	char				valid;
};

static ssize_t version_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	struct pcon_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->version);
}

static ssize_t revision_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	struct pcon_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->revision);
}

static ssize_t imb_volt_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	s32 reg_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	reg_data = i2c_smbus_read_word_data(data->client, PCON_IMBV_VOLT_VALUE_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}
	else {
		return sprintf(buf, "%d\n", (reg_data & M_IMBV_VOLT_VALUE_REG_IMB_VOLT) >> S_IMBV_VOLT_VALUE_REG_IMB_VOLT);
	}
}

static ssize_t imb_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	s32 reg_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	reg_data = i2c_smbus_read_word_data(data->client, PCON_IMBV_VOLT_VALUE_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}
	else {
		return sprintf(buf, "%d\n", (reg_data & M_IMBV_VOLT_VALUE_REG_IMB) >> S_IMBV_VOLT_VALUE_REG_IMB);
	}
}

static ssize_t imb_uv_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	s32 reg_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	reg_data = i2c_smbus_read_word_data(data->client, PCON_IMBV_ERROR_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}
	else {
		return sprintf(buf, "%d\n", (reg_data & M_IMBV_ERROR_REG_IMBV_UV) >> S_IMBV_ERROR_REG_IMBV_UV);
	}
}

static ssize_t imb_ov_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	s32 reg_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	reg_data = i2c_smbus_read_word_data(data->client, PCON_IMBV_ERROR_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}
	else {
		return sprintf(buf, "%d\n", (reg_data & M_IMBV_ERROR_REG_IMBV_OV) >> S_IMBV_ERROR_REG_IMBV_OV);
	}
}

static ssize_t spi_i2c_select_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	s32 reg_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	reg_data = i2c_smbus_read_word_data(data->client, PCON_SPI_SELECT_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}
	else {
		return sprintf(buf, "%d\n", (reg_data & M_SPI_SELECT_REG_SPI_I2C_SELECT) >> S_SPI_SELECT_REG_SPI_I2C_SELECT);
	}
}

static ssize_t spi_i2c_select_store(struct device *dev, struct device_attribute *devattr,
	const char *buf, size_t count)
{
	long val;
	s32 reg_data;
	u16 reg_val_masked;
	struct pcon_data *data = dev_get_drvdata(dev);
	int ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val != 0 && val != 1) {
		dev_err(dev, "spi_i2c_select only supports 0 or 1; got %li", val);
		return -EINVAL;
	}

	reg_data = i2c_smbus_read_word_data(data->client, PCON_SPI_SELECT_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return -EBUSY;
	}

	reg_val_masked = reg_data & ~(M_SPI_SELECT_REG_EVENT_CFG_SELECT | M_SPI_SELECT_REG_SPI_I2C_SELECT);
	if (val)
		reg_val_masked |= M_SPI_SELECT_REG_SPI_I2C_SELECT;

	ret = i2c_smbus_write_word_data(data->client, PCON_SPI_SELECT_REG, reg_val_masked);
	if (ret < 0) {
		dev_err(dev, "error writing register");
		return -EBUSY;
	}
	else {
		return count;
	}	
}

static ssize_t event_cfg_select_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	s32 reg_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	reg_data = i2c_smbus_read_word_data(data->client, PCON_SPI_SELECT_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}
	else {
		return sprintf(buf, "%d\n", (reg_data & M_SPI_SELECT_REG_EVENT_CFG_SELECT) >> S_SPI_SELECT_REG_SPI_I2C_SELECT);
	}
}

static ssize_t event_cfg_select_store(struct device *dev, struct device_attribute *devattr,
	const char *buf, size_t count)
{
	long val;
	s32 reg_data;
	u16 reg_val_masked;
	struct pcon_data *data = dev_get_drvdata(dev);
	int ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val != 0 && val != 1) {
		dev_err(dev, "spi_i2c_select only supports 0 or 1; got %li", val);
		return -EINVAL;
	}

	reg_data = i2c_smbus_read_word_data(data->client, PCON_SPI_SELECT_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return -EBUSY;
	}

	reg_val_masked = reg_data & ~(M_SPI_SELECT_REG_EVENT_CFG_SELECT | M_SPI_SELECT_REG_SPI_I2C_SELECT);
	if (val)
		reg_val_masked |= M_SPI_SELECT_REG_EVENT_CFG_SELECT | M_SPI_SELECT_REG_SPI_I2C_SELECT;

	ret = i2c_smbus_write_word_data(data->client, PCON_SPI_SELECT_REG, reg_val_masked);
	if (ret < 0) {
		dev_err(dev, "error writing register");
		return -EBUSY;
	}
	else {
		return count;
	}	
}

static ssize_t uptime_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	s32 lsw_data, msw_data;
	u32 uptime_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	lsw_data = i2c_smbus_read_word_data(data->client, PCON_UP_TIMER_LSW);
	if (lsw_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}

	msw_data = i2c_smbus_read_word_data(data->client, PCON_UP_TIMER_MSW);
	if (msw_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
		return sprintf(buf, "%d\n", -1);
	}

	uptime_data = lsw_data | (msw_data << 16);
	return sprintf(buf, "%d\n", uptime_data);
}

static ssize_t global_show(struct device *dev, struct device_attribute *devattr,
	char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct pcon_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	u32 val = i2c_smbus_read_word_data(client, attr->index);

	if (val < 0) {
		dev_err(dev, "error reading or writing register over i2c");		
		return -EBUSY;
	}
	else {
		dev_dbg(dev, "reading register %i with value 0x%04x",
			attr->index, val);
		return sprintf(buf, "0x%04x\n",val);
	}
}

static ssize_t global_store(struct device *dev,
	struct device_attribute *devattr, const char *buf,
	size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct pcon_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	long val;
	int ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return ret;

	ret = i2c_smbus_write_word_data(client, attr->index, val);
	if (ret < 0) {
		dev_err(dev, "error reading or writing register over i2c");		
		return -EBUSY;
	}
	else {
		dev_dbg(dev, "wrote register %i with value 0x%04x",
			attr->index, (u32)val);
		return count;
	}
}

static ssize_t __channel_select_store(struct device *dev, u16 channel)
{
	u32 reg_data;
	struct pcon_data *data = dev_get_drvdata(dev);

	if (channel >= data->num_channels)
		return -EINVAL;

	reg_data = ((channel << S_CHANNEL_SELECT_REG_CH_SEL) & M_CHANNEL_SELECT_REG_CH_SEL);
	reg_data |= ((~channel << S_CHANNEL_SELECT_REG_INV_CH_SEL) & M_CHANNEL_SELECT_REG_INV_CH_SEL);

	return i2c_smbus_write_word_data(data->client, PCON_CHANNEL_SELECT_REG, reg_data);
}

static ssize_t channel_show(struct device *dev, struct device_attribute *devattr,
			char *buf)
{
	struct sensor_device_attribute_2 *attr = to_sensor_dev_attr_2(devattr);
	struct pcon_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	u32 val;

	mutex_lock(&data->lock);
	val = __channel_select_store(dev, attr->index);
	if (val < 0) goto release_lock;

	val = i2c_smbus_read_word_data(client, attr->nr);
release_lock:
	mutex_unlock(&data->lock);

	if (val < 0) {
		dev_err(dev, "error reading or writing register over i2c");		
		return -EBUSY;
	}
	else {
		return sprintf(buf, "0x%04x\n",val);
	}
}

static ssize_t channel_store(struct device *dev,
			struct device_attribute *devattr, const char *buf,
			size_t count)
{
	struct sensor_device_attribute_2 *attr = to_sensor_dev_attr_2(devattr);
	struct pcon_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	long val;
	int ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return ret;

	mutex_lock(&data->lock);
	ret = __channel_select_store(dev, attr->index);
	if (ret < 0) goto release_lock;

	ret = i2c_smbus_write_word_data(client,attr->nr, val);
release_lock:
	mutex_unlock(&data->lock);
	if (ret < 0) {
		dev_err(dev, "error reading or writing register over i2c");		
		return -EBUSY;
	}
	else {
		return count;
	}
}

static DEVICE_ATTR_RO(version);
static DEVICE_ATTR_RO(revision);
static DEVICE_ATTR_RO(imb_volt);
static DEVICE_ATTR_RO(imb);
static DEVICE_ATTR_RO(imb_uv);
static DEVICE_ATTR_RO(imb_ov);
static DEVICE_ATTR_RW(spi_i2c_select);
static DEVICE_ATTR_RW(event_cfg_select);
static DEVICE_ATTR_RO(uptime);
static SENSOR_DEVICE_ATTR_RW(version_id_reg, global, PCON_VERSION_ID_REG);
static SENSOR_DEVICE_ATTR_RW(imb_volt_value_reg, global, PCON_IMBV_VOLT_VALUE_REG);
static SENSOR_DEVICE_ATTR_RW(imbv_error_reg, global, PCON_IMBV_ERROR_REG);
static SENSOR_DEVICE_ATTR_RW(channel_select_reg, global, PCON_CHANNEL_SELECT_REG);
static SENSOR_DEVICE_ATTR_RW(spi_select_reg, global, PCON_SPI_SELECT_REG);
static SENSOR_DEVICE_ATTR_RO(up_timer_lsw, global, PCON_UP_TIMER_LSW);
static SENSOR_DEVICE_ATTR_RO(up_timer_msw, global, PCON_UP_TIMER_MSW);


static struct attribute *pcon_global_attrs[] = {
	&dev_attr_version.attr,
	&dev_attr_revision.attr,
	&dev_attr_imb_volt.attr,
	&dev_attr_imb.attr,
	&dev_attr_imb_uv.attr,
	&dev_attr_imb_ov.attr,
	&dev_attr_spi_i2c_select.attr,
	&dev_attr_event_cfg_select.attr,
	&dev_attr_uptime.attr,
	&sensor_dev_attr_version_id_reg.dev_attr.attr,
	&sensor_dev_attr_imb_volt_value_reg.dev_attr.attr,
	&sensor_dev_attr_imbv_error_reg.dev_attr.attr,
	&sensor_dev_attr_channel_select_reg.dev_attr.attr,
	&sensor_dev_attr_spi_select_reg.dev_attr.attr,
	&sensor_dev_attr_up_timer_lsw.dev_attr.attr,
	&sensor_dev_attr_up_timer_msw.dev_attr.attr,
	NULL
};

static const struct attribute_group pcon_global_group = {
	.attrs = pcon_global_attrs,
};

static SENSOR_DEVICE_ATTR_2_RW(volt_set_inv_reg, channel, PCON_VOLT_SET_INV_REG, 0);
static SENSOR_DEVICE_ATTR_2_RW(volt_set_reg, channel, PCON_VOLT_SET_REG, 0);
static SENSOR_DEVICE_ATTR_2_RW(under_volt_set_inv_reg, channel, PCON_UNDER_VOLT_SET_INV_REG, 0);
static SENSOR_DEVICE_ATTR_2_RW(under_volt_set_reg, channel, PCON_UNDER_VOLT_SET_REG, 0);
static SENSOR_DEVICE_ATTR_2_RW(over_volt_set_inv_reg, channel, PCON_OVER_VOLT_SET_INV_REG, 0);
static SENSOR_DEVICE_ATTR_2_RW(over_volt_set_reg, channel, PCON_OVER_VOLT_SET_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(measured_volt_reg, channel, PCON_MEASURED_VOLT_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(measured_current_reg, channel, PCON_MEASURED_CURRENT_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(current_multiplier_reg, channel, PCON_CURRENT_MULTIPLIER_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(start_time_reg, channel, PCON_START_TIME_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(volt_ramp_reg, channel, PCON_VOLT_RAMP_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(max_current_reg, channel, PCON_MAX_CURRENT_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(phase_offset_reg, channel, PCON_PHASE_OFFSET_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(volt_trim_allowance_reg, channel, PCON_VOLT_TRIM_ALLOWANCE_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(b0_coeff_reg, channel, PCON_B0_COEFF_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(b1_coeff_reg, channel, PCON_B1_COEFF_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(b2_coeff_reg, channel, PCON_B2_COEFF_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(a1_coeff_reg, channel, PCON_A1_COEFF_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(a2_coeff_reg, channel, PCON_A2_COEFF_REG, 0);
static SENSOR_DEVICE_ATTR_2_RO(misc_reg, channel, PCON_MISC_REG, 0);

static struct sensor_device_attribute_2 *channel_attrs_2[] = {
	&sensor_dev_attr_volt_set_inv_reg,
	&sensor_dev_attr_volt_set_reg,
	&sensor_dev_attr_under_volt_set_inv_reg,
	&sensor_dev_attr_under_volt_set_reg,
	&sensor_dev_attr_over_volt_set_inv_reg,
	&sensor_dev_attr_over_volt_set_reg,
	&sensor_dev_attr_measured_volt_reg,
	&sensor_dev_attr_measured_current_reg,
	&sensor_dev_attr_current_multiplier_reg,
	&sensor_dev_attr_start_time_reg,
	&sensor_dev_attr_volt_ramp_reg,
	&sensor_dev_attr_max_current_reg,
	&sensor_dev_attr_phase_offset_reg,
	&sensor_dev_attr_volt_trim_allowance_reg,
	&sensor_dev_attr_b0_coeff_reg,
	&sensor_dev_attr_b1_coeff_reg,
	&sensor_dev_attr_b2_coeff_reg,
	&sensor_dev_attr_a1_coeff_reg,
	&sensor_dev_attr_a2_coeff_reg,
	&sensor_dev_attr_misc_reg,
};

static void pcon_init_client(struct i2c_client *client)
{
	//struct pcon_data *data = i2c_get_clientdata(client);
}

static const struct i2c_device_id pcon_id[];

static int __pcon_data_init(struct device *dev, struct pcon_data *data)
{
	s32 reg_data;

	reg_data = i2c_smbus_read_word_data(data->client, PCON_VERSION_ID_REG);
	if (reg_data < 0) {
		dev_err(dev, "%s error reading register", data->client->name);
	}
	else {
		data->version = (reg_data & M_VERSION_ID_REG_VERSION) >> S_VERSION_ID_REG_VERSION;
		data->revision = (reg_data & M_VERSION_ID_REG_REVISION) >> S_VERSION_ID_REG_REVISION;
	}

	switch(data->type) {
	case pcon:
		data->num_channels = PCON_MAX_CHANNELS_PER_DEV;
		break;
	case pconm:
		data->num_channels = PCONM_MAX_CHANNELS_PER_DEV;
		break;
	default:
		data->num_channels = 0;
		dev_warn(dev, "unknown pcon type %i", data->type);
	}

	data->valid = 1;
	return 0;
}

static int __pcon_create_attribute_groups(struct device *dev, struct pcon_data *data)
{
	struct attribute_group *dynamic_groups;
	struct sensor_device_attribute_2 *dynamic_attrs;
	struct attribute **attrs;

	int dynamic_attrcount = (ARRAY_SIZE(channel_attrs_2)) * data->num_channels;

	int attrcount = (ARRAY_SIZE(channel_attrs_2) + 1) * data->num_channels;

	int dynamic_groupcount =  data->num_channels;

	int groupcount = 1 + data->num_channels + 1;

	int channel, channel_attrnum;
	int dynamic_groupidx, dynamic_attridx;

	dynamic_attrs = devm_kcalloc(dev, dynamic_attrcount, sizeof(*dynamic_attrs), GFP_KERNEL);
	if (!dynamic_attrs)
		return -ENOMEM;

	attrs = devm_kcalloc(dev, attrcount, sizeof(*attrs), GFP_KERNEL);
	if (!attrs)
		return -ENOMEM;

	dynamic_groups = devm_kcalloc(dev, dynamic_groupcount, sizeof(*dynamic_groups), GFP_KERNEL);
	if (!dynamic_groups)
		return -ENOMEM;
	dev_info(dev, "%i named groups created for device", dynamic_groupcount);

	data->groups = devm_kcalloc(dev, groupcount, sizeof(*data->groups), GFP_KERNEL);
	if (!data->groups)
		return -ENOMEM;

	data->groups[0] = &pcon_global_group;

	dynamic_attridx = 0;
	dynamic_groupidx = 0;

	for (channel = 0; channel < data->num_channels; channel++) {
		data->groups[dynamic_groupidx+1] = &dynamic_groups[dynamic_groupidx];

		dynamic_groups[dynamic_groupidx].name = devm_kasprintf(dev, GFP_KERNEL, "channel%i", channel);
		dynamic_groups[dynamic_groupidx].attrs = &attrs[dynamic_attridx+dynamic_groupidx];

		for (channel_attrnum = 0; channel_attrnum < ARRAY_SIZE(channel_attrs_2); channel_attrnum++) {
			dynamic_attrs[dynamic_attridx] = *channel_attrs_2[channel_attrnum];
			dynamic_attrs[dynamic_attridx].index = channel;

			attrs[dynamic_attridx+dynamic_groupidx] = &dynamic_attrs[dynamic_attridx].dev_attr.attr;
			dynamic_attridx++;
		}
		dynamic_groupidx++;
	}

	data->groups[groupcount-1] = NULL;
	return 0;
}


static int pcon_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct i2c_adapter *adapter = client->adapter;
	struct pcon_data *data;
	struct device *hwmon_dev;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
					I2C_FUNC_SMBUS_WORD_DATA)) {
		dev_err(dev, "%s i2c_check_functionality\n", client->name);
		return -EOPNOTSUPP;
	}

	data = devm_kzalloc(dev, sizeof(struct pcon_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);

	mutex_init(&data->lock);
	data->client = client;
	if (dev->of_node)
		data->type = (enum chips)of_device_get_match_data(dev);
	else
		data->type = i2c_match_id(pcon_id, client)->driver_data;

	dev_info(dev, "%s chip found\n", client->name);

	/* Initialize the AD7418 chip */
	pcon_init_client(client);
	ret = __pcon_data_init(dev, data);
	if (ret < 0) return ret;

	ret = __pcon_create_attribute_groups(dev, data);
	if (ret < 0) return ret;

	hwmon_dev = devm_hwmon_device_register_with_groups(dev,
							client->name,
							data, data->groups);
	return PTR_ERR_OR_ZERO(hwmon_dev);
}

/* 
PCON” uses XC6SLX150
PCONMini: uses XC6SLX45
On X3b we use three PCON Minis.
On X4 PCON 0 and 1 are PCON and PCON 2 is a mini. 
*/

static const struct i2c_device_id pcon_id[] = {
	{ "pcon", pcon },
	{ "pconm", pconm },
	{ }
};
MODULE_DEVICE_TABLE(i2c, pcon_id);

static struct i2c_driver pcon_driver = {
	.driver = {
		.name	= "pcon",
	},
	.probe	= pcon_probe,
	.id_table	= pcon_id,
};

module_i2c_driver(pcon_driver);

MODULE_DESCRIPTION("PCON driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
