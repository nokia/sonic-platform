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

#define DRIVER_NAME "nokia_7220h3_swpld1"

// SWPLD1 REG ADDRESS-MAP
#define SWPLD1_SWBD_ID_REG           0x01
#define SWPLD1_SWBD_VER_REG          0x02
#define SWPLD1_CPLD_REV_REG          0x03
#define SWPLD1_TEST_REG              0x0F
#define SWPLD1_PSU1_REG              0x11
#define SWPLD1_PSU2_REG              0x12
#define SWPLD1_PWR1_REG              0x21
#define SWPLD1_PWR2_REG              0x22
#define SWPLD1_MAC_ROV_REG           0x25
#define SWPLD1_PSU_FAN_INT_REG       0x26
#define SWPLD1_SWPLD_INT_REG         0x27
#define SWPLD1_MB_CPU_INT_REG        0x28
#define SWPLD1_SMB_ALERT_REG         0x29
#define SWPLD1_VR_ALERT_REG          0x2A
#define SWPLD1_PCIE_ALERT_REG        0x2B
#define SWPLD1_FP_LED1_REG           0x41
#define SWPLD1_FP_LED2_REG           0x42
#define SWPLD1_FAN_LED1_REG          0x46
#define SWPLD1_FAN_LED2_REG          0x47
#define SWPLD1_MISC_SEL_REG          0x51

// REG BIT FIELD POSITION or MASK
#define SWPLD1_CPLD_REV_REG_TYPE         0x07
#define SWPLD1_CPLD_REV_REG_MSK          0x3F

#define SWPLD1_PSU1_REG_PSU2_INT         0x01
#define SWPLD1_PSU1_REG_PSU2_OK          0x02
#define SWPLD1_PSU1_REG_PSU2_PRES        0x03
#define SWPLD1_PSU1_REG_PSU1_INT         0x04
#define SWPLD1_PSU1_REG_PSU1_OK          0x05
#define SWPLD1_PSU1_REG_PSU1_PRES        0x06

#define SWPLD1_PSU2_REG_PSU2_WP          0x02
#define SWPLD1_PSU2_REG_PSU2_PSON        0x03
#define SWPLD1_PSU2_REG_PSU1_WP          0x06
#define SWPLD1_PSU2_REG_PSU1_PSON        0x07

#define SWPLD1_PWR1_REG_MAC_1V2          0x00
#define SWPLD1_PWR1_REG_BMC_1V15         0x01
#define SWPLD1_PWR1_REG_BMC_1V2          0x02
#define SWPLD1_PWR1_REG_MAC_1V8          0x03
#define SWPLD1_PWR1_REG_2V5              0x04
#define SWPLD1_PWR1_REG_3V3_CT           0x05
#define SWPLD1_PWR1_REG_3V3              0x06
#define SWPLD1_PWR1_REG_5V               0x07

#define SWPLD1_PWR2_REG_MAC_PLL_0V8      0x04
#define SWPLD1_PWR2_REG_MAC_0V8          0x05
#define SWPLD1_PWR2_REG_MAC_VCORE        0x06

#define SWPLD1_MAC_ROV_REG_AVS0          0x00
#define SWPLD1_MAC_ROV_REG_AVS1          0x01
#define SWPLD1_MAC_ROV_REG_AVS2          0x02
#define SWPLD1_MAC_ROV_REG_AVS3          0x03
#define SWPLD1_MAC_ROV_REG_AVS4          0x04
#define SWPLD1_MAC_ROV_REG_AVS5          0x05
#define SWPLD1_MAC_ROV_REG_AVS6          0x06
#define SWPLD1_MAC_ROV_REG_AVS7          0x07

#define SWPLD1_PSU_FAN_INT_REG_FAN_ALERT_N 0x00
#define SWPLD1_PSU_FAN_INT_REG_PSU_INT_N   0x01

#define SWPLD1_SWPLD_INT_REG_SWPLD2_INT_N 0x00
#define SWPLD1_SWPLD_INT_REG_SWPLD3_INT_N 0x01

#define SWPLD1_MB_CPU_INT_REG_MISC_INT_N    0x00
#define SWPLD1_MB_CPU_INT_REG_OP_MOD_INT_N  0x01
#define SWPLD1_MB_CPU_INT_REG_PSU_FAN_INT_N 0x02

