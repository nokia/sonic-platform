//  * CPLD driver for Nokia-7220-IXR-H4-32D
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

#define DRIVER_NAME "h4_32d_cpupld"

// REGISTERS ADDRESS MAP
#define CODE_REV_REG                0x00
#define SCRATCH_REG                 0x01
#define BOARD_INFO_REG              0x02
#define BIOS_CTRL_REG               0x03
#define EEPROM_CTRL_REG             0x04
#define MARGIN_CTRL_REG             0x05
#define WATCHDOG_REG                0x06
#define RST_CAUSE_REG               0x08
#define PWR_CTRL_REG0               0x09
#define PWR_STATUS_REG0             0x0A
#define PWR_CTRL_REG1               0x0C
#define PWR_STATUS_REG1             0x0D
#define BOARD_REG0                  0x10
#define BOARD_REG1                  0x11
#define CPU_INT_REG0                0x15
#define CPU_INT_REG1                0x16
#define CPU_INT_REG2                0x17
#define RST_REG0                    0x20
#define RST_REG1                    0x21
#define RST_REG2                    0x22
#define HITLESS_REG                 0x39
#define CODE_DAY_REG                0xF0
#define CODE_MONTH_REG              0xF1
#define CODE_YEAR_REG               0xF2
#define TEST_CODE_REV_REG           0xF3


// REG BIT FIELD POSITION or MASK
#define BOARD_INFO_REG_TYPE_MSK     0xF
#define BOARD_INFO_REG_VER          0x4

#define BIOS_CTRL_REG_GLOBE_RST     0x1
#define BIOS_CTRL_REG_RST_BIOS_BAK  0x4
#define BIOS_CTRL_REG_BIOS_SEL      0x5

#define EEPROM_CTRL_REG_SPI_BIOS_WP 0x2

#define MARGIN_CTRL_REG_CTRL        0x0

#define WATCHDOG_REG_WD_PUNCH       0x0
#define WATCHDOG_REG_WD_EN          0x3
#define WATCHDOG_REG_WD_TIMER       0x4

#define PWR_CTRL_REG0_1V24_EN       0x0
#define PWR_CTRL_REG0_1V8_EN        0x1
#define PWR_CTRL_REG0_3V3_EN        0x2
#define PWR_CTRL_REG0_1V15_CPLD_EN  0x3
#define PWR_CTRL_REG0_1V15_RAM_EN   0x4
#define PWR_CTRL_REG0_1V05_EN       0x5
#define PWR_CTRL_REG0_1V05_VNN_EN   0x6
#define PWR_CTRL_REG0_1V2_VDDQ_EN   0x7

#define PWR_STATUS_REG0_1V24        0x0
#define PWR_STATUS_REG0_1V8         0x1
#define PWR_STATUS_REG0_3V3         0x2
#define PWR_STATUS_REG0_1V15        0x3
#define PWR_STATUS_REG0_1V15_RAM    0x4
#define PWR_STATUS_REG0_1V05        0x5
#define PWR_STATUS_REG0_1V05_VNN    0x6
#define PWR_STATUS_REG0_1V2_VDDQ    0x7

#define PWR_CTRL_REG1_2V5_VPP_EN    0x0
#define PWR_CTRL_REG1_0V6_VTT_EN    0x1

#define PWR_STATUS_REG1_2V5_VPP     0x0
#define PWR_STATUS_REG1_0V6_VTT     0x1
#define PWR_STATUS_REG1_MB_PWR      0x6
#define PWR_STATUS_REG1_HW_EN       0x7

#define BOARD_REG0_BOOT_SUCCESS     0x0
#define BOARD_REG0_BIOS_WD_EN       0x3
#define BOARD_REG0_BOOT_TIMER       0x4
#define BOARD_REG0_BIOS_REC         0x7

#define BOARD_REG1_RMT_ACCESS       0x3
#define BOARD_REG1_USB_OC           0x4
#define BOARD_REG1_THERMAL_IN       0x5
#define BOARD_REG1_THERMAL_OUT      0x6
#define BOARD_REG1_TPM_PIRQ         0x7

