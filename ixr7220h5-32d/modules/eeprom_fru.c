// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * eeprom_fru.c - NOKIA EEPROM FRU Sysfs driver
 *
 *
 * Copyright (C) 2024 Nokia Corporation.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * see <http://www.gnu.org/licenses/>
 */

#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>

#define EEPROM_NAME                 "eeprom_fru"
#define FIELD_LEN_MAX 255
#define VERBOSE 0
static unsigned int debug = 0;
module_param_named(debug, debug, uint, 0);
MODULE_PARM_DESC(debug, "Debug enable(default to 0)");

static unsigned int read_eeprom_max_len = 0xb7;
module_param_named(read_eeprom_max_len, read_eeprom_max_len, uint, 0);
MODULE_PARM_DESC(read_eeprom_max_len, "read_eeprom_max_len(default to 176)");

#define FRU_END_OF_FIELDS 0xc1
#define BUF2STR_MAXIMUM_OUTPUT_SIZE  (3*1024 + 1)
struct fru_header {
    u8 version;
    union {
        struct {
            u8 internal;
            u8 chassis;
            u8 board;
            u8 product;
            u8 multi;
        } offset;
        u8 offsets[5];
    };
    u8 pad;
    u8 checksum;
};

struct at24_data {
    /*
     * Lock protects against activities from other Linux tasks,
     * but not from changes by other I2C masters.
     */
    struct mutex lock;
    struct i2c_client *client;
    char part_number[FIELD_LEN_MAX + 1];
    char product_version[FIELD_LEN_MAX + 1];
    char serial_number[FIELD_LEN_MAX + 1];
#if VERBOSE
    char mfg_name[FIELD_LEN_MAX+1];
    char product_name[FIELD_LEN_MAX + 1];
    char extra[3][FIELD_LEN_MAX + 1];
#endif
};

u8 fru_calc_checksum(void *area, size_t len)
{
    u8 checksum = 0;
    u8 * data = area;
    size_t i;

    for (i = 0; i < len - 1; i++)
        checksum += data[i];

    return -checksum;
}

int fru_checksum_is_valid(void *area, size_t len)
{
    u8 * data = area;
    /* Checksum is valid when the stored checksum equals calculated */
    return data[len - 1] == fru_calc_checksum(area, len);
}

const char * buf2str_extended(const u8 *buf, int len, const char *sep)
{
    static char str[BUF2STR_MAXIMUM_OUTPUT_SIZE] = {0};
    char *cur;
    int i;
    int sz;
    int left;
    int sep_len;

    if (!buf) {
        snprintf(str, sizeof(str), "<NULL>");
        return (const char *)str;
    }
    cur = str;
    left = sizeof(str);
    if (sep) {
        sep_len = strlen(sep);
    } else {
        sep_len = 0;
    }
    for (i = 0; i < len; i++) {
        /* may return more than 2, depending on locale */
        sz = snprintf(cur, left, "%2.2x", buf[i]);
        if (sz >= left) {
            /* buffer overflow, truncate */
            break;
        }
        cur += sz;
        left -= sz;
        /* do not write separator after last byte */
        if (sep && i != (len - 1)) {
            if (sep_len >= left) {
                break;
            }
            strncpy(cur, sep, left - sz);
            cur += sep_len;
            left -= sep_len;
        }
    }
    *cur = '\0';

    return (const char *)str;
}

const char * buf2str(const u8 *buf, int len)
{
    return buf2str_extended(buf, len, NULL);
}

char * get_fru_area_str(struct device *dev, u8 * data, u32 * offset)
{
    static const char bcd_plus[] = "0123456789 -.:,_";
    char * str;
    int len, off, size, i, j, k, typecode, char_idx;
    union {
        u32 bits;
        char chars[4];
    } u;

    size = 0;
    off = *offset;

    /* bits 6:7 contain format */
    typecode = ((data[off] & 0xC0) >> 6);

    /* bits 0:5 contain length */
    len = data[off++];
    len &= 0x3f;

    switch (typecode) {
    case 0:           /* 00b: binary/unspecified */
    case 1:           /* 01b: BCD plus */
        /* hex dump or BCD -> 2x length */
        size = (len * 2);
        break;
    case 2:           /* 10b: 6-bit ASCII */
        /* 4 chars per group of 1-3 bytes, round up to 4 bytes boundary */
        size = (len / 3 + 1) * 4;
        break;
    case 3:           /* 11b: 8-bit ASCII */
        /* no length adjustment */
        size = len;
        break;
    }

    if (size < 1) {
        *offset = off;
        return NULL;
    }
    str = devm_kzalloc(dev, size+1, GFP_KERNEL);
    if (!str)
        return NULL;

    if (size == 0) {
        str[0] = '\0';
        *offset = off;
        return str;
    }

    switch (typecode) {
    case 0:        /* Binary */
        strncpy(str, buf2str(&data[off], len), size);
        break;

    case 1:        /* BCD plus */
        for (k = 0; k < size; k++)
            str[k] = bcd_plus[((data[off + k / 2] >> ((k % 2) ? 0 : 4)) & 0x0f)];
        str[k] = '\0';
        break;

    case 2:        /* 6-bit ASCII */
        for (i = j = 0; i < len; i += 3) {
            u.bits = 0;
            k = ((len - i) < 3 ? (len - i) : 3);

            memcpy((void *)&u.bits, &data[off+i], k);
            char_idx = 0;
            for (k=0; k<4; k++) {
                str[j++] = ((u.chars[char_idx] & 0x3f) + 0x20);
                u.bits >>= 6;
            }
        }
        str[j] = '\0';
        break;

    case 3:
        memcpy(str, &data[off], size);
        str[size] = '\0';
        break;
    }

    off += len;
    *offset = off;

    return str;
}

