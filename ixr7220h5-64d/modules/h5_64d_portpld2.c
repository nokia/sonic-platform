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

#define DRIVER_NAME "h5_64d_portpld2"

// REGISTERS ADDRESS MAP
#define SCRATCH_REG             0x00
#define CODE_REV_REG            0x01
#define BOARD_REV_REG           0x02
#define BOARD_CFG_REG           0x03
#define SYS_EEPROM_REG          0x05
#define BOARD_CTRL_REG          0x07
#define LED_TEST_REG            0x08
#define RST_PLD_REG             0x10
#define INT_CLR_REG             0x20
#define INT_MSK_REG             0x21
#define INT_REG                 0x22
#define PLD_INT_REG0            0x23
#define PLD_INT_REG1            0x24
#define PLD_INT_REG2            0x25
#define PLD_INT_REG3            0x26
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
#define PERIF_STAT_REG0         0x50
#define PERIF_STAT_REG1         0x51
#define PERIF_STAT_REG2         0x54
#define PERIF_STAT_REG3         0x55
#define PWR_STATUS_REG0         0x68
#define PWR_STATUS_REG1         0x69
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

// common index of each qsfp modules
#define QSFP33_INDEX            0x0
#define QSFP34_INDEX            0x1
#define QSFP35_INDEX            0x2
#define QSFP36_INDEX            0x3
#define QSFP37_INDEX            0x4
#define QSFP38_INDEX            0x5
#define QSFP39_INDEX            0x6
#define QSFP40_INDEX            0x7
#define QSFP41_INDEX            0x0
#define QSFP42_INDEX            0x1
#define QSFP43_INDEX            0x2
#define QSFP44_INDEX            0x3
#define QSFP45_INDEX            0x4
#define QSFP46_INDEX            0x5
#define QSFP47_INDEX            0x6
#define QSFP48_INDEX            0x7
#define QSFP49_INDEX            0x0
#define QSFP50_INDEX            0x1
#define QSFP51_INDEX            0x2
#define QSFP52_INDEX            0x3
#define QSFP53_INDEX            0x4
#define QSFP54_INDEX            0x5
#define QSFP55_INDEX            0x6
#define QSFP56_INDEX            0x7
#define QSFP57_INDEX            0x0
#define QSFP58_INDEX            0x1
#define QSFP59_INDEX            0x2
#define QSFP60_INDEX            0x3
#define QSFP61_INDEX            0x4
#define QSFP62_INDEX            0x5
#define QSFP63_INDEX            0x6
#define QSFP64_INDEX            0x7

static const unsigned short cpld_address_list[] = {0x45, I2C_CLIENT_END};

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

