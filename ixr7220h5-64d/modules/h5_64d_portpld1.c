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

#define DRIVER_NAME "h5_64d_portpld1"

// REGISTERS ADDRESS MAP
#define SCRATCH_REG             0x00
#define CODE_REV_REG            0x01
#define BOARD_REV_REG           0x02
#define BOARD_CFG_REG           0x03
#define LED_TEST_REG            0x08
#define RST_PLD_REG             0x10
#define RST_MSK_REG             0x11
#define RST_CTRL_REG            0x12
#define INT_CLR_REG             0x20
#define INT_MSK_REG             0x21
#define INT_REG                 0x22
#define PLD_INT_REG             0x23
#define SFP_INT_REG             0x24
#define QSFP_PRS_INT_REG0       0x28
#define QSFP_PRS_INT_REG1       0x29
#define QSFP_PRS_INT_REG2       0x2A
#define QSFP_PRS_INT_REG3       0x2B
#define QSFP_INT_EVT_REG0       0x2C
#define QSFP_INT_EVT_REG1       0x2D
#define QSFP_INT_EVT_REG2       0x2E
#define QSFP_INT_EVT_REG3       0x2F
#define QSFP_RST_REG0           0x30
#define QSFP_RST_REG1           0x31
#define QSFP_RST_REG2           0x32
#define QSFP_RST_REG3           0x33
#define QSFP_LPMODE_REG0        0x34
#define QSFP_LPMODE_REG1        0x35
#define QSFP_LPMODE_REG2        0x36
#define QSFP_LPMODE_REG3        0x37
#define QSFP_MODSEL_REG0        0x38
#define QSFP_MODSEL_REG1        0x39
#define QSFP_MODSEL_REG2        0x3A
#define QSFP_MODSEL_REG3        0x3B
#define QSFP_MODPRS_REG0        0x3C
#define QSFP_MODPRS_REG1        0x3D
#define QSFP_MODPRS_REG2        0x3E
#define QSFP_MODPRS_REG3        0x3F
#define QSFP_INT_STAT_REG0      0x40
#define QSFP_INT_STAT_REG1      0x41
#define QSFP_INT_STAT_REG2      0x42
#define QSFP_INT_STAT_REG3      0x43
#define SFP_CTRL_REG            0x44
#define SFP_STAT_REG            0x45
#define CODE_DAY_REG            0xF0
#define CODE_MONTH_REG          0xF1
#define CODE_YEAR_REG           0xF2
#define TEST_CODE_REV_REG       0xF3

// REG BIT FIELD POSITION or MASK
#define BOARD_REV_REG_VER_MSK   0x7

#define LED_TEST_REG_AMB        0x0
#define LED_TEST_REG_GRN        0x1
#define LED_TEST_REG_BLINK      0x3
#define LED_TEST_REG_SRC_SEL    0x7

#define RST_PLD_REG_SOFT_RST    0x0

#define SFP_REG0_TX_FAULT       0x4
#define SFP_REG0_RX_LOS         0x5
#define SFP_REG0_PRS            0x6

#define SFP_REG1_TX_EN          0x7

// common index of each qsfp modules
#define QSFP01_INDEX            0x0
#define QSFP02_INDEX            0x1
#define QSFP03_INDEX            0x2
#define QSFP04_INDEX            0x3
#define QSFP05_INDEX            0x4
#define QSFP06_INDEX            0x5
#define QSFP07_INDEX            0x6
#define QSFP08_INDEX            0x7
#define QSFP09_INDEX            0x0
#define QSFP10_INDEX            0x1
#define QSFP11_INDEX            0x2
#define QSFP12_INDEX            0x3
#define QSFP13_INDEX            0x4
#define QSFP14_INDEX            0x5
#define QSFP15_INDEX            0x6
#define QSFP16_INDEX            0x7
#define QSFP17_INDEX            0x0
#define QSFP18_INDEX            0x1
#define QSFP19_INDEX            0x2
#define QSFP20_INDEX            0x3
#define QSFP21_INDEX            0x4
#define QSFP22_INDEX            0x5
#define QSFP23_INDEX            0x6
#define QSFP24_INDEX            0x7
#define QSFP25_INDEX            0x0
#define QSFP26_INDEX            0x1
#define QSFP27_INDEX            0x2
#define QSFP28_INDEX            0x3
#define QSFP29_INDEX            0x4
#define QSFP30_INDEX            0x5
#define QSFP31_INDEX            0x6
#define QSFP32_INDEX            0x7