#define SWPLD1_SMB_ALERT_REG_SYNCE_INT_N     0x03
#define SWPLD1_SMB_ALERT_REG_I210_SMB_ALRT_N 0x04
#define SWPLD1_SMB_ALERT_REG_3V3_VR_ALRT_N   0x05
#define SWPLD1_SMB_ALERT_REG_VCORE_ALRT_N    0x06
#define SWPLD1_SMB_ALERT_REG_0V8_VR_ALRT_N   0x07

#define SWPLD1_VR_ALERT_REG_CPU_THRML_INT_N 0x00
#define SWPLD1_VR_ALERT_REG_3V3_VR_HOT      0x01
#define SWPLD1_VR_ALERT_REG_VCORE_HOT       0x02
#define SWPLD1_VR_ALERT_REG_0V8_VR_HOT      0x03
#define SWPLD1_VR_ALERT_REG_3V3_VR_FAULT    0x05
#define SWPLD1_VR_ALERT_REG_VCORE_FAULT     0x06
#define SWPLD1_VR_ALERT_REG_0V8_VR_FAULT    0x07

#define SWPLD1_PCIE_ALERT_REG_PCIE_ALRT_N      0x02
#define SWPLD1_PCIE_ALERT_REG_SYNCE_PRS_N      0x03
#define SWPLD1_PCIE_ALERT_REG_MAC_PCIE_WAKE_N  0x04
#define SWPLD1_PCIE_ALERT_REG_I210_PCIE_WAKE_N 0x05

#define SWPLD1_FP_LED1_REG_PSU2_LED         0x00
#define SWPLD1_FP_LED1_REG_PSU1_LED         0x02

#define SWPLD1_FP_LED2_REG_FAN_LED          0x00
#define SWPLD1_FP_LED2_REG_SYS_LED          0x02

#define SWPLD1_FAN_LED1_REG_FAN4_LED        0x00
#define SWPLD1_FAN_LED1_REG_FAN3_LED        0x02
#define SWPLD1_FAN_LED1_REG_FAN2_LED        0x04
#define SWPLD1_FAN_LED1_REG_FAN1_LED        0x06

#define SWPLD1_FAN_LED2_REG_FAN6_LED        0x04
#define SWPLD1_FAN_LED2_REG_FAN5_LED        0x06

#define SWPLD1_MISC_SEL_REG_CONSOLE_SEL     0x01

static const unsigned short cpld_address_list[] = {0x32, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int swbd_id;
    int swbd_version;
    int cpld_type;
    int cpld_version;
};

static int nokia_7220_h3_swpld_read(struct cpld_data *data, u8 reg)
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

static void nokia_7220_h3_swpld_write(struct cpld_data *data, u8 reg, u8 value)
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

static ssize_t show_swbd_id(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    if (data->swbd_id == 0x06) 
        return sprintf(buf, "0x%02x: IPDC032A\n", data->swbd_id);
    else
        return sprintf(buf, "0x%02x: Unkown\n", data->swbd_id); 
}

static ssize_t show_swbd_version(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *swbd_version = NULL;
    
    switch ((data->swbd_version & 0xF0) >> 4) {
    case 0xA:
        swbd_version = "EVT";
        break;
    case 0xB:
        swbd_version = "DVT";
        break;
    case 0x0:
        swbd_version = "MP";
        break;
    default:
        swbd_version = "Unknown";
        break;
    }

    return sprintf(buf, "%s: 0x%02x\n", swbd_version, data->swbd_version); 
}

static ssize_t show_cpld_type(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *cpld_type = NULL;
    
    switch (data->cpld_type) {
    case 0:
        cpld_type = "Official type";
        break;
    case 1:
        cpld_type = "Test type";
        break;
    default:
        cpld_type = "Unknown";
        break;
    }

    return sprintf(buf, "0x%02x %s\n", data->cpld_type, cpld_type);
}

static ssize_t show_cpld_version(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "0x%02x\n", data->cpld_version);
}

static ssize_t show_scratch(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_TEST_REG);

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

    nokia_7220_h3_swpld_write(data, SWPLD1_TEST_REG, usr_val);

    return count;
}

static ssize_t show_psu1_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_PSU1_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_psu2_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_PSU2_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_psu2_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_PSU2_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD1_PSU2_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_pwr1_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_PWR1_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_pwr2_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_PWR2_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_mac_rov_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_MAC_ROV_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_psu_fan_int_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_PSU_FAN_INT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_swpld_int_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_SWPLD_INT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_mb_cpu_int_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_MB_CPU_INT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_smb_alert_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_SMB_ALERT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_vr_alert_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_VR_ALERT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_pcie_alert_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_PCIE_ALERT_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_fp_led1_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
      
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FP_LED1_REG);
    usr_val = (reg_val>>sda->index) & 0x03;

    return sprintf(buf, "%d\n", usr_val);
}

