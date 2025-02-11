#include <linux/kernel.h>
//  An hwmon driver for delta PSU
//
//  Copyright (C) 2024 Delta Network Technology Corporation
//  Copyright (C) 2024 Nokia Corporation.
//

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/delay.h>

/* PSU parameter */
#define PSU_REG_RW_VOUT_MODE            (0x20)
#define PSU_REG_RO_FAN_STATUS           (0x81)
#define PSU_REG_RO_FAN_SPEED            (0x90)
#define PSU_REG_RO_VIN                  (0x88)
#define PSU_REG_RO_VOUT                 (0x8b)
#define PSU_REG_RO_IIN                  (0x89)
#define PSU_REG_RO_IOUT                 (0x8c)
#define PSU_REG_RO_POUT                 (0x96)
#define PSU_REG_RO_PIN                  (0x97)
#define PSU_REG_RO_TEMP1                (0x8d)
#define PSU_REG_RO_TEMP2                (0x8e)
#define PSU_REG_RO_TEMP3                (0x8f)
#define PSU_REG_RO_MFR_MODEL            (0x9a)
#define PSU_REG_RO_MFR_SERIAL           (0x9e)
#define PSU_MFR_MODELNAME_LENGTH        (11)
#define PSU_MFR_SERIALNUM_LENGTH        (20)
#define PSU_DRIVER_NAME                 "dni_psu"

/* fan in PSU */
#define	PSU_FAN_NUMBER                  (1)
#define PSU_FAN1_FAULT_BIT              (7)

/* thermal in PSU */
#define	PSU_THERMAL_NUMBER              (3)

/* Address scanned */
static const unsigned short normal_i2c[] = { 0x58, 0x59, I2C_CLIENT_END };

/* This is additional data */
struct psu_data
{
    struct device *hwmon_dev;
    struct mutex update_lock;
    char valid;
    unsigned long last_updated; /* In jiffies */
    /* Registers value */
    u8  vout_mode;
    u16 v_in;
    u16 v_out;
    u16 i_in;
    u16 i_out;
    u16 p_in;
    u16 p_out;
    u16 temp_input[PSU_THERMAL_NUMBER];
    u8  fan_fault;
    u16 fan_speed[PSU_FAN_NUMBER];
    u8  mfr_model[PSU_MFR_MODELNAME_LENGTH+1];
    u8  mfr_serial[PSU_MFR_SERIALNUM_LENGTH+1];
};