static int decode_fru_product_info_area(struct i2c_client *client, u8 * raw_data, u32 offset)
{
    char * fru_area;
    u8 * fru_data;
    u32 fru_len, i;
    u8 * tmp = raw_data + offset;
    struct device *dev = &client->dev;
    struct at24_data *at24 = i2c_get_clientdata(client);
    fru_len = 0;

    /* read enough to check length field */
    fru_len = 8 * tmp[1];

    if (fru_len == 0) {
        return -EINVAL;
    }
    fru_data = devm_kzalloc(dev, fru_len, GFP_KERNEL);

    if (!fru_data)
        return -ENOMEM;

    memcpy(fru_data, raw_data+offset, fru_len);

    struct fru_product_info_area_field {
        char name[64];
        char * p;
    };

    const struct fru_product_info_area_field fru_fields[] = {
        {"Product Area Format Version", NULL},
        {"Product Area Length", NULL},
        {"Language Code", NULL},
#if VERBOSE
        {"Manufacturer Name", at24->mfg_name},
        {"Product Name", at24->product_name},
#else
        {"Manufacturer Name", NULL},
        {"Product Name", NULL},
#endif
        {"Product Part/Model Number", at24->part_number},
        {"Product Version", at24->product_version},
        {"Product Serial Number", at24->serial_number},
        {"Asset Tag", NULL},
        {"FRU File ID", NULL},
#if VERBOSE
        {"Product Extra 1", at24->extra[0]},
        {"Product Extra 2", at24->extra[1]},
        {"Product Extra 3", at24->extra[2]}
#else
        {"Product Extra 1", NULL},
        {"Product Extra 2", NULL},
        {"Product Extra 3", NULL}
#endif
    };

    /* Check area checksum */
    if(debug && !fru_checksum_is_valid(fru_data, fru_len)) {
        dev_warn(dev, "Invalid eeprom checksum.\n");
        return -EINVAL;
    }

    i = 3;
    int j = 0;
    for(; j < ARRAY_SIZE(fru_fields); j++)
    {
        if(j < 3) {
            if(debug) {
                dev_info(dev, "%s: %x\n", fru_fields[j].name, fru_data[j]);
            }
            continue;
        }
        fru_area = get_fru_area_str(dev, fru_data, &i);
        if(fru_area &&  fru_fields[j].p) {
             if (strlen(fru_area) > 0) {
                if(debug) {
                    dev_info(dev, "%s: %s\n", fru_fields[j].name, fru_area);
                }
                int len = strlen(fru_area);
                if( len > FIELD_LEN_MAX) {
                    len = FIELD_LEN_MAX;
                }
                strncpy(fru_fields[j].p, fru_area, len);
            }
        }
    }
    return 0;
}

int decode_eeprom(struct i2c_client *client)
{
    int ret;

    u8 * raw_data = kzalloc(read_eeprom_max_len, GFP_KERNEL);
    if (!raw_data) {
        return -ENOMEM;
    }

    // i2c_smbus initiate eeprom chip's internel "CURRENT ADDRESS" for reading
    ret = i2c_smbus_write_word_data(client, 0, 0);
    msleep(1);
    for (int i = 0; i < read_eeprom_max_len; i++) {
        raw_data[i]= i2c_smbus_read_byte(client);
    }

    struct fru_header header;
    memset(&header, 0, sizeof(struct fru_header));

    if(debug) {
        print_hex_dump(KERN_INFO, "", DUMP_PREFIX_NONE, 16, 1, raw_data, read_eeprom_max_len, true);
    }

    /* According to IPMI Platform Management FRU Information Storage Definition v1.0 */
    memcpy(&header, raw_data, 8);
#if VERBOSE
    if (header.version != 1) {
        struct device *dev = &client->dev;
        dev_err(dev,  "Unknown FRU header version 0x%02x", header.version);
        kfree(raw_data);
        return -1;
    }
#endif
    /*
    * Only process Product Info Area
    */
    if ((header.offset.product*8) >= sizeof(struct fru_header))
        decode_fru_product_info_area(client, raw_data, header.offset.product*8);

    kfree(raw_data);
    return 0;
}