static SENSOR_DEVICE_ATTR(qsfp33_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP33_INDEX);
static SENSOR_DEVICE_ATTR(qsfp34_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP34_INDEX);
static SENSOR_DEVICE_ATTR(qsfp35_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP35_INDEX);
static SENSOR_DEVICE_ATTR(qsfp36_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP36_INDEX);
static SENSOR_DEVICE_ATTR(qsfp37_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP37_INDEX);
static SENSOR_DEVICE_ATTR(qsfp38_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP38_INDEX);
static SENSOR_DEVICE_ATTR(qsfp39_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP39_INDEX);
static SENSOR_DEVICE_ATTR(qsfp40_prs, S_IRUGO, show_qsfp_prs0, NULL, QSFP40_INDEX);
static SENSOR_DEVICE_ATTR(qsfp41_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP41_INDEX);
static SENSOR_DEVICE_ATTR(qsfp42_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP42_INDEX);
static SENSOR_DEVICE_ATTR(qsfp43_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP43_INDEX);
static SENSOR_DEVICE_ATTR(qsfp44_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP44_INDEX);
static SENSOR_DEVICE_ATTR(qsfp45_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP45_INDEX);
static SENSOR_DEVICE_ATTR(qsfp46_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP46_INDEX);
static SENSOR_DEVICE_ATTR(qsfp47_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP47_INDEX);
static SENSOR_DEVICE_ATTR(qsfp48_prs, S_IRUGO, show_qsfp_prs1, NULL, QSFP48_INDEX);
static SENSOR_DEVICE_ATTR(qsfp49_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP49_INDEX);
static SENSOR_DEVICE_ATTR(qsfp50_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP50_INDEX);
static SENSOR_DEVICE_ATTR(qsfp51_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP51_INDEX);
static SENSOR_DEVICE_ATTR(qsfp52_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP52_INDEX);
static SENSOR_DEVICE_ATTR(qsfp53_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP53_INDEX);
static SENSOR_DEVICE_ATTR(qsfp54_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP54_INDEX);
static SENSOR_DEVICE_ATTR(qsfp55_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP55_INDEX);
static SENSOR_DEVICE_ATTR(qsfp56_prs, S_IRUGO, show_qsfp_prs2, NULL, QSFP56_INDEX);
static SENSOR_DEVICE_ATTR(qsfp57_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP57_INDEX);
static SENSOR_DEVICE_ATTR(qsfp58_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP58_INDEX);
static SENSOR_DEVICE_ATTR(qsfp59_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP59_INDEX);
static SENSOR_DEVICE_ATTR(qsfp60_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP60_INDEX);
static SENSOR_DEVICE_ATTR(qsfp61_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP61_INDEX);
static SENSOR_DEVICE_ATTR(qsfp62_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP62_INDEX);
static SENSOR_DEVICE_ATTR(qsfp63_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP63_INDEX);
static SENSOR_DEVICE_ATTR(qsfp64_prs, S_IRUGO, show_qsfp_prs3, NULL, QSFP64_INDEX);

static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);

static struct attribute *h5_64d_portpld2_attributes[] = {
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,
    &sensor_dev_attr_led_test_amb.dev_attr.attr,
    &sensor_dev_attr_led_test_grn.dev_attr.attr,
    &sensor_dev_attr_led_test_blink.dev_attr.attr,
    &sensor_dev_attr_led_test_src_sel.dev_attr.attr,
    &sensor_dev_attr_rst_pld_soft.dev_attr.attr,

    &sensor_dev_attr_qsfp33_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp34_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp35_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp36_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp37_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp38_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp39_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp40_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp41_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp42_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp43_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp44_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp45_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp46_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp47_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp48_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp49_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp50_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp51_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp52_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp53_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp54_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp55_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp56_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp57_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp58_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp59_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp60_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp61_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp62_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp63_prs.dev_attr.attr,
    &sensor_dev_attr_qsfp64_prs.dev_attr.attr,

    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,
    NULL
};

static const struct attribute_group h5_64d_portpld2_group = {
    .attrs = h5_64d_portpld2_attributes,
};

static int h5_64d_portpld2_probe(struct i2c_client *client,
        const struct i2c_device_id *dev_id)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H5-64D PortPLD2 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &h5_64d_portpld2_group);
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

static void h5_64d_portpld2_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &h5_64d_portpld2_group);
    kfree(data);
}

static const struct of_device_id h5_64d_portpld2_of_ids[] = {
    {
        .compatible = "nokia,h5_64d_portpld2",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, h5_64d_portpld2_of_ids);

static const struct i2c_device_id h5_64d_portpld2_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, h5_64d_portpld2_ids);

static struct i2c_driver h5_64d_portpld2_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(h5_64d_portpld2_of_ids),
    },
    .probe        = h5_64d_portpld2_probe,
    .remove       = h5_64d_portpld2_remove,
    .id_table     = h5_64d_portpld2_ids,
    .address_list = cpld_address_list,
};

static int __init h5_64d_portpld2_init(void)
{
    return i2c_add_driver(&h5_64d_portpld2_driver);
}

static void __exit h5_64d_portpld2_exit(void)
{
    i2c_del_driver(&h5_64d_portpld2_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H5-64D CPLD driver");
MODULE_LICENSE("GPL");

module_init(h5_64d_portpld2_init);
module_exit(h5_64d_portpld2_exit);