static int     two_complement_to_int(u16 data, u8 valid_bit, int mask);
static int     calculate_return_value(int value);
static ssize_t for_vin(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_iin(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_iout(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_pin(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_pout(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_temp1(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_temp2(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_temp3(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_fan_speed(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_fan_fault(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_vout_data(struct device *dev, struct device_attribute *dev_attr, char *buf);
static int     psu_read_byte(struct i2c_client *client, u8 reg);
static int     psu_read_word(struct i2c_client *client, u8 reg);
static int     psu_read_block(struct i2c_client *client, u8 command, u8 *data);
static struct  psu_data *psu_update_device(struct device *dev, u8 reg);
static ssize_t for_serial(struct device *dev, struct device_attribute *dev_attr, char *buf);
static ssize_t for_model(struct device *dev, struct device_attribute *dev_attr, char *buf);

enum psu_sysfs_attributes
{
    PSU_V_IN,
    PSU_V_OUT,
    PSU_I_IN,
    PSU_I_OUT,
    PSU_P_IN,
    PSU_P_OUT,
    PSU_TEMP1_INPUT,
    PSU_TEMP2_INPUT,
    PSU_TEMP3_INPUT,
    PSU_FAN1_FAULT,
    PSU_FAN1_DUTY_CYCLE,
    PSU_FAN1_SPEED,
    PSU_MFR_MODEL,
    PSU_MFR_SERIAL,
};

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static int calculate_return_value(int value)
{
    int multiplier = 1000;
    int exponent = 0, mantissa = 0;

    exponent = two_complement_to_int(value >> 11, 5, 0x1f);
    mantissa = two_complement_to_int(value & 0x7ff, 11, 0x7ff);

    return (exponent >= 0) ? (mantissa << exponent) * multiplier : (mantissa * multiplier) / (1 << -exponent);

}

static ssize_t for_vin(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_VIN);

    return sprintf(buf, "%d\n", calculate_return_value(data->v_in));
}

static ssize_t for_iin(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_IIN);


    return sprintf(buf, "%d\n", calculate_return_value(data->i_in));
}

static ssize_t for_iout(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_IOUT);

    return sprintf(buf, "%d\n", calculate_return_value(data->i_out));
}

static ssize_t for_pin(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_PIN);

    return sprintf(buf, "%d\n", calculate_return_value(data->p_in));
}

static ssize_t for_pout(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_POUT);

    return sprintf(buf, "%d\n", calculate_return_value(data->p_out));
}

static ssize_t for_temp1(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_TEMP1);

    return sprintf(buf, "%d\n", calculate_return_value(data->temp_input[0]));
}

static ssize_t for_temp2(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_TEMP2);

    return sprintf(buf, "%d\n", calculate_return_value(data->temp_input[1]));
}

static ssize_t for_temp3(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_TEMP3);

    return sprintf(buf, "%d\n", calculate_return_value(data->temp_input[2]));
}

static ssize_t for_fan_speed(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_FAN_SPEED);

    return sprintf(buf, "%d\n", calculate_return_value(data->fan_speed[0])/1000);
}

static ssize_t for_vout_data(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = psu_update_device(dev, PSU_REG_RW_VOUT_MODE);
    int exponent = 0, mantissa = 0;
    int multiplier = 1000;

    data = psu_update_device(dev, PSU_REG_RO_VOUT);
    mdelay(30);
    exponent = two_complement_to_int(data->vout_mode, 5, 0x1f);
    mantissa = data->v_out;

    return (exponent > 0) ? sprintf(buf, "%d\n", \
        mantissa * (1 << exponent)) : \
        sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static ssize_t for_fan_fault(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct psu_data *data = psu_update_device(dev, PSU_REG_RO_FAN_STATUS);

    u8 shift = (attr->index == PSU_FAN1_FAULT) ? PSU_FAN1_FAULT_BIT : (PSU_FAN1_FAULT_BIT - (attr->index - PSU_FAN1_FAULT));

    return sprintf(buf, "%d\n", data->fan_fault >> shift);
}

static ssize_t for_serial(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = dev_get_drvdata(dev);

    if (!data->valid)
        return 0;

    return sprintf(buf, "%s\n", data->mfr_serial);
}

static ssize_t for_model(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct psu_data *data = dev_get_drvdata(dev);

    if (!data->valid)
        return 0;

    return sprintf(buf, "%s\n", data->mfr_model);
}

static int psu_read_byte(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}

static int psu_read_word(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_word_data(client, reg);
}

static int psu_read_block(struct i2c_client *client, u8 command, u8 *data)
{
	int result = i2c_smbus_read_block_data(client, command, data);
	if (unlikely(result < 0))
		goto abort;

    result = 0;
abort:
    return result;
}

struct reg_data_byte
{
    u8 reg;
    u8 *value;
};

struct reg_data_word
{
    u8 reg;
    u16 *value;
};

static struct psu_data *psu_update_device(struct device *dev, u8 reg)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct psu_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated))
    {
        int i, status;

        struct reg_data_byte regs_byte[] = {
                {PSU_REG_RW_VOUT_MODE,  &data->vout_mode},
                {PSU_REG_RO_FAN_STATUS, &data->fan_fault}
        };

        struct reg_data_word regs_word[] = {
                {PSU_REG_RO_VIN,       &data->v_in},
                {PSU_REG_RO_VOUT,      &data->v_out},
                {PSU_REG_RO_IIN,       &data->i_in},
                {PSU_REG_RO_IOUT,      &data->i_out},
                {PSU_REG_RO_POUT,      &data->p_out},
                {PSU_REG_RO_PIN,       &data->p_in},
                {PSU_REG_RO_TEMP1,     &(data->temp_input[0])},
                {PSU_REG_RO_TEMP2,     &(data->temp_input[1])},
                {PSU_REG_RO_TEMP3,     &(data->temp_input[2])},
                {PSU_REG_RO_FAN_SPEED, &(data->fan_speed[0])},
        };

        //dev_info(&client->dev, "start data update\n");

        /* one milliseconds from now */
        data->last_updated = jiffies + HZ / 1000;

        for (i = 0; i < ARRAY_SIZE(regs_byte); i++)
        {
            if (reg != regs_byte[i].reg)
                continue;

            status = psu_read_byte(client, regs_byte[i].reg);
            if (status < 0)
            {
                dev_info(&client->dev, "reg %d, err %d\n", regs_byte[i].reg, status);
                *(regs_byte[i].value) = 0;
            }
            else
            {
                *(regs_byte[i].value) = status;
            }
            break;
        }

        for (i = 0; i < ARRAY_SIZE(regs_word); i++)
        {
            if (reg != regs_word[i].reg)
                continue;

            status = psu_read_word(client, regs_word[i].reg);
            if (status < 0)
            {
                dev_info(&client->dev, "reg %d, err %d\n", regs_word[i].reg, status);
                *(regs_word[i].value) = 0;
            }
            else
            {
                *(regs_word[i].value) = status;
            }
            break;
        }

        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);
    return data;
}