#define CPU_INT_REG0_THERMTRIP      0x1
#define CPU_INT_REG0_HOT_CPLD       0x2
#define CPU_INT_REG0_THERMTRIP_MSK  0x5
#define CPU_INT_REG0_HOT_CPLD_MSK   0x6

#define CPU_INT_REG1_TMP75          0x0
#define CPU_INT_REG1_MCERR          0x1
#define CPU_INT_REG1_IERR           0x2
#define CPU_INT_REG1_FATAL          0x3
#define CPU_INT_REG1_TMP75_MSK      0x4
#define CPU_INT_REG1_MCERR_MSK      0x5
#define CPU_INT_REG1_IERR_MSK       0x6
#define CPU_INT_REG1_FATAL_MSK      0x7

#define CPU_INT_REG2_MB             0x0
#define CPU_INT_REG2_OP_MOD         0x1
#define CPU_INT_REG2_PSU_FAN        0x2
#define CPU_INT_REG2_PSU_PWR        0x3
#define CPU_INT_REG2_MB_MSK         0x4
#define CPU_INT_REG2_OP_MOD_MSK     0x5
#define CPU_INT_REG2_PSU_FAN_MSK    0x6
#define CPU_INT_REG2_PSU_PWR_MSK    0x7

#define RST_REG0_PLD_SOFT_RST       0x0
#define RST_REG0_CPU_PWR_DOWN       0x1
#define RST_REG0_CPU_RST_BTN        0x2
#define RST_REG0_RST_RSTIC          0x3

#define RST_REG1_RST_CPU_RTC        0x1
#define RST_REG1_CPU_RTEST          0x2
#define RST_REG1_ASYNC_RST          0x3
#define RST_REG1_RST_MR             0x4

#define RST_REG2_RST_TPM            0x0

#define HITLESS_REG_EN              0x0

static const unsigned short cpld_address_list[] = {0x31, I2C_CLIENT_END};

struct cpld_data {
    struct i2c_client *client;
    struct mutex  update_lock;
    int reset_cause;
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

static ssize_t show_code_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_REV_REG);
    return sprintf(buf, "0x%02x\n", val);
}

static ssize_t show_board_type(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *brd_type = NULL;
    u8 val = 0;
      
    val = cpld_i2c_read(data, BOARD_INFO_REG) & BOARD_INFO_REG_TYPE_MSK;
    
    switch (val) {
    case 0:
        brd_type = "H3 BROADWELL-DE CPU";
        break;
    case 1:
        brd_type = "H3 Denverton CPU Platform";
        break;
    case 2:
        brd_type = "H4 Denverton C3758R";
        break;
    default:
        brd_type = "RESERVED";
        break;
    }

    return sprintf(buf, "0x%x %s\n", val, brd_type);
}