static const unsigned short cpld_address_list[] = {0x41, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int code_ver;
    int board_ver;
    int code_day;
    int code_month;
    int code_year;
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
    return sprintf(buf, "0x%02x\n", data->code_ver);
}

static ssize_t show_board_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "0x%x\n", data->board_ver);
}

static ssize_t show_led_test(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, LED_TEST_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_led_test(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, LED_TEST_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, LED_TEST_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_rst(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, RST_PLD_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_rst(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, RST_PLD_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, RST_PLD_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_qsfp_prs0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_prs1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_prs2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_qsfp_prs3(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = cpld_i2c_read(data, QSFP_MODPRS_REG3);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_code_day(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", data->code_day);
}

static ssize_t show_code_month(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", data->code_month);
}

static ssize_t show_code_year(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%d\n", data->code_year);
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0); 
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(board_ver, S_IRUGO, show_board_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(led_test_amb, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_AMB);
static SENSOR_DEVICE_ATTR(led_test_grn, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_GRN);
static SENSOR_DEVICE_ATTR(led_test_blink, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_BLINK);
static SENSOR_DEVICE_ATTR(led_test_src_sel, S_IRUGO | S_IWUSR, show_led_test, set_led_test, LED_TEST_REG_SRC_SEL);
static SENSOR_DEVICE_ATTR(rst_pld_soft, S_IRUGO | S_IWUSR, show_rst, set_rst, RST_PLD_REG_SOFT_RST);

static SENSOR_DEVICE_ATTR(qsfp1_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP01_INDEX);
static SENSOR_DEVICE_ATTR(qsfp2_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP02_INDEX);
static SENSOR_DEVICE_ATTR(qsfp3_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP03_INDEX);
static SENSOR_DEVICE_ATTR(qsfp4_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP04_INDEX);
static SENSOR_DEVICE_ATTR(qsfp5_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP05_INDEX);
static SENSOR_DEVICE_ATTR(qsfp6_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP06_INDEX);
static SENSOR_DEVICE_ATTR(qsfp7_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP07_INDEX);
static SENSOR_DEVICE_ATTR(qsfp8_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP08_INDEX);
static SENSOR_DEVICE_ATTR(qsfp9_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP09_INDEX);
static SENSOR_DEVICE_ATTR(qsfp10_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP10_INDEX);
static SENSOR_DEVICE_ATTR(qsfp11_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP11_INDEX);
static SENSOR_DEVICE_ATTR(qsfp12_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP12_INDEX);
static SENSOR_DEVICE_ATTR(qsfp13_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP13_INDEX);
static SENSOR_DEVICE_ATTR(qsfp14_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP14_INDEX);
static SENSOR_DEVICE_ATTR(qsfp15_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP15_INDEX);
static SENSOR_DEVICE_ATTR(qsfp16_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP16_INDEX);
static SENSOR_DEVICE_ATTR(qsfp17_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP17_INDEX);
static SENSOR_DEVICE_ATTR(qsfp18_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP18_INDEX);
static SENSOR_DEVICE_ATTR(qsfp19_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP19_INDEX);
static SENSOR_DEVICE_ATTR(qsfp20_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP20_INDEX);
static SENSOR_DEVICE_ATTR(qsfp21_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP21_INDEX);
static SENSOR_DEVICE_ATTR(qsfp22_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP22_INDEX);
static SENSOR_DEVICE_ATTR(qsfp23_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP23_INDEX);
static SENSOR_DEVICE_ATTR(qsfp24_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP24_INDEX);
static SENSOR_DEVICE_ATTR(qsfp25_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP25_INDEX);
static SENSOR_DEVICE_ATTR(qsfp26_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP26_INDEX);
static SENSOR_DEVICE_ATTR(qsfp27_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP27_INDEX);
static SENSOR_DEVICE_ATTR(qsfp28_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP28_INDEX);
static SENSOR_DEVICE_ATTR(qsfp29_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP29_INDEX);
static SENSOR_DEVICE_ATTR(qsfp30_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP30_INDEX);
static SENSOR_DEVICE_ATTR(qsfp31_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP31_INDEX);
static SENSOR_DEVICE_ATTR(qsfp32_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP32_INDEX);

static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);

static struct attribute *h5_64d_portpld1_attributes[] = {
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,
    &sensor_dev_attr_led_test_amb.dev_attr.attr,
    &sensor_dev_attr_led_test_grn.dev_attr.attr,
    &sensor_dev_attr_led_test_blink.dev_attr.attr,
    &sensor_dev_attr_led_test_src_sel.dev_attr.attr,    
    &sensor_dev_attr_rst_pld_soft.dev_attr.attr,

    &sensor_dev_attr_qsfp1_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp2_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp3_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp4_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp5_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp6_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp7_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp8_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp9_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp10_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp11_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp12_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp13_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp14_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp15_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp16_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp17_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp18_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp19_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp20_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp21_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp22_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp23_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp24_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp25_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp26_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp27_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp28_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp29_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp30_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp31_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp32_prs.dev_attr.attr,

    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,  
    NULL
};

static const struct attribute_group h5_64d_portpld1_group = {
    .attrs = h5_64d_portpld1_attributes,
};

static int h5_64d_portpld1_probe(struct i2c_client *client,
        const struct i2c_device_id *dev_id)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H5-64D PortPLD1 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &h5_64d_portpld1_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->code_ver = cpld_i2c_read(data, CODE_REV_REG);
    data->board_ver = cpld_i2c_read(data, BOARD_REV_REG) & BOARD_REV_REG_VER_MSK;
    data->code_day = cpld_i2c_read(data, CODE_DAY_REG);
    data->code_month = cpld_i2c_read(data, CODE_MONTH_REG);
    data->code_year = cpld_i2c_read(data, CODE_YEAR_REG);
    cpld_i2c_write(data, QSFP_RST_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG2, 0xFF);
    cpld_i2c_write(data, QSFP_RST_REG3, 0xFF);
    cpld_i2c_write(data, QSFP_LPMODE_REG0, 0x0);
    cpld_i2c_write(data, QSFP_LPMODE_REG1, 0x0);
    cpld_i2c_write(data, QSFP_LPMODE_REG2, 0x0);
    cpld_i2c_write(data, QSFP_LPMODE_REG3, 0x0);
    cpld_i2c_write(data, QSFP_MODSEL_REG0, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG1, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG2, 0xFF);
    cpld_i2c_write(data, QSFP_MODSEL_REG3, 0xFF);
    
    return 0;

exit:
    return status;
}

static void h5_64d_portpld1_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &h5_64d_portpld1_group);
    kfree(data);
}

static const struct of_device_id h5_64d_portpld1_of_ids[] = {
    {
        .compatible = "nokia,h5_64d_portpld1",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, h5_64d_portpld1_of_ids);

static const struct i2c_device_id h5_64d_portpld1_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, h5_64d_portpld1_ids);

static struct i2c_driver h5_64d_portpld1_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(h5_64d_portpld1_of_ids),
    },
    .probe        = h5_64d_portpld1_probe,
    .remove       = h5_64d_portpld1_remove,
    .id_table     = h5_64d_portpld1_ids,
    .address_list = cpld_address_list,
};

static int __init h5_64d_portpld1_init(void)
{
    return i2c_add_driver(&h5_64d_portpld1_driver);
}

static void __exit h5_64d_portpld1_exit(void)
{
    i2c_del_driver(&h5_64d_portpld1_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H5-64D CPLD driver");
MODULE_LICENSE("GPL");

module_init(h5_64d_portpld1_init);
module_exit(h5_64d_portpld1_exit);
