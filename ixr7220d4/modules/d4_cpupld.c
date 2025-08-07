//  * CPLD driver for Nokia-7220-IXR-D4
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
#include <linux/delay.h>

#define DRIVER_NAME "d4_cpupld"

// REGISTERS ADDRESS MAP
#define BOARD_INFO_REG          0x00
#define CPLD_VER_REG            0x01
#define WATCHDOG_REG1           0x02
#define WATCHDOG_REG2           0x03
#define SYSTEM_RST_REG          0x04
#define PWR_RAIL_REG1           0x05
#define PWR_RAIL_REG2           0x06
#define MISC_REG                0x07
#define CPU_STATUS_REG          0x08
#define LAST_RST_REG            0x24


// REG BIT FIELD POSITION or MASK
#define BOARD_INFO_REG_PCB_VER_MSK            0xF
#define BOARD_INFO_REG_PWR_CAT                0x4
#define WATCHDOG_REG1_LT_SPI_CS_SEL           0x3
#define WATCHDOG_REG1_SPI_CS_SEL              0x4
#define WATCHDOG_REG1_RST_LOG_CLR             0x6
#define WATCHDOG_REG1_OS_DONE                 0x7
#define WATCHDOG_REG2_TEST_MODE               0x0
#define WATCHDOG_REG2_WDT                     0x1
#define WATCHDOG_REG2_BOOT_DEV_SEL            0x2
#define WATCHDOG_REG2_UPD_BOOT_DEV_SEL        0x3
#define WATCHDOG_REG2_WATCHDOG_EN             0x4
#define SOFT_RST                              0x0
#define CPU_JTAG_RST                          0x1
#define COLD_RST                              0x3
#define I210_RST_L_MASK                       0x5
#define PWR_RAIL_REG1_PWRGD_P3V3              0x0
#define PWR_RAIL_REG1_PWRGD_P1V5_PCH          0x1
#define PWR_RAIL_REG1_PWRGD_P1V05_PCH         0x2
#define PWR_RAIL_REG1_PWRGD_P0V6_VTT_DIMM     0x3
#define PWR_RAIL_REG1_PWRGD_DDR4_VPP          0x4
#define PWR_RAIL_REG1_PCH_SLP_S3_N            0x5
#define PWR_RAIL_REG1_CPU_XDP_SYSPWROK        0x6
#define PWR_RAIL_REG1_C33_BDX_PWRGOOD_CPU     0x7
#define PWR_RAIL_REG2_VR_P1V2_VDDQ            0x0
#define PWR_RAIL_REG2_PVCCSCFUSESUS           0x1
#define PWR_RAIL_REG2_PVCCKRHV                0x2
#define PWR_RAIL_REG2_PVCCIN                  0x3
#define PWR_RAIL_REG2_P5V_STBY                0x4
#define MISC_JTAG_SEL                         0x3
#define CPU_STATUS_REG_LSB_MSK                0xF
#define CPU_STATUS_REG_MSB                    0x4


static const unsigned short cpld_address_list[] = {0x65, I2C_CLIENT_END};

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

/* ---------------- */

static ssize_t show_pcb_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    char *str_ver = NULL;
    u8 val = 0;
    val = cpld_i2c_read(data, BOARD_INFO_REG) & BOARD_INFO_REG_PCB_VER_MSK;

    switch (val) {
    case 0xA:
        str_ver = "R0A";
        break;
    case 0xB:
        str_ver = "R0B";
        break;
    default: // 0x1
        str_ver = "R01";
        break;
    }

    return sprintf(buf, "0x%x %s\n", val, str_ver);
}

/* ---------------- */

static ssize_t show_board_power_cat(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, BOARD_INFO_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_cpld_ver(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, CPLD_VER_REG);
    return sprintf(buf, "0x%02x\n", val);
}

/* ---------------- */