static ssize_t set_fp_led1_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    if (usr_val > 3) {
        return -EINVAL;
    }

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FP_LED1_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD1_FP_LED1_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_fp_led2_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 bits;

    if (sda->index == 0) bits = 0x3;
    else if (sda->index == 2) bits = 0x7;

    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FP_LED2_REG);
    usr_val = (reg_val>>sda->index) & bits;

    return sprintf(buf, "%d\n", usr_val);
}

static ssize_t set_fp_led2_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
    u8 mask;
    u8 bits;

    if (sda->index == 0) bits = 0x3;
    else if (sda->index == 2) bits = 0x7;

    int ret = kstrtou8(buf, 10, &usr_val);
    if (ret != 0) {
        return ret; 
    }
    if (((sda->index == 0) && (usr_val > 3)) || 
        ((sda->index == 2) && (usr_val > 7))) {
        return -EINVAL;
    }    

    mask = (~(bits << sda->index)) & 0xFF;
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FP_LED2_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD1_FP_LED2_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_fan_led1_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
      
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FAN_LED1_REG);
    usr_val = (reg_val>>sda->index) & 0x03;

    return sprintf(buf, "%d\n", usr_val);
}

static ssize_t set_fan_led1_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    if (usr_val > 2) {
        return -EINVAL;
    }

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FAN_LED1_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD1_FAN_LED1_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_fan_led2_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 reg_val = 0;
    u8 usr_val = 0;
      
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FAN_LED2_REG);
    usr_val = (reg_val>>sda->index) & 0x03;

    return sprintf(buf, "%d\n", usr_val);
}