static ssize_t trigger_read_eeprom(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    if(!strncmp(buf,"1", count-1)) {
        struct at24_data *data = dev_get_drvdata(dev);
        decode_eeprom(data->client);
    }
    return count;
}

static ssize_t show_part_number(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->part_number);
}

static ssize_t show_serial_number(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->serial_number);
}

static ssize_t show_product_version(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->product_version);
}

#if VERBOSE
static ssize_t show_mfg_name(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->mfg_name);
}

static ssize_t show_product_name(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->product_name);
}

static ssize_t show_extra1(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->extra[0]);
}

static ssize_t show_extra2(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->extra[1]);
}

static ssize_t show_extra3(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->extra[2]);
}
#endif

// sysfs attributes
static SENSOR_DEVICE_ATTR(read_eeprom, S_IWUSR, NULL, trigger_read_eeprom, 0);
static SENSOR_DEVICE_ATTR(part_number, S_IRUGO, show_part_number, NULL, 0);
static SENSOR_DEVICE_ATTR(serial_number, S_IRUGO, show_serial_number, NULL, 0);
static SENSOR_DEVICE_ATTR(product_version, S_IRUGO, show_product_version, NULL, 0);

#if VERBOSE
static SENSOR_DEVICE_ATTR(mfg_name, S_IRUGO, show_mfg_name, NULL, 0);
static SENSOR_DEVICE_ATTR(product_name, S_IRUGO, show_product_name, NULL, 0);
static SENSOR_DEVICE_ATTR(extra1, S_IRUGO, show_extra1, NULL, 0);
static SENSOR_DEVICE_ATTR(extra2, S_IRUGO, show_extra2, NULL, 0);
static SENSOR_DEVICE_ATTR(extra3, S_IRUGO, show_extra3, NULL, 0);
#endif

static struct attribute *eeprom_attributes[] = {
    &sensor_dev_attr_read_eeprom.dev_attr.attr,
    &sensor_dev_attr_part_number.dev_attr.attr,
    &sensor_dev_attr_serial_number.dev_attr.attr,
    &sensor_dev_attr_product_version.dev_attr.attr,
#if VERBOSE
    &sensor_dev_attr_mfg_name.dev_attr.attr,
    &sensor_dev_attr_product_name.dev_attr.attr,
    &sensor_dev_attr_extra1.dev_attr.attr,
    &sensor_dev_attr_extra2.dev_attr.attr,
    &sensor_dev_attr_extra3.dev_attr.attr,
#endif
    NULL
};

static const struct attribute_group eeprom_group = {
    .attrs = eeprom_attributes,
};

static int eeprom_probe(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct at24_data *data;
    int status;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_BLOCK_DATA)) {
        dev_info(&client->dev, "i2c_check_functionality failed!\n");
        status = -EIO;
        return status;
    }

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        return status;
    }

    mutex_init(&data->lock);
    i2c_set_clientdata(client, data);

    dev_info(&client->dev, "eeprom chip found\n");
    /* Create sysfs entries */
    status = sysfs_create_group(&client->dev.kobj, &eeprom_group);
    if (status) {
        dev_err(dev, "Cannot create sysfs\n");
        kfree(data);
        return status;
    }
    data->client = client;
    decode_eeprom(client);
    return status;
}

static void eeprom_remove(struct i2c_client *client)
{
    struct at24_data *data = i2c_get_clientdata(client);
    sysfs_remove_group(&client->dev.kobj, &eeprom_group);
    kfree(data);
    return;
}

static const struct i2c_device_id eeprom_id[] = {
    { EEPROM_NAME, 0 },
    { EEPROM_NAME, 0 },
    {}
};

MODULE_DEVICE_TABLE(i2c, eeprom_id);

/* Address scanned */
static const unsigned short normal_i2c[] = { 0x50, 0x51, I2C_CLIENT_END };

/* This is the driver that will be inserted */
static struct i2c_driver eeprom_driver = {
        .class            = I2C_CLASS_HWMON,
        .driver = {
                    .name = EEPROM_NAME,
                  },
        .probe            = eeprom_probe,
        .remove           = eeprom_remove,
        .id_table         = eeprom_id,
        .address_list     = normal_i2c,
};

static int __init eeprom_init(void)
{
    return i2c_add_driver(&eeprom_driver);
}

static void __exit eeprom_exit(void)
{
    i2c_del_driver(&eeprom_driver);
}

MODULE_DESCRIPTION("NOKIA EEPROM FRU Sysfs driver");
MODULE_AUTHOR("Nokia");
MODULE_LICENSE("GPL");

module_init(eeprom_init);
module_exit(eeprom_exit);