static ssize_t show_watchdog1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, WATCHDOG_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_watchdog1(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, WATCHDOG_REG1);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, WATCHDOG_REG1, (reg_val | usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_watchdog2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, WATCHDOG_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_watchdog2(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, WATCHDOG_REG2);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, WATCHDOG_REG2, (reg_val | usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_system_rst(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, SYSTEM_RST_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_system_rst(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, SYSTEM_RST_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, SYSTEM_RST_REG, (reg_val | usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_pwr_rail1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, PWR_RAIL_REG1);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_pwr_rail2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, PWR_RAIL_REG2);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

/* ---------------- */

static ssize_t show_misc(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    struct sensor_device_attribute *sda = to_sensor_dev_attr(devattr);
    u8 val = 0;
    val = cpld_i2c_read(data, MISC_REG);

    return sprintf(buf, "%d\n", (val>>sda->index) & 0x1 ? 1:0);
}

static ssize_t set_misc(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
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
    reg_val = cpld_i2c_read(data, MISC_REG);
    reg_val = reg_val & mask;
    usr_val = usr_val << sda->index;
    cpld_i2c_write(data, MISC_REG, (reg_val | usr_val));

    return count;
}

/* ---------------- */

static ssize_t show_lsb_post_code(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, CPU_STATUS_REG) & CPU_STATUS_REG_LSB_MSK;
    return sprintf(buf, "0x%x\n", val);
}

static ssize_t show_msb_post_code(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    u8 val = 0;
    val = cpld_i2c_read(data, CPU_STATUS_REG) >> CPU_STATUS_REG_MSB;
    return sprintf(buf, "0x%x\n", val);
}

/* ---------------- */

static ssize_t show_last_rst(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct cpld_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%02x\n", data->reset_cause);
}



// sysfs attributes
static SENSOR_DEVICE_ATTR(pcb_ver, S_IRUGO, show_pcb_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(board_power_cat, S_IRUGO, show_board_power_cat, NULL, 0);
static SENSOR_DEVICE_ATTR(cpld_ver, S_IRUGO, show_cpld_ver, NULL, 0);
static SENSOR_DEVICE_ATTR(wd1_lt_spi_cs_sel, S_IRUGO, show_watchdog1, NULL, WATCHDOG_REG1_LT_SPI_CS_SEL);
static SENSOR_DEVICE_ATTR(wd1_spi_cs_sel, S_IRUGO, show_watchdog1, NULL, WATCHDOG_REG1_SPI_CS_SEL);
static SENSOR_DEVICE_ATTR(wd1_rst_log_clr, S_IRUGO | S_IWUSR, show_watchdog1, set_watchdog1, WATCHDOG_REG1_RST_LOG_CLR);
static SENSOR_DEVICE_ATTR(wd1_os_done, S_IRUGO | S_IWUSR, show_watchdog1, set_watchdog1, WATCHDOG_REG1_OS_DONE);
static SENSOR_DEVICE_ATTR(wd2_test_mode, S_IRUGO | S_IWUSR, show_watchdog2, set_watchdog2, WATCHDOG_REG2_TEST_MODE);
static SENSOR_DEVICE_ATTR(wd2_wdt, S_IRUGO | S_IWUSR, show_watchdog2, set_watchdog2, WATCHDOG_REG2_WDT);
static SENSOR_DEVICE_ATTR(wd2_boot_dev_sel, S_IRUGO | S_IWUSR, show_watchdog2, set_watchdog2, WATCHDOG_REG2_BOOT_DEV_SEL);
static SENSOR_DEVICE_ATTR(wd2_upd_boot_dev_sel, S_IRUGO | S_IWUSR, show_watchdog2, set_watchdog2, WATCHDOG_REG2_UPD_BOOT_DEV_SEL);
static SENSOR_DEVICE_ATTR(wd2_enable, S_IRUGO | S_IWUSR, show_watchdog2, set_watchdog2, WATCHDOG_REG2_WATCHDOG_EN);
static SENSOR_DEVICE_ATTR(soft_rst, S_IRUGO | S_IWUSR, show_system_rst, set_system_rst, SOFT_RST);
static SENSOR_DEVICE_ATTR(cpu_jtag_rst, S_IRUGO | S_IWUSR, show_system_rst, set_system_rst, CPU_JTAG_RST);
static SENSOR_DEVICE_ATTR(cold_rst, S_IRUGO | S_IWUSR, show_system_rst, set_system_rst, COLD_RST);
static SENSOR_DEVICE_ATTR(i210_rst_l_mask, S_IRUGO | S_IWUSR, show_system_rst, set_system_rst, I210_RST_L_MASK);
static SENSOR_DEVICE_ATTR(pwr_rail1_p3v3_pg, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_PWRGD_P3V3);
static SENSOR_DEVICE_ATTR(pwr_rail1_p1v5_pg, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_PWRGD_P1V5_PCH);
static SENSOR_DEVICE_ATTR(pwr_rail1_p1v05_pg, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_PWRGD_P1V05_PCH);
static SENSOR_DEVICE_ATTR(pwr_rail1_p0v6_vtt_pg, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_PWRGD_P0V6_VTT_DIMM);
static SENSOR_DEVICE_ATTR(pwr_rail1_ddr4_vpp_pg, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_PWRGD_DDR4_VPP);
static SENSOR_DEVICE_ATTR(pwr_rail1_slp_s3_n, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_PCH_SLP_S3_N);
static SENSOR_DEVICE_ATTR(pwr_rail1_xdp_syspwok, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_CPU_XDP_SYSPWROK);
static SENSOR_DEVICE_ATTR(pwr_rail1_procpwrgd_pch, S_IRUGO, show_pwr_rail1, NULL, PWR_RAIL_REG1_C33_BDX_PWRGOOD_CPU);
static SENSOR_DEVICE_ATTR(pwr_rail2_p1v2, S_IRUGO, show_pwr_rail2, NULL, PWR_RAIL_REG2_VR_P1V2_VDDQ);
static SENSOR_DEVICE_ATTR(pwr_rail2_pvccscfusesus, S_IRUGO, show_pwr_rail2, NULL, PWR_RAIL_REG2_PVCCSCFUSESUS);
static SENSOR_DEVICE_ATTR(pwr_rail2_pvcckrhv, S_IRUGO, show_pwr_rail2, NULL, PWR_RAIL_REG2_PVCCKRHV);
static SENSOR_DEVICE_ATTR(pwr_rail2_p1v8, S_IRUGO, show_pwr_rail2, NULL, PWR_RAIL_REG2_PVCCIN);
static SENSOR_DEVICE_ATTR(pwr_rail2_p5v, S_IRUGO, show_pwr_rail2, NULL, PWR_RAIL_REG2_P5V_STBY);
static SENSOR_DEVICE_ATTR(jtag_sel, S_IRUGO | S_IWUSR, show_misc, set_misc, MISC_JTAG_SEL);
static SENSOR_DEVICE_ATTR(lsb_post_code, S_IRUGO, show_lsb_post_code, NULL, 0);
static SENSOR_DEVICE_ATTR(msb_post_code, S_IRUGO, show_msb_post_code, NULL, 0);
static SENSOR_DEVICE_ATTR(reset_cause, S_IRUGO, show_last_rst, NULL, 0);

static struct attribute *d4_cpupld_attributes[] = {
    &sensor_dev_attr_pcb_ver.dev_attr.attr,
    &sensor_dev_attr_board_power_cat.dev_attr.attr,
    &sensor_dev_attr_cpld_ver.dev_attr.attr,
    &sensor_dev_attr_wd1_lt_spi_cs_sel.dev_attr.attr,
    &sensor_dev_attr_wd1_spi_cs_sel.dev_attr.attr,
    &sensor_dev_attr_wd1_rst_log_clr.dev_attr.attr,
    &sensor_dev_attr_wd1_os_done.dev_attr.attr,
    &sensor_dev_attr_wd2_test_mode.dev_attr.attr,
    &sensor_dev_attr_wd2_wdt.dev_attr.attr,
    &sensor_dev_attr_wd2_boot_dev_sel.dev_attr.attr,
    &sensor_dev_attr_wd2_upd_boot_dev_sel.dev_attr.attr,
    &sensor_dev_attr_wd2_enable.dev_attr.attr,
    &sensor_dev_attr_soft_rst.dev_attr.attr,
    &sensor_dev_attr_cpu_jtag_rst.dev_attr.attr,
    &sensor_dev_attr_cold_rst.dev_attr.attr,
    &sensor_dev_attr_i210_rst_l_mask.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_p3v3_pg.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_p1v5_pg.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_p1v05_pg.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_p0v6_vtt_pg.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_ddr4_vpp_pg.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_slp_s3_n.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_xdp_syspwok.dev_attr.attr,
    &sensor_dev_attr_pwr_rail1_procpwrgd_pch.dev_attr.attr,
    &sensor_dev_attr_pwr_rail2_p1v2.dev_attr.attr,
    &sensor_dev_attr_pwr_rail2_pvccscfusesus.dev_attr.attr,
    &sensor_dev_attr_pwr_rail2_pvcckrhv.dev_attr.attr,
    &sensor_dev_attr_pwr_rail2_p1v8.dev_attr.attr,
    &sensor_dev_attr_pwr_rail2_p5v.dev_attr.attr,
    &sensor_dev_attr_jtag_sel.dev_attr.attr,
    &sensor_dev_attr_lsb_post_code.dev_attr.attr,
    &sensor_dev_attr_msb_post_code.dev_attr.attr,
    &sensor_dev_attr_reset_cause.dev_attr.attr,
    NULL
};

static const struct attribute_group d4_cpupld_group = {
    .attrs = d4_cpupld_attributes,
};

static int d4_cpupld_probe(struct i2c_client *client)
{
    int status;
    struct cpld_data *data = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "CPLD PROBE ERROR: i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "Nokia-7220-IXR-D4 CPU CPLD chip found.\n");
    data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_err(&client->dev, "CPLD PROBE ERROR: Can't allocate memory\n");
        status = -ENOMEM;
        goto exit;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    status = sysfs_create_group(&client->dev.kobj, &d4_cpupld_group);
    if (status) {
        dev_err(&client->dev, "CPLD INIT ERROR: Cannot create sysfs\n");
        goto exit;
    }

    data->reset_cause = cpld_i2c_read(data, LAST_RST_REG);
    u8 val = cpld_i2c_read(data, WATCHDOG_REG1);
    cpld_i2c_write(data, WATCHDOG_REG1, val | 0x40);
    dev_info(&client->dev, "[CPU CPLD]: Clear RST reason\n");
    msleep(200);
    cpld_i2c_write(data, WATCHDOG_REG1, val & 0xBF);
    msleep(200);
    dev_info(&client->dev, "[CPU CPLD]: Clear RST reason .. done\n");

    return 0;

exit:
    return status;
}

static void d4_cpupld_remove(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &d4_cpupld_group);
    kfree(data);
}

static const struct of_device_id d4_cpupld_of_ids[] = {
    {
        .compatible = "nokia,d4_cpupld",
        .data       = (void *) 0,
    },
    { },
};
MODULE_DEVICE_TABLE(of, d4_cpupld_of_ids);

static const struct i2c_device_id d4_cpupld_ids[] = {
    { DRIVER_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, d4_cpupld_ids);

static struct i2c_driver d4_cpupld_driver = {
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = of_match_ptr(d4_cpupld_of_ids),
    },
    .probe        = d4_cpupld_probe,
    .remove       = d4_cpupld_remove,
    .id_table     = d4_cpupld_ids,
    .address_list = cpld_address_list,
};

static int __init d4_cpupld_init(void)
{
    return i2c_add_driver(&d4_cpupld_driver);
}

static void __exit d4_cpupld_exit(void)
{
    i2c_del_driver(&d4_cpupld_driver);
}

MODULE_AUTHOR("Nokia");
MODULE_DESCRIPTION("NOKIA-7220-IXR-D4 CPLD driver");
MODULE_LICENSE("GPL");

module_init(d4_cpupld_init);
module_exit(d4_cpupld_exit);