static ssize_t set_fan_led2_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    if (usr_val > 2) {
        return -EINVAL;
    }

    mask = (~(0x3 << sda->index)) & 0xFF;
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_FAN_LED2_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD1_FAN_LED2_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_misc_sel_reg(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
      
    val = nokia_7220_h3_swpld_read(data, SWPLD1_MISC_SEL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_misc_sel_reg(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = nokia_7220_h3_swpld_read(data, SWPLD1_MISC_SEL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    nokia_7220_h3_swpld_write(data, SWPLD1_MISC_SEL_REG, (reg_val | usr_val));

    return count;
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(swbd_id,      S_IRUGO, show_swbd_id, NULL, 0);
static SENSOR_DEVICE_ATTR(swbd_version, S_IRUGO, show_swbd_version, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_type,    S_IRUGO, show_cpld_type, NULL, SWPLD1_CPLD_REV_REG_TYPE);
static SENSOR_DEVICE_ATTR(cpld_version, S_IRUGO, show_cpld_version, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch,      S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);

static SENSOR_DEVICE_ATTR(psu2_alert, S_IRUGO, show_psu1_reg, NULL, SWPLD1_PSU1_REG_PSU2_INT);
static SENSOR_DEVICE_ATTR(psu2_ok,    S_IRUGO, show_psu1_reg, NULL, SWPLD1_PSU1_REG_PSU2_OK);
static SENSOR_DEVICE_ATTR(psu2_pres,  S_IRUGO, show_psu1_reg, NULL, SWPLD1_PSU1_REG_PSU2_PRES);
static SENSOR_DEVICE_ATTR(psu1_alert, S_IRUGO, show_psu1_reg, NULL, SWPLD1_PSU1_REG_PSU1_INT);
static SENSOR_DEVICE_ATTR(psu1_ok,    S_IRUGO, show_psu1_reg, NULL, SWPLD1_PSU1_REG_PSU1_OK);
static SENSOR_DEVICE_ATTR(psu1_pres,  S_IRUGO, show_psu1_reg, NULL, SWPLD1_PSU1_REG_PSU1_PRES);

static SENSOR_DEVICE_ATTR(psu2_eeprom_wp, S_IRUGO | S_IWUSR, show_psu2_reg, set_psu2_reg, SWPLD1_PSU2_REG_PSU2_WP);
static SENSOR_DEVICE_ATTR(psu2_pson,      S_IRUGO | S_IWUSR, show_psu2_reg, set_psu2_reg, SWPLD1_PSU2_REG_PSU2_PSON);
static SENSOR_DEVICE_ATTR(psu1_eeprom_wp, S_IRUGO | S_IWUSR, show_psu2_reg, set_psu2_reg, SWPLD1_PSU2_REG_PSU1_WP);
static SENSOR_DEVICE_ATTR(psu1_pson,      S_IRUGO | S_IWUSR, show_psu2_reg, set_psu2_reg, SWPLD1_PSU2_REG_PSU1_PSON);

static SENSOR_DEVICE_ATTR(vcc_mac_1v2,   S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_MAC_1V2);
static SENSOR_DEVICE_ATTR(vcc_bmc_1v15,  S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_BMC_1V15);
static SENSOR_DEVICE_ATTR(vcc_bmc_1v2,   S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_BMC_1V2);
static SENSOR_DEVICE_ATTR(vcc_mac_1v8,   S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_MAC_1V8);
static SENSOR_DEVICE_ATTR(vcc_2V5,       S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_2V5);
static SENSOR_DEVICE_ATTR(vcc_3v3_ct,    S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_3V3_CT);
static SENSOR_DEVICE_ATTR(vcc_3v3,       S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_3V3);
static SENSOR_DEVICE_ATTR(vcc_5v,        S_IRUGO, show_pwr1_reg, NULL, SWPLD1_PWR1_REG_5V);

static SENSOR_DEVICE_ATTR(vcc_mac_pll_0v8,  S_IRUGO, show_pwr2_reg, NULL, SWPLD1_PWR2_REG_MAC_PLL_0V8);
static SENSOR_DEVICE_ATTR(vcc_mac_0v8,      S_IRUGO, show_pwr2_reg, NULL, SWPLD1_PWR2_REG_MAC_0V8);
static SENSOR_DEVICE_ATTR(vcc_mac_avs_0v91, S_IRUGO, show_pwr2_reg, NULL, SWPLD1_PWR2_REG_MAC_VCORE);

static SENSOR_DEVICE_ATTR(bcm_avs0, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS0);
static SENSOR_DEVICE_ATTR(bcm_avs1, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS1);
static SENSOR_DEVICE_ATTR(bcm_avs2, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS2);
static SENSOR_DEVICE_ATTR(bcm_avs3, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS3);
static SENSOR_DEVICE_ATTR(bcm_avs4, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS4);
static SENSOR_DEVICE_ATTR(bcm_avs5, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS5);
static SENSOR_DEVICE_ATTR(bcm_avs6, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS6);
static SENSOR_DEVICE_ATTR(bcm_avs7, S_IRUGO, show_mac_rov_reg, NULL, SWPLD1_MAC_ROV_REG_AVS7);

static SENSOR_DEVICE_ATTR(fan_alert_n, S_IRUGO, show_psu_fan_int_reg, NULL, SWPLD1_PSU_FAN_INT_REG_FAN_ALERT_N);
static SENSOR_DEVICE_ATTR(psu_int_n,   S_IRUGO, show_psu_fan_int_reg, NULL, SWPLD1_PSU_FAN_INT_REG_PSU_INT_N);

static SENSOR_DEVICE_ATTR(swpld2_int_n, S_IRUGO, show_swpld_int_reg, NULL, SWPLD1_SWPLD_INT_REG_SWPLD2_INT_N);
static SENSOR_DEVICE_ATTR(swpld3_int_n, S_IRUGO, show_swpld_int_reg, NULL, SWPLD1_SWPLD_INT_REG_SWPLD3_INT_N);

static SENSOR_DEVICE_ATTR(cpld_misc_int_n,    S_IRUGO, show_mb_cpu_int_reg, NULL, SWPLD1_MB_CPU_INT_REG_MISC_INT_N);
static SENSOR_DEVICE_ATTR(cpld_op_mod_int_n,  S_IRUGO, show_mb_cpu_int_reg, NULL, SWPLD1_MB_CPU_INT_REG_OP_MOD_INT_N);
static SENSOR_DEVICE_ATTR(cpld_psu_fan_int_n, S_IRUGO, show_mb_cpu_int_reg, NULL, SWPLD1_MB_CPU_INT_REG_PSU_FAN_INT_N);

static SENSOR_DEVICE_ATTR(synce_int_n,     S_IRUGO, show_smb_alert_reg, NULL, SWPLD1_SMB_ALERT_REG_SYNCE_INT_N);
static SENSOR_DEVICE_ATTR(i210_smb_alrt_n, S_IRUGO, show_smb_alert_reg, NULL, SWPLD1_SMB_ALERT_REG_I210_SMB_ALRT_N);
static SENSOR_DEVICE_ATTR(vr_3v3_alrt_n,   S_IRUGO, show_smb_alert_reg, NULL, SWPLD1_SMB_ALERT_REG_3V3_VR_ALRT_N);
static SENSOR_DEVICE_ATTR(vcore_alrt_n,    S_IRUGO, show_smb_alert_reg, NULL, SWPLD1_SMB_ALERT_REG_VCORE_ALRT_N);
static SENSOR_DEVICE_ATTR(vr_0v8_alrt_n,   S_IRUGO, show_smb_alert_reg, NULL, SWPLD1_SMB_ALERT_REG_0V8_VR_ALRT_N);

static SENSOR_DEVICE_ATTR(cpu_thrml_int_n, S_IRUGO, show_vr_alert_reg, NULL, SWPLD1_VR_ALERT_REG_CPU_THRML_INT_N);
static SENSOR_DEVICE_ATTR(vr_3v3_hot_n,    S_IRUGO, show_vr_alert_reg, NULL, SWPLD1_VR_ALERT_REG_3V3_VR_HOT);
static SENSOR_DEVICE_ATTR(vcore_hot_n,     S_IRUGO, show_vr_alert_reg, NULL, SWPLD1_VR_ALERT_REG_VCORE_HOT);
static SENSOR_DEVICE_ATTR(vr_0v8_hot_n,    S_IRUGO, show_vr_alert_reg, NULL, SWPLD1_VR_ALERT_REG_0V8_VR_HOT);
static SENSOR_DEVICE_ATTR(vr_3v3_fault_n,  S_IRUGO, show_vr_alert_reg, NULL, SWPLD1_VR_ALERT_REG_3V3_VR_FAULT);
static SENSOR_DEVICE_ATTR(vcore_fault_n,   S_IRUGO, show_vr_alert_reg, NULL, SWPLD1_VR_ALERT_REG_VCORE_FAULT);
static SENSOR_DEVICE_ATTR(vr_0v8_fault_n,  S_IRUGO, show_vr_alert_reg, NULL, SWPLD1_VR_ALERT_REG_0V8_VR_FAULT);

static SENSOR_DEVICE_ATTR(mac_pcie_alrt_n,  S_IRUGO, show_pcie_alert_reg, NULL, SWPLD1_PCIE_ALERT_REG_PCIE_ALRT_N);
static SENSOR_DEVICE_ATTR(synce_prs_n,      S_IRUGO, show_pcie_alert_reg, NULL, SWPLD1_PCIE_ALERT_REG_SYNCE_PRS_N);
static SENSOR_DEVICE_ATTR(mac_pcie_wake_n,  S_IRUGO, show_pcie_alert_reg, NULL, SWPLD1_PCIE_ALERT_REG_MAC_PCIE_WAKE_N);
static SENSOR_DEVICE_ATTR(i210_pcie_wake_n, S_IRUGO, show_pcie_alert_reg, NULL, SWPLD1_PCIE_ALERT_REG_I210_PCIE_WAKE_N);

static SENSOR_DEVICE_ATTR(led_psu2, S_IRUGO | S_IWUSR, show_fp_led1_reg, set_fp_led1_reg, SWPLD1_FP_LED1_REG_PSU2_LED);
static SENSOR_DEVICE_ATTR(led_psu1, S_IRUGO | S_IWUSR, show_fp_led1_reg, set_fp_led1_reg, SWPLD1_FP_LED1_REG_PSU1_LED);

static SENSOR_DEVICE_ATTR(led_fan, S_IRUGO | S_IWUSR, show_fp_led2_reg, set_fp_led2_reg, SWPLD1_FP_LED2_REG_FAN_LED);
static SENSOR_DEVICE_ATTR(led_sys, S_IRUGO | S_IWUSR, show_fp_led2_reg, set_fp_led2_reg, SWPLD1_FP_LED2_REG_SYS_LED);

static SENSOR_DEVICE_ATTR(fan4_led, S_IRUGO | S_IWUSR, show_fan_led1_reg, set_fan_led1_reg, SWPLD1_FAN_LED1_REG_FAN4_LED);
static SENSOR_DEVICE_ATTR(fan3_led, S_IRUGO | S_IWUSR, show_fan_led1_reg, set_fan_led1_reg, SWPLD1_FAN_LED1_REG_FAN3_LED);
static SENSOR_DEVICE_ATTR(fan2_led, S_IRUGO | S_IWUSR, show_fan_led1_reg, set_fan_led1_reg, SWPLD1_FAN_LED1_REG_FAN2_LED);
static SENSOR_DEVICE_ATTR(fan1_led, S_IRUGO | S_IWUSR, show_fan_led1_reg, set_fan_led1_reg, SWPLD1_FAN_LED1_REG_FAN1_LED);

static SENSOR_DEVICE_ATTR(fan6_led, S_IRUGO | S_IWUSR, show_fan_led2_reg, set_fan_led2_reg, SWPLD1_FAN_LED2_REG_FAN6_LED);
static SENSOR_DEVICE_ATTR(fan5_led, S_IRUGO | S_IWUSR, show_fan_led2_reg, set_fan_led2_reg, SWPLD1_FAN_LED2_REG_FAN5_LED);

static SENSOR_DEVICE_ATTR(console_sel,     S_IRUGO | S_IWUSR, show_misc_sel_reg, set_misc_sel_reg, SWPLD1_MISC_SEL_REG_CONSOLE_SEL);

static struct attribute *nokia_7220_h3_swpld1_attributes[] = {
    &sensor_dev_attr_swbd_id.dev_attr.attr,
    &sensor_dev_attr_swbd_version.dev_attr.attr,   
    &sensor_dev_attr_cpld_type.dev_attr.attr,
    &sensor_dev_attr_cpld_version.dev_attr.attr,    
    &sensor_dev_attr_scratch.dev_attr.attr,
    
    &sensor_dev_attr_psu2_alert.dev_attr.attr,
    &sensor_dev_attr_psu2_ok.dev_attr.attr, 
    &sensor_dev_attr_psu2_pres.dev_attr.attr,
    &sensor_dev_attr_psu1_alert.dev_attr.attr,
    &sensor_dev_attr_psu1_ok.dev_attr.attr, 
    &sensor_dev_attr_psu1_pres.dev_attr.attr, 

    &sensor_dev_attr_psu2_eeprom_wp.dev_attr.attr, 
    &sensor_dev_attr_psu2_pson.dev_attr.attr,
    &sensor_dev_attr_psu1_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_psu1_pson.dev_attr.attr, 

    &sensor_dev_attr_vcc_mac_1v2.dev_attr.attr, 
    &sensor_dev_attr_vcc_bmc_1v15.dev_attr.attr,
    &sensor_dev_attr_vcc_bmc_1v2.dev_attr.attr,
    &sensor_dev_attr_vcc_mac_1v8.dev_attr.attr,
    &sensor_dev_attr_vcc_2V5.dev_attr.attr, 
    &sensor_dev_attr_vcc_3v3_ct.dev_attr.attr,
    &sensor_dev_attr_vcc_3v3.dev_attr.attr,
    &sensor_dev_attr_vcc_5v.dev_attr.attr,
    
    &sensor_dev_attr_vcc_mac_pll_0v8.dev_attr.attr,
    &sensor_dev_attr_vcc_mac_0v8.dev_attr.attr,
    &sensor_dev_attr_vcc_mac_avs_0v91.dev_attr.attr,

    &sensor_dev_attr_bcm_avs0.dev_attr.attr,
    &sensor_dev_attr_bcm_avs1.dev_attr.attr,
    &sensor_dev_attr_bcm_avs2.dev_attr.attr,
    &sensor_dev_attr_bcm_avs3.dev_attr.attr,
    &sensor_dev_attr_bcm_avs4.dev_attr.attr,
    &sensor_dev_attr_bcm_avs5.dev_attr.attr,
    &sensor_dev_attr_bcm_avs6.dev_attr.attr,
    &sensor_dev_attr_bcm_avs7.dev_attr.attr,

    &sensor_dev_attr_fan_alert_n.dev_attr.attr,
    &sensor_dev_attr_psu_int_n.dev_attr.attr,

    &sensor_dev_attr_swpld2_int_n.dev_attr.attr,
    &sensor_dev_attr_swpld3_int_n.dev_attr.attr,

    &sensor_dev_attr_cpld_misc_int_n.dev_attr.attr,
    &sensor_dev_attr_cpld_op_mod_int_n.dev_attr.attr,
    &sensor_dev_attr_cpld_psu_fan_int_n.dev_attr.attr,

    &sensor_dev_attr_synce_int_n.dev_attr.attr,
    &sensor_dev_attr_i210_smb_alrt_n.dev_attr.attr,
    &sensor_dev_attr_vr_3v3_alrt_n.dev_attr.attr,
    &sensor_dev_attr_vcore_alrt_n.dev_attr.attr,
    &sensor_dev_attr_vr_0v8_alrt_n.dev_attr.attr,

    &sensor_dev_attr_cpu_thrml_int_n.dev_attr.attr,
    &sensor_dev_attr_vr_3v3_hot_n.dev_attr.attr,
    &sensor_dev_attr_vcore_hot_n.dev_attr.attr,
    &sensor_dev_attr_vr_0v8_hot_n.dev_attr.attr,
    &sensor_dev_attr_vr_3v3_fault_n.dev_attr.attr,
    &sensor_dev_attr_vcore_fault_n.dev_attr.attr,
    &sensor_dev_attr_vr_0v8_fault_n.dev_attr.attr,

    &sensor_dev_attr_mac_pcie_alrt_n.dev_attr.attr,
    &sensor_dev_attr_synce_prs_n.dev_attr.attr,
    &sensor_dev_attr_mac_pcie_wake_n.dev_attr.attr,
    &sensor_dev_attr_i210_pcie_wake_n.dev_attr.attr,

    &sensor_dev_attr_led_psu2.dev_attr.attr,
    &sensor_dev_attr_led_psu1.dev_attr.attr,

    &sensor_dev_attr_led_fan.dev_attr.attr,
    &sensor_dev_attr_led_sys.dev_attr.attr,

    &sensor_dev_attr_fan4_led.dev_attr.attr,
    &sensor_dev_attr_fan3_led.dev_attr.attr,
    &sensor_dev_attr_fan2_led.dev_attr.attr,
    &sensor_dev_attr_fan1_led.dev_attr.attr,

    &sensor_dev_attr_fan6_led.dev_attr.attr,
    &sensor_dev_attr_fan5_led.dev_attr.attr,
    
    &sensor_dev_attr_console_sel.dev_attr.attr,
    NULL
};

static const struct attribute_group nokia_7220_h3_swpld1_group = {
    .attrs = nokia_7220_h3_swpld1_attributes,
};

static int nokia_7220_h3_swpld1_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H3 SWPLD1 chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &nokia_7220_h3_swpld1_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->swbd_id = nokia_7220_h3_swpld_read(data, SWPLD1_SWBD_ID_REG);
    data->swbd_version = nokia_7220_h3_swpld_read(data, SWPLD1_SWBD_VER_REG); 
    data->cpld_type = nokia_7220_h3_swpld_read(data, SWPLD1_CPLD_REV_REG) >> SWPLD1_CPLD_REV_REG_TYPE; 
    data->cpld_version = nokia_7220_h3_swpld_read(data, SWPLD1_CPLD_REV_REG) & SWPLD1_CPLD_REV_REG_MSK;
   
    return 0;

exit:
    return status;
}

static void nokia_7220_h3_swpld1_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &nokia_7220_h3_swpld1_group);
    kfree(data);
}

static const struct of_device_id nokia_7220_h3_swpld1_of_ids[] = {
    {
        .compatible = "nokia,7220_h3_swpld1",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, nokia_7220_h3_swpld1_of_ids);

static const struct i2c_device_id nokia_7220_h3_swpld1_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, nokia_7220_h3_swpld1_ids);

static struct i2c_driver nokia_7220_h3_swpld1_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(nokia_7220_h3_swpld1_of_ids),
    },
    .probe        = nokia_7220_h3_swpld1_probe,
    .remove       = nokia_7220_h3_swpld1_remove,
    .id_table     = nokia_7220_h3_swpld1_ids,
    .address_list = cpld_address_list,
};

static int __init nokia_7220_h3_swpld1_init(void)
{
    return i2c_add_driver(&nokia_7220_h3_swpld1_driver);
}

static void __exit nokia_7220_h3_swpld1_exit(void)
{
    i2c_del_driver(&nokia_7220_h3_swpld1_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H3 CPLD driver");
MODULE_LICENSE("GPL");

module_init(nokia_7220_h3_swpld1_init);
module_exit(nokia_7220_h3_swpld1_exit);