static ssize_t show_board_ver(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *str_ver = NULL;
    u8 val = 0;
      
    val = cpld_i2c_read(data, BOARD_INFO_REG) >> BOARD_INFO_REG_VER;
    
    switch (val) {
    case 0:
        str_ver = "X00 (EVT)";
        break;
    case 1:
        str_ver = "X01 (DVT)";
        break;
    case 2:
        str_ver = "X02 (PVT)";
        break;    
    default:
        str_ver = "Reserved";
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

static ssize_t show_bios_ctrl(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, BIOS_CTRL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_bios_ctrl(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, BIOS_CTRL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, BIOS_CTRL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_eeprom_ctrl(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, EEPROM_CTRL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_eeprom_ctrl(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, EEPROM_CTRL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, EEPROM_CTRL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_margin_ctrl(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, MARGIN_CTRL_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x3);
}

static ssize_t set_margin_ctrl(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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

    mask = (~(3 << sda->index)) & 0xFF;
    reg_val = cpld_i2c_read(data, MARGIN_CTRL_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, MARGIN_CTRL_REG, (reg_val | usr_val));

    return count;
}

static ssize_t show_watchdog(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 reg_val;
    int ret;
    val = cpld_i2c_read(data, WATCHDOG_REG);    

    switch (sda->index) {
    case WATCHDOG_REG_WD_PUNCH:
    case WATCHDOG_REG_WD_EN:
        return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
    case WATCHDOG_REG_WD_TIMER:
        reg_val = (val>>sda->index) & 0x7;
        switch (reg_val) {
        case 0b000:
            ret = 15;
            break;
        case 0b001:
            ret = 20;
            break;
        case 0b010:
            ret = 30;
            break;
        case 0b011:
            ret = 40;
            break;
        case 0b100:
            ret = 50;
            break;
        case 0b101:
            ret = 60;
            break;
        case 0b110:
            ret = 65;
            break;
        case 0b111:
            ret = 70;
            break; 
        default:
            ret = 0;
            break;
        } 
        return sprintf(buf, "0x%x: %d seconds\n", reg_val, ret);  
    default:
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    }

}

static ssize_t set_watchdog(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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

    switch (sda->index) {
    case WATCHDOG_REG_WD_PUNCH:
    case WATCHDOG_REG_WD_EN:
        if (usr_val > 1) {
            return -EINVAL;
        }
        mask = (~(1 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, WATCHDOG_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, WATCHDOG_REG, (reg_val | usr_val));
        break;
    case WATCHDOG_REG_WD_TIMER:
        if (usr_val > 7) {
            return -EINVAL;
        }
        mask = (~(7 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, WATCHDOG_REG);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, WATCHDOG_REG, (reg_val | usr_val));
        break;
    default:        
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);     
    } 

    return count;
}

static ssize_t show_rst_cause(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%02x\n", data->reset_cause);
}

static ssize_t show_pwr_ctrl0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, PWR_CTRL_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_pwr_ctrl0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, PWR_CTRL_REG0);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, PWR_CTRL_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_pwr_status0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, PWR_STATUS_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_pwr_ctrl1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, PWR_CTRL_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_pwr_ctrl1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, PWR_CTRL_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, PWR_CTRL_REG1, (reg_val | usr_val));

    return count;
}

static ssize_t show_pwr_status1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, PWR_STATUS_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_board_reg0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    u8 reg_val;
    int ret;
    val = cpld_i2c_read(data, BOARD_REG0);

    switch (sda->index) {
    case BOARD_REG0_BOOT_SUCCESS:
    case BOARD_REG0_BIOS_WD_EN:
    case BOARD_REG0_BIOS_REC:
        return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
    case BOARD_REG0_BOOT_TIMER:
        reg_val = (val>>sda->index) & 0x7;
        switch (reg_val) {
        case 0b000:
            ret = 180;
            break;
        case 0b001:
            ret = 240;
            break;
        case 0b010:
            ret = 300;
            break;
        case 0b011:
            ret = 360;
            break;
        case 0b100:
            ret = 420;
            break;
        case 0b101:
            ret = 480;
            break;
        case 0b110:
            ret = 540;
            break;
        case 0b111:
            ret = 600;
            break; 
        default:
            ret = 0;
            break;
        } 
        return sprintf(buf, "0x%x: %d seconds\n", reg_val, ret);  
    default:
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    }
}

static ssize_t set_board_reg0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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

    switch (sda->index) {    
    case BOARD_REG0_BIOS_WD_EN:
        if (usr_val > 1) {
            return -EINVAL;
        }
        mask = (~(1 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, BOARD_REG0);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, BOARD_REG0, (reg_val | usr_val));
        break;
    case BOARD_REG0_BOOT_TIMER:
        if (usr_val > 7) {
            return -EINVAL;
        }
        mask = (~(7 << sda->index)) & 0xFF;
        reg_val = cpld_i2c_read(data, BOARD_REG0);
        reg_val = reg_val & mask;
        usr_val = usr_val << sda->index;
        cpld_i2c_write(data, BOARD_REG0, (reg_val | usr_val));
        break;
    default:        
        return sprintf(buf, "Error: Wrong bitwise(%d) to set!\n", sda->index);
    } 

    return count;
}

static ssize_t show_board_reg1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, BOARD_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_cpu_int0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, CPU_INT_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_cpu_int0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, CPU_INT_REG0);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, CPU_INT_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_cpu_int1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, CPU_INT_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_cpu_int1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, CPU_INT_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, CPU_INT_REG1, (reg_val | usr_val));

    return count;
}

static ssize_t show_cpu_int2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, CPU_INT_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_cpu_int2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, CPU_INT_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, CPU_INT_REG2, (reg_val | usr_val));

    return count;
}