/* sysfs attributes for hwmon */
static SENSOR_DEVICE_ATTR(psu_v_in, S_IRUGO, for_vin, NULL, PSU_V_IN);
static SENSOR_DEVICE_ATTR(psu_v_out, S_IRUGO, for_vout_data, NULL, PSU_V_OUT);
static SENSOR_DEVICE_ATTR(psu_i_in, S_IRUGO, for_iin, NULL, PSU_I_IN);
static SENSOR_DEVICE_ATTR(psu_i_out, S_IRUGO, for_iout, NULL, PSU_I_OUT);
static SENSOR_DEVICE_ATTR(psu_p_in, S_IRUGO, for_pin, NULL, PSU_P_IN);
static SENSOR_DEVICE_ATTR(psu_p_out, S_IRUGO, for_pout, NULL, PSU_P_OUT);
static SENSOR_DEVICE_ATTR(psu_temp1_input, S_IRUGO, for_temp1, NULL, PSU_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(psu_temp2_input, S_IRUGO, for_temp2, NULL, PSU_TEMP2_INPUT);
static SENSOR_DEVICE_ATTR(psu_temp3_input, S_IRUGO, for_temp3, NULL, PSU_TEMP3_INPUT);
static SENSOR_DEVICE_ATTR(psu_fan1_fault, S_IRUGO, for_fan_fault, NULL, PSU_FAN1_FAULT);
static SENSOR_DEVICE_ATTR(psu_fan1_speed_rpm, S_IRUGO, for_fan_speed, NULL, PSU_FAN1_SPEED);
static SENSOR_DEVICE_ATTR(psu_mfr_model, S_IRUGO, for_model, NULL, PSU_MFR_MODEL);
static SENSOR_DEVICE_ATTR(psu_mfr_serial, S_IRUGO, for_serial, NULL, PSU_MFR_SERIAL);

static struct attribute *psu_attributes[] = {
    &sensor_dev_attr_psu_v_in.dev_attr.attr,
    &sensor_dev_attr_psu_v_out.dev_attr.attr,
    &sensor_dev_attr_psu_i_in.dev_attr.attr,
    &sensor_dev_attr_psu_i_out.dev_attr.attr,
    &sensor_dev_attr_psu_p_in.dev_attr.attr,
    &sensor_dev_attr_psu_p_out.dev_attr.attr,
    &sensor_dev_attr_psu_temp1_input.dev_attr.attr,
    &sensor_dev_attr_psu_temp2_input.dev_attr.attr,
    &sensor_dev_attr_psu_temp3_input.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_fault.dev_attr.attr,
    &sensor_dev_attr_psu_fan1_speed_rpm.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_model.dev_attr.attr,
    &sensor_dev_attr_psu_mfr_serial.dev_attr.attr,
    NULL
};

static const struct attribute_group psu_group = {
    .attrs = psu_attributes,
};

static int psu_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct psu_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_BLOCK_DATA))
    {
        dev_info(&client->dev, "i2c_check_functionality failed!!!\n");
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data)
    {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->valid = 0;
    mutex_init(&data->update_lock);

    dev_info(&client->dev, "chip found\n");

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &psu_group);
    if (status)
    {
        dev_info(&client->dev, "sysfs_create_group failed!!!\n");
        goto exit_sysfs_create_group;
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, PSU_DRIVER_NAME, NULL, NULL);
    if (IS_ERR(data->hwmon_dev))
    {
	    dev_info(&client->dev, "hwmon_device_register failed!!!\n");
        status = PTR_ERR(data->hwmon_dev);
        goto exit_hwmon_device_register;
    }

    dev_info(&client->dev, "%s: psu '%s'\n",
        dev_name(data->hwmon_dev), client->name);

    // PSU mfr_model
    status = psu_read_block(client, PSU_REG_RO_MFR_MODEL, data->mfr_model);
    data->mfr_model[ARRAY_SIZE(data->mfr_model) - 1] = '\0';
    if (status < 0)
    {
        dev_info(&client->dev, "reg %d, err %d\n", PSU_REG_RO_MFR_MODEL, status);
        data->mfr_model[1] = '\0';
    }
    // PSU mfr_serial
    status = psu_read_block(client, PSU_REG_RO_MFR_SERIAL, data->mfr_serial);
    data->mfr_serial[ARRAY_SIZE(data->mfr_serial) - 1] = '\0';
    if (status < 0)
    {
        dev_info(&client->dev, "reg %d, err %d\n", PSU_REG_RO_MFR_SERIAL, status);
        data->mfr_serial[1] = '\0';
    }

    return 0;

exit_hwmon_device_register:
    sysfs_remove_group(&client->dev.kobj, &psu_group);
exit_sysfs_create_group:
    kfree(data);
exit:
    return status;
}

static void psu_remove(struct i2c_client *client)
{
    struct psu_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &psu_group);
    kfree(data);

    return;
}

static const struct i2c_device_id psu_id[] = {
    { PSU_DRIVER_NAME, 0 },
    { PSU_DRIVER_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, psu_id);

/* This is the driver that will be inserted */
static struct i2c_driver psu_driver = {
        .class            = I2C_CLASS_HWMON,
        .driver = {
                    .name = PSU_DRIVER_NAME,
                  },
        .probe            = psu_probe,
        .remove           = psu_remove,
        .id_table         = psu_id,
        .address_list     = normal_i2c,
};

static int __init dni_psu_init(void)
{
    return i2c_add_driver(&psu_driver);
}

static void __exit dni_psu_exit(void)
{
    i2c_del_driver(&psu_driver);
}

MODULE_AUTHOR("DNI SW5");
MODULE_DESCRIPTION("DNI PSU Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.2");

module_init(dni_psu_init);
module_exit(dni_psu_exit);