static ssize_t show_rst0(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, RST_REG0);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_rst0(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, RST_REG0);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, RST_REG0, (reg_val | usr_val));

    return count;
}

static ssize_t show_rst1(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, RST_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_rst1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, RST_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, RST_REG1, (reg_val | usr_val));

    return count;
}

static ssize_t show_rst2(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, RST_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_rst2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
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
    reg_val = cpld_i2c_read(data, RST_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, RST_REG2, (reg_val | usr_val));

    return count;
}

static ssize_t show_hitless(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, HITLESS_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t show_code_day(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_DAY_REG);
    return sprintf(buf, "%d\n", val);
}

static ssize_t show_code_month(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_MONTH_REG);
    return sprintf(buf, "%d\n", val);
}

static ssize_t show_code_year(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
      
    val = cpld_i2c_read(data, CODE_YEAR_REG);
    return sprintf(buf, "%d\n", val);
}

// sysfs attributes 
static SENSOR_DEVICE_ATTR(code_ver, S_IRUGO, show_code_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(board_type, S_IRUGO, show_board_type, NULL, 0);
static SENSOR_DEVICE_ATTR(board_ver, S_IRUGO, show_board_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(scratch, S_IRUGO | S_IWUSR, show_scratch, set_scratch, 0);
static SENSOR_DEVICE_ATTR(globe_rst, S_IRUGO | S_IWUSR, show_bios_ctrl, set_bios_ctrl, BIOS_CTRL_REG_GLOBE_RST);
static SENSOR_DEVICE_ATTR(rst_bios_bak, S_IRUGO | S_IWUSR, show_bios_ctrl, set_bios_ctrl, BIOS_CTRL_REG_RST_BIOS_BAK);
static SENSOR_DEVICE_ATTR(bios_sel, S_IRUGO | S_IWUSR, show_bios_ctrl, set_bios_ctrl, BIOS_CTRL_REG_BIOS_SEL);
static SENSOR_DEVICE_ATTR(bios_wp, S_IRUGO | S_IWUSR, show_eeprom_ctrl, set_eeprom_ctrl, EEPROM_CTRL_REG_SPI_BIOS_WP);
static SENSOR_DEVICE_ATTR(margin_ctrl, S_IRUGO | S_IWUSR, show_margin_ctrl, set_margin_ctrl, MARGIN_CTRL_REG_CTRL);
static SENSOR_DEVICE_ATTR(wd_punch, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_PUNCH);
static SENSOR_DEVICE_ATTR(wd_enable, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_EN);
static SENSOR_DEVICE_ATTR(wd_timer, S_IRUGO | S_IWUSR, show_watchdog, set_watchdog, WATCHDOG_REG_WD_TIMER);
static SENSOR_DEVICE_ATTR(reset_cause, S_IRUGO, show_rst_cause, NULL, 0);
static SENSOR_DEVICE_ATTR(pwr_1v24_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_1V24_EN);
static SENSOR_DEVICE_ATTR(pwr_1v8_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_1V8_EN);
static SENSOR_DEVICE_ATTR(pwr_3v3_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_3V3_EN);
static SENSOR_DEVICE_ATTR(pwr_1v15_cpld_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_1V15_CPLD_EN);
static SENSOR_DEVICE_ATTR(pwr_1v15_ram_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_1V15_RAM_EN);
static SENSOR_DEVICE_ATTR(pwr_1v05_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_1V05_EN);
static SENSOR_DEVICE_ATTR(pwr_1v05_vnn_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_1V05_VNN_EN);
static SENSOR_DEVICE_ATTR(pwr_1v2_vddq_en, S_IRUGO | S_IWUSR, show_pwr_ctrl0, set_pwr_ctrl0, PWR_CTRL_REG0_1V2_VDDQ_EN);
static SENSOR_DEVICE_ATTR(pwr_status_1v24, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_1V24);
static SENSOR_DEVICE_ATTR(pwr_status_1v8, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_1V8);
static SENSOR_DEVICE_ATTR(pwr_status_3v3, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_3V3);
static SENSOR_DEVICE_ATTR(pwr_status_1v15, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_1V15);
static SENSOR_DEVICE_ATTR(pwr_status_1v15_ram, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_1V15_RAM);
static SENSOR_DEVICE_ATTR(pwr_status_1v05, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_1V05);
static SENSOR_DEVICE_ATTR(pwr_status_1v05_vnn, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_1V05_VNN);
static SENSOR_DEVICE_ATTR(pwr_status_1v2_vddq, S_IRUGO, show_pwr_status0, NULL, PWR_STATUS_REG0_1V2_VDDQ);
static SENSOR_DEVICE_ATTR(pwr_2v5_vpp_en, S_IRUGO | S_IWUSR, show_pwr_ctrl1, set_pwr_ctrl1, PWR_CTRL_REG1_2V5_VPP_EN);
static SENSOR_DEVICE_ATTR(pwr_0v6_vtt_en, S_IRUGO | S_IWUSR, show_pwr_ctrl1, set_pwr_ctrl1, PWR_CTRL_REG1_0V6_VTT_EN);
static SENSOR_DEVICE_ATTR(pwr_status_2v5_vpp, S_IRUGO, show_pwr_status1, NULL, PWR_STATUS_REG1_2V5_VPP);
static SENSOR_DEVICE_ATTR(pwr_status_0v6_vtt, S_IRUGO, show_pwr_status1, NULL, PWR_STATUS_REG1_0V6_VTT);
static SENSOR_DEVICE_ATTR(pwr_status_mb_pwr, S_IRUGO, show_pwr_status1, NULL, PWR_STATUS_REG1_MB_PWR);
static SENSOR_DEVICE_ATTR(pwr_status_hw_en_pwr, S_IRUGO, show_pwr_status1, NULL, PWR_STATUS_REG1_HW_EN);
static SENSOR_DEVICE_ATTR(brd_boot_success, S_IRUGO, show_board_reg0, NULL, BOARD_REG0_BOOT_SUCCESS);
static SENSOR_DEVICE_ATTR(brd_bios_wd_en, S_IRUGO | S_IWUSR, show_board_reg0, set_board_reg0, BOARD_REG0_BIOS_WD_EN);
static SENSOR_DEVICE_ATTR(brd_boot_timer, S_IRUGO | S_IWUSR, show_board_reg0, set_board_reg0, BOARD_REG0_BOOT_TIMER);
static SENSOR_DEVICE_ATTR(brd_boot_bios_rec, S_IRUGO, show_board_reg0, NULL, BOARD_REG0_BIOS_REC);
static SENSOR_DEVICE_ATTR(brd_rmt_access, S_IRUGO, show_board_reg1, NULL, BOARD_REG1_RMT_ACCESS);
static SENSOR_DEVICE_ATTR(brd_usb_oc, S_IRUGO, show_board_reg1, NULL, BOARD_REG1_USB_OC);
static SENSOR_DEVICE_ATTR(brd_thermal_in, S_IRUGO, show_board_reg1, NULL, BOARD_REG1_THERMAL_IN);
static SENSOR_DEVICE_ATTR(brd_thermal_out, S_IRUGO, show_board_reg1, NULL, BOARD_REG1_THERMAL_OUT);
static SENSOR_DEVICE_ATTR(brd_tpm_pirq, S_IRUGO, show_board_reg1, NULL, BOARD_REG1_TPM_PIRQ);
static SENSOR_DEVICE_ATTR(int_thermtrip, S_IRUGO, show_cpu_int0, NULL, CPU_INT_REG0_THERMTRIP);
static SENSOR_DEVICE_ATTR(int_hot_cpld, S_IRUGO, show_cpu_int0, NULL, CPU_INT_REG0_HOT_CPLD);
static SENSOR_DEVICE_ATTR(int_thermtrip_msk, S_IRUGO | S_IWUSR, show_cpu_int0, set_cpu_int0, CPU_INT_REG0_THERMTRIP_MSK);
static SENSOR_DEVICE_ATTR(int_hot_cpld_msk, S_IRUGO | S_IWUSR, show_cpu_int0, set_cpu_int0, CPU_INT_REG0_HOT_CPLD_MSK);
static SENSOR_DEVICE_ATTR(int_tmp75, S_IRUGO, show_cpu_int1, NULL, CPU_INT_REG1_TMP75);
static SENSOR_DEVICE_ATTR(int_mcerr, S_IRUGO, show_cpu_int1, NULL, CPU_INT_REG1_MCERR);
static SENSOR_DEVICE_ATTR(int_ierr, S_IRUGO, show_cpu_int1, NULL, CPU_INT_REG1_IERR);
static SENSOR_DEVICE_ATTR(int_fatal_err, S_IRUGO, show_cpu_int1, NULL, CPU_INT_REG1_FATAL);
static SENSOR_DEVICE_ATTR(int_tmp75_msk, S_IRUGO | S_IWUSR, show_cpu_int1, set_cpu_int1, CPU_INT_REG1_TMP75_MSK);
static SENSOR_DEVICE_ATTR(int_mcerr_msk, S_IRUGO | S_IWUSR, show_cpu_int1, set_cpu_int1, CPU_INT_REG1_MCERR_MSK);
static SENSOR_DEVICE_ATTR(int_ierr_msk, S_IRUGO | S_IWUSR, show_cpu_int1, set_cpu_int1, CPU_INT_REG1_IERR_MSK);
static SENSOR_DEVICE_ATTR(int_fatal_err_msk, S_IRUGO | S_IWUSR, show_cpu_int1, set_cpu_int1, CPU_INT_REG1_FATAL_MSK);
static SENSOR_DEVICE_ATTR(int_mb, S_IRUGO, show_cpu_int2, NULL, CPU_INT_REG2_MB);
static SENSOR_DEVICE_ATTR(int_op_mod, S_IRUGO, show_cpu_int2, NULL, CPU_INT_REG2_OP_MOD);
static SENSOR_DEVICE_ATTR(int_psu_fan, S_IRUGO, show_cpu_int2, NULL, CPU_INT_REG2_PSU_FAN);
static SENSOR_DEVICE_ATTR(int_psu_pwr, S_IRUGO, show_cpu_int2, NULL, CPU_INT_REG2_PSU_PWR);
static SENSOR_DEVICE_ATTR(int_mb_msk, S_IRUGO | S_IWUSR, show_cpu_int2, set_cpu_int2, CPU_INT_REG2_MB_MSK);
static SENSOR_DEVICE_ATTR(int_op_mod_msk, S_IRUGO | S_IWUSR, show_cpu_int2, set_cpu_int2, CPU_INT_REG2_OP_MOD_MSK);
static SENSOR_DEVICE_ATTR(int_psu_fan_msk, S_IRUGO | S_IWUSR, show_cpu_int2, set_cpu_int2, CPU_INT_REG2_PSU_FAN_MSK);
static SENSOR_DEVICE_ATTR(int_psu_pwr_msk, S_IRUGO | S_IWUSR, show_cpu_int2, set_cpu_int2, CPU_INT_REG2_PSU_PWR_MSK);
static SENSOR_DEVICE_ATTR(rst_pld_soft, S_IRUGO | S_IWUSR, show_rst0, set_rst0, RST_REG0_PLD_SOFT_RST);
static SENSOR_DEVICE_ATTR(rst_pwr_down, S_IRUGO | S_IWUSR, show_rst0, set_rst0, RST_REG0_CPU_PWR_DOWN);
static SENSOR_DEVICE_ATTR(rst_cpu_btn, S_IRUGO | S_IWUSR, show_rst0, set_rst0, RST_REG0_CPU_RST_BTN);
static SENSOR_DEVICE_ATTR(rst_rstic, S_IRUGO | S_IWUSR, show_rst0, set_rst0, RST_REG0_RST_RSTIC);
static SENSOR_DEVICE_ATTR(rst_cpu_rtc, S_IRUGO | S_IWUSR, show_rst1, set_rst1, RST_REG1_RST_CPU_RTC);
static SENSOR_DEVICE_ATTR(rst_cpu_rtest, S_IRUGO | S_IWUSR, show_rst1, set_rst1, RST_REG1_CPU_RTEST);
static SENSOR_DEVICE_ATTR(rst_async, S_IRUGO | S_IWUSR, show_rst1, set_rst1, RST_REG1_ASYNC_RST);
static SENSOR_DEVICE_ATTR(rst_mr, S_IRUGO | S_IWUSR, show_rst1, set_rst1, RST_REG1_RST_MR);
static SENSOR_DEVICE_ATTR(rst_tpm, S_IRUGO | S_IWUSR, show_rst2, set_rst2, RST_REG2_RST_TPM);
static SENSOR_DEVICE_ATTR(hitless_en, S_IRUGO, show_hitless, NULL, HITLESS_REG_EN);
static SENSOR_DEVICE_ATTR(code_day, S_IRUGO, show_code_day, NULL, 0);
static SENSOR_DEVICE_ATTR(code_month, S_IRUGO, show_code_month, NULL, 0);
static SENSOR_DEVICE_ATTR(code_year, S_IRUGO, show_code_year, NULL, 0);

static struct attribute *h4_32d_cpupld_attributes[] = {
    &sensor_dev_attr_code_ver.dev_attr.attr,
    &sensor_dev_attr_board_type.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,
    &sensor_dev_attr_scratch.dev_attr.attr,
    &sensor_dev_attr_globe_rst.dev_attr.attr,
    &sensor_dev_attr_rst_bios_bak.dev_attr.attr,
    &sensor_dev_attr_bios_sel.dev_attr.attr,
    &sensor_dev_attr_bios_wp.dev_attr.attr,
    &sensor_dev_attr_margin_ctrl.dev_attr.attr,
    &sensor_dev_attr_wd_punch.dev_attr.attr,
    &sensor_dev_attr_wd_enable.dev_attr.attr,
    &sensor_dev_attr_wd_timer.dev_attr.attr,
    &sensor_dev_attr_reset_cause.dev_attr.attr,
    &sensor_dev_attr_pwr_1v24_en.dev_attr.attr,
    &sensor_dev_attr_pwr_1v8_en.dev_attr.attr,
    &sensor_dev_attr_pwr_3v3_en.dev_attr.attr,
    &sensor_dev_attr_pwr_1v15_cpld_en.dev_attr.attr,
    &sensor_dev_attr_pwr_1v15_ram_en.dev_attr.attr,
    &sensor_dev_attr_pwr_1v05_en.dev_attr.attr,
    &sensor_dev_attr_pwr_1v05_vnn_en.dev_attr.attr,
    &sensor_dev_attr_pwr_1v2_vddq_en.dev_attr.attr,
    &sensor_dev_attr_pwr_status_1v24.dev_attr.attr,
    &sensor_dev_attr_pwr_status_1v8.dev_attr.attr,
    &sensor_dev_attr_pwr_status_3v3.dev_attr.attr,
    &sensor_dev_attr_pwr_status_1v15.dev_attr.attr,
    &sensor_dev_attr_pwr_status_1v15_ram.dev_attr.attr,
    &sensor_dev_attr_pwr_status_1v05.dev_attr.attr,
    &sensor_dev_attr_pwr_status_1v05_vnn.dev_attr.attr,
    &sensor_dev_attr_pwr_status_1v2_vddq.dev_attr.attr,
    &sensor_dev_attr_pwr_2v5_vpp_en.dev_attr.attr,
    &sensor_dev_attr_pwr_0v6_vtt_en.dev_attr.attr,
    &sensor_dev_attr_pwr_status_2v5_vpp.dev_attr.attr,
    &sensor_dev_attr_pwr_status_0v6_vtt.dev_attr.attr,
    &sensor_dev_attr_pwr_status_mb_pwr.dev_attr.attr,
    &sensor_dev_attr_pwr_status_hw_en_pwr.dev_attr.attr,
    &sensor_dev_attr_brd_boot_success.dev_attr.attr,
    &sensor_dev_attr_brd_bios_wd_en.dev_attr.attr,
    &sensor_dev_attr_brd_boot_timer.dev_attr.attr,
    &sensor_dev_attr_brd_boot_bios_rec.dev_attr.attr,
    &sensor_dev_attr_brd_rmt_access.dev_attr.attr,
    &sensor_dev_attr_brd_usb_oc.dev_attr.attr,
    &sensor_dev_attr_brd_thermal_in.dev_attr.attr,
    &sensor_dev_attr_brd_thermal_out.dev_attr.attr,
    &sensor_dev_attr_brd_tpm_pirq.dev_attr.attr,
    &sensor_dev_attr_int_thermtrip.dev_attr.attr,
    &sensor_dev_attr_int_hot_cpld.dev_attr.attr,
    &sensor_dev_attr_int_thermtrip_msk.dev_attr.attr,
    &sensor_dev_attr_int_hot_cpld_msk.dev_attr.attr,
    &sensor_dev_attr_int_tmp75.dev_attr.attr,
    &sensor_dev_attr_int_mcerr.dev_attr.attr,
    &sensor_dev_attr_int_ierr.dev_attr.attr,
    &sensor_dev_attr_int_fatal_err.dev_attr.attr,
    &sensor_dev_attr_int_tmp75_msk.dev_attr.attr,
    &sensor_dev_attr_int_mcerr_msk.dev_attr.attr,
    &sensor_dev_attr_int_ierr_msk.dev_attr.attr,
    &sensor_dev_attr_int_fatal_err_msk.dev_attr.attr,
    &sensor_dev_attr_int_mb.dev_attr.attr,
    &sensor_dev_attr_int_op_mod.dev_attr.attr,
    &sensor_dev_attr_int_psu_fan.dev_attr.attr, 
    &sensor_dev_attr_int_psu_pwr.dev_attr.attr,
    &sensor_dev_attr_int_mb_msk.dev_attr.attr,
    &sensor_dev_attr_int_op_mod_msk.dev_attr.attr,
    &sensor_dev_attr_int_psu_fan_msk.dev_attr.attr, 
    &sensor_dev_attr_int_psu_pwr_msk.dev_attr.attr,
    &sensor_dev_attr_rst_pld_soft.dev_attr.attr,
    &sensor_dev_attr_rst_pwr_down.dev_attr.attr,
    &sensor_dev_attr_rst_cpu_btn.dev_attr.attr,
    &sensor_dev_attr_rst_rstic.dev_attr.attr,    
    &sensor_dev_attr_rst_cpu_rtc.dev_attr.attr,
    &sensor_dev_attr_rst_cpu_rtest.dev_attr.attr,
    &sensor_dev_attr_rst_async.dev_attr.attr,
    &sensor_dev_attr_rst_mr.dev_attr.attr,
    &sensor_dev_attr_rst_tpm.dev_attr.attr, 
    &sensor_dev_attr_hitless_en.dev_attr.attr,
    &sensor_dev_attr_code_day.dev_attr.attr,
    &sensor_dev_attr_code_month.dev_attr.attr,
    &sensor_dev_attr_code_year.dev_attr.attr,
    NULL
};

static const struct attribute_group h4_32d_cpupld_group = {
    .attrs = h4_32d_cpupld_attributes,
};

static int h4_32d_cpupld_probe(struct i2c_client *client)
{
    int status;
     struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-H4-32D CPUCPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &h4_32d_cpupld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->reset_cause = cpld_i2c_read(data, RST_CAUSE_REG);
    cpld_i2c_write(data, RST_CAUSE_REG, 0);

    return 0;

exit:
    return status;
}

static void h4_32d_cpupld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &h4_32d_cpupld_group);
    kfree(data);
}

static const struct of_device_id h4_32d_cpupld_of_ids[] = {
    {
        .compatible = "nokia,h4_32d_cpupld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, h4_32d_cpupld_of_ids);

static const struct i2c_device_id h4_32d_cpupld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, h4_32d_cpupld_ids);

static struct i2c_driver h4_32d_cpupld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(h4_32d_cpupld_of_ids),
    },
    .probe        = h4_32d_cpupld_probe,
    .remove       = h4_32d_cpupld_remove,
    .id_table     = h4_32d_cpupld_ids,
    .address_list = cpld_address_list,
};

static int __init h4_32d_cpupld_init(void)
{
    return i2c_add_driver(&h4_32d_cpupld_driver);
}

static void __exit h4_32d_cpupld_exit(void)
{
    i2c_del_driver(&h4_32d_cpupld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-H4-32D CPLD driver");
MODULE_LICENSE("GPL");

module_init(h4_32d_cpupld_init);
module_exit(h4_32d_cpupld_exit);
