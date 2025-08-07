// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * eeprom_tlv.c - NOKIA EEPROM TLV Sysfs driver
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

#define EEPROM_NAME                 "eeprom_tlv"
#define FIELD_LEN_MAX 255
#define VERBOSE 0
static unsigned int debug = 0;
module_param_named(debug, debug, uint, 0);
MODULE_PARM_DESC(debug, "Debug enable(default to 0)");

static unsigned int read_eeprom_max_len = 96;
module_param_named(read_eeprom_max_len, read_eeprom_max_len, uint, 0);
MODULE_PARM_DESC(read_eeprom_max_len, "read_eeprom_max_len(default to 96)");

/**
 *  The TLV Types.
 */
#define ONIE_TLV_CODE_PRODUCT_NAME 0x21
#define ONIE_TLV_CODE_PART_NUMBER 0x22
#define ONIE_TLV_CODE_SERIAL_NUMBER 0x23
#define ONIE_TLV_CODE_MAC_BASE 0x24
#define ONIE_TLV_CODE_MANUF_DATE 0x25
#define ONIE_TLV_CODE_DEVICE_VERSION 0x26
#define ONIE_TLV_CODE_LABEL_REVISION 0x27
#define ONIE_TLV_CODE_PLATFORM_NAME 0x28
#define ONIE_TLV_CODE_ONIE_VERSION 0x29
#define ONIE_TLV_CODE_MAC_SIZE 0x2A
#define ONIE_TLV_CODE_MANUF_NAME 0x2B
#define ONIE_TLV_CODE_MANUF_COUNTRY 0x2C
#define ONIE_TLV_CODE_VENDOR_NAME 0x2D
#define ONIE_TLV_CODE_DIAG_VERSION 0x2E
#define ONIE_TLV_CODE_SERVICE_TAG 0x2F
#define ONIE_TLV_CODE_UNDEFINED 0xFC
#define ONIE_TLV_CODE_VENDOR_EXT 0xFD
#define ONIE_TLV_CODE_CRC_32 0xFE
#define ONIE_TLV_TYPE_INVALID 0xFF

#define ONIE_TLV_INFO_ID_STRING "TlvInfo"
#define ONIE_TLV_INFO_VERSION 0x01
#define ONIE_TLV_INFO_MAX_LEN 2048
#define ONIE_TLV_TOTAL_LEN_MAX (ONIE_TLV_INFO_MAX_LEN - sizeof(onie_tlvinfo_header_t))

#define MAC_LEN 6
#define DATE_LEN 19
#define VER_LEN 1
#define COUNTRY_CODE_LEN 2


/**
 * ONIE TLV EEPROM Header
 */
typedef struct __attribute__((__packed__)) onie_tlvinfo_header_s
{
    char signature[8]; /* 0x00 - 0x07 EEPROM Tag "TlvInfo" */
    u8 version;   /* 0x08        Structure version    */
    u16 totallen; /* 0x09 - 0x0A Length of all data which follows */
} onie_tlvinfo_header_t;

/**
 * ONIE TLV Entry
 */
typedef struct __attribute__((__packed__)) onie_tlvinfo_tlv_s
{
    u8 type;
    u8 length;
    u8 value[0];
} onie_tlvinfo_tlv_t;

struct at24_data {
    /*
     * Lock protects against activities from other Linux tasks,
     * but not from changes by other I2C masters.
     */
    struct mutex lock;
    struct i2c_client *client;
    char part_number[FIELD_LEN_MAX + 1];
    char serial_number[FIELD_LEN_MAX + 1];
#if VERBOSE
    char product_name[FIELD_LEN_MAX + 1];
    char base_mac[MAC_LEN + 1];
    char mfg_date[DATE_LEN + 1];
    char device_version[VER_LEN + 1];
    char label_version[FIELD_LEN_MAX + 1];
    char platform_name[FIELD_LEN_MAX + 1];
    char onie_version[FIELD_LEN_MAX + 1];
    u16 mac_size;
    char mfg_name[FIELD_LEN_MAX + 1];
    char mfg_country[COUNTRY_CODE_LEN + 1];
    char vendor_name[FIELD_LEN_MAX + 1];
    char diag_version[FIELD_LEN_MAX + 1];
    char service_tag[FIELD_LEN_MAX + 1];
    char vendor_ext[FIELD_LEN_MAX + 1];
    u32 crc;
#endif
};

inline char * onie_tag_to_field_name(u8 tag)
{
    switch (tag)
    {
        case ONIE_TLV_CODE_PART_NUMBER:
            return "part_number";
        case ONIE_TLV_CODE_SERIAL_NUMBER:
            return "serial_number";
#if VERBOSE
        case ONIE_TLV_CODE_PRODUCT_NAME:
            return "product_name";
        case ONIE_TLV_CODE_MAC_BASE:
            return "base_mac";
        case ONIE_TLV_CODE_MANUF_DATE:
            return "mfg_date";
        case ONIE_TLV_CODE_DEVICE_VERSION:
            return "device_version";
        case ONIE_TLV_CODE_LABEL_REVISION:
            return "label_version";
        case ONIE_TLV_CODE_PLATFORM_NAME:
            return "platform_name";
        case ONIE_TLV_CODE_ONIE_VERSION:
            return "onie_version";
        case ONIE_TLV_CODE_MAC_SIZE:
            return "mac_size";
        case ONIE_TLV_CODE_MANUF_NAME:
            return "mfg_name";
        case ONIE_TLV_CODE_MANUF_COUNTRY:
            return "mfg_country";
        case ONIE_TLV_CODE_VENDOR_NAME:
            return "vendor_name";
        case ONIE_TLV_CODE_DIAG_VERSION:
            return "diag_version";
        case ONIE_TLV_CODE_SERVICE_TAG:
            return "service_tag";
        case ONIE_TLV_CODE_VENDOR_EXT:
            return "vendor_ext";
        case ONIE_TLV_CODE_CRC_32:
            return "crc";
#endif
        default:
            return "unknown";
    }
}

static u32 TlvBigEndianToInteger(const u8 *buff, u8 len)
{
    u32 value = 0;
    for(int i = 0; i < len; ++i)
    {
        value = value << 8;
        value |= (buff[i] & 0xFF);
    }
    return value;
}

int tlv_decode(struct i2c_client *client, const u8 *buffer, const u32 length)
{
    struct device *dev = &client->dev;
    struct at24_data *at24 = i2c_get_clientdata(client);
    u32 offset = 0, L = 0;
    if (!buffer || length <= 0)
    {
        return -1;
    }

    for (; offset < length;)
    {
        u8 T = *(u8 *)(buffer + offset);
        offset += 1;

        if (T == ONIE_TLV_TYPE_INVALID)
        {
            continue;
        }

        L = TlvBigEndianToInteger(buffer + offset, 1);
        offset += 1;

        char sbuf[FIELD_LEN_MAX + 1] ={0};
        if(L > FIELD_LEN_MAX) {
            L = FIELD_LEN_MAX;
        }

        memcpy(sbuf, buffer + offset, L);
        if(debug) {
            switch(T)
            {
                case ONIE_TLV_CODE_MAC_BASE:
                    dev_info(dev, "Tag 0x%x [%s] [%x:%x]: %pM", T, onie_tag_to_field_name(T), offset, L, sbuf);
                    break;
                case ONIE_TLV_CODE_MAC_SIZE:
                    dev_info(dev, "Tag 0x%x [%s] [%x:%x]: 0x%02x", T, onie_tag_to_field_name(T), offset, L, *(u32 *)sbuf);
                    break;
                case ONIE_TLV_CODE_CRC_32:
                    dev_info(dev, "Tag 0x%x [%s] [%x:%x]: 0x%08x", T, onie_tag_to_field_name(T), offset, L, *(u32 *)sbuf);
                    break;
                default:
                    dev_info(dev, "Tag 0x%x [%s] [%x:%x]: %s", T, onie_tag_to_field_name(T), offset, L, sbuf);
                    break;
            }
        }

        int len = strlen(sbuf);
        if( len > FIELD_LEN_MAX) {
            len = FIELD_LEN_MAX;
        }

        switch(T)
        {
            case ONIE_TLV_CODE_PART_NUMBER:
                strncpy(at24->part_number, sbuf, len);
                break;
            case ONIE_TLV_CODE_SERIAL_NUMBER:
                strncpy(at24->serial_number, sbuf, len);
                break;
#if VERBOSE
            case ONIE_TLV_CODE_PRODUCT_NAME:
                strncpy(at24->product_name, sbuf, len);
                break;
            case ONIE_TLV_CODE_MAC_BASE:
                if( len > MAC_LEN) {
                    len = MAC_LEN;
                }
                strncpy(at24->base_mac, sbuf, len);
                break;
            case ONIE_TLV_CODE_MANUF_DATE:
                if( len > DATE_LEN) {
                    len = DATE_LEN;
                }
                strncpy(at24->mfg_date, sbuf, len);
                break;
            case ONIE_TLV_CODE_DEVICE_VERSION:
                if( len > VER_LEN) {
                    len = VER_LEN;
                }
                strncpy(at24->device_version, sbuf, len);
                break;
            case ONIE_TLV_CODE_LABEL_REVISION:
                strncpy(at24->label_version, sbuf, len);
                break;
            case ONIE_TLV_CODE_PLATFORM_NAME:
                strncpy(at24->platform_name, sbuf, len);
                break;
            case ONIE_TLV_CODE_ONIE_VERSION:
                strncpy(at24->onie_version, sbuf, len);
                break;
            case ONIE_TLV_CODE_MAC_SIZE:
                at24->mac_size = *(u16*)sbuf;
                break;
            case ONIE_TLV_CODE_MANUF_NAME:
                strncpy(at24->mfg_name, sbuf, len);
                break;
            case ONIE_TLV_CODE_MANUF_COUNTRY:
                if( len > COUNTRY_CODE_LEN) {
                    len = COUNTRY_CODE_LEN;
                }
                strncpy(at24->mfg_country, sbuf, len);
                break;
            case ONIE_TLV_CODE_VENDOR_NAME:
                strncpy(at24->vendor_name, sbuf, len);
                break;
            case ONIE_TLV_CODE_DIAG_VERSION:
                strncpy(at24->diag_version, sbuf, len);
                break;
            case ONIE_TLV_CODE_SERVICE_TAG:
                strncpy(at24->service_tag, sbuf, len);
                break;
            case ONIE_TLV_CODE_VENDOR_EXT:
                strncpy(at24->vendor_ext, sbuf, len);
                break;
            case ONIE_TLV_CODE_CRC_32:
                at24->crc = *(u32*)sbuf;
                break;
#endif
            default:
                break;
        }

        // Cannot parse if length field of TLV entry exceeds the total length of the buffer
        if (offset + L > length)
        {
            dev_err(dev, "Tag 0x%x with length %x exceeds total buffer length %x", T, L, length);
            return false;
        }
        if (L <= 0)
        {
            continue;
        }

        offset += L;
#if VERBOSE
        // CRC32 should always be the last TLV entry
        if (T == ONIE_TLV_CODE_CRC_32)
        {
            if(debug) {
                dev_info(dev,"Found CRC TLV entry at offset %u, done decoding", offset);
            }
            break;
        }
#endif
    }

    return true;
}

static inline __u16 bswap_16(__u16 x) {
    return (__u16)((x >> 8) | (x << 8));
}

int decode_onie_eeprom(struct i2c_client *client, u8 * raw_data)
{
    struct device *dev = &client->dev;
    onie_tlvinfo_header_t *eeprom_hdr = (onie_tlvinfo_header_t *)raw_data;
    onie_tlvinfo_tlv_t *eeprom_tlv = (onie_tlvinfo_tlv_t *)&raw_data[sizeof(onie_tlvinfo_header_t)];

    int len = bswap_16(eeprom_hdr->totallen);
    if(debug) {
        dev_info(dev, "len:%d\n", len);
    }
    if (len <= ONIE_TLV_TOTAL_LEN_MAX)
    {
        if (!tlv_decode(client, (u8 *)eeprom_tlv, len))
        {
            dev_err(dev, "Failed to decode onie eeprom");
            return -1;
        }
        return 0;
    }

    dev_err(dev, "Onie eeprom header is not valid");
    return 0;
}

int decode_eeprom(struct i2c_client *client)
{
    u8 * raw_data = kzalloc(read_eeprom_max_len, GFP_KERNEL);
    if (!raw_data) {
        return -ENOMEM;
    }

    for (int i = 0; i < read_eeprom_max_len; i++) {
        // i2c_smbus_read_byte_data initiate eeprom chip's internel "CURRENT ADDRESS" for reading
        if(i == 0)
            raw_data[i] = i2c_smbus_read_byte_data(client,0);
        else
            raw_data[i]= i2c_smbus_read_byte(client);
    }

    if(debug) {
        print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16, 1, raw_data, read_eeprom_max_len, true);
    }

    if(!strncmp(raw_data,"TlvInfo", 7)) {
        decode_onie_eeprom(client, raw_data);
    }

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

#if VERBOSE
static ssize_t show_product_name(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->product_name);
}

static ssize_t show_base_mac(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%pM\n", data->base_mac);
}

static ssize_t show_mfg_date(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->mfg_date);
}

static ssize_t show_device_version(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->device_version);
}

static ssize_t show_label_version(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->label_version);
}

static ssize_t show_platform_name(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->platform_name);
}

static ssize_t show_onie_version(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->onie_version);
}

static ssize_t show_mac_size(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%02x\n", data->mac_size);
}

static ssize_t show_mfg_name(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->mfg_name);
}

static ssize_t show_mfg_country(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->mfg_country);
}

static ssize_t show_vendor_name(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->vendor_name);
}

static ssize_t show_diag_version(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->diag_version);
}

static ssize_t show_service_tag(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->service_tag);
}

static ssize_t show_vendor_ext(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "%s\n", data->vendor_ext);
}

static ssize_t show_crc(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct at24_data *data = dev_get_drvdata(dev);
    return sprintf(buf, "0x%08x\n", data->crc);
}

// sysfs attributes
static SENSOR_DEVICE_ATTR(product_name, S_IRUGO, show_product_name, NULL, 0);
static SENSOR_DEVICE_ATTR(base_mac, S_IRUGO, show_base_mac, NULL, 0);
static SENSOR_DEVICE_ATTR(mfg_date, S_IRUGO, show_mfg_date, NULL, 0);
static SENSOR_DEVICE_ATTR(device_version, S_IRUGO, show_device_version, NULL, 0);
static SENSOR_DEVICE_ATTR(label_version, S_IRUGO, show_label_version, NULL, 0);
static SENSOR_DEVICE_ATTR(platform_name, S_IRUGO, show_platform_name, NULL, 0);
static SENSOR_DEVICE_ATTR(onie_version, S_IRUGO, show_onie_version, NULL, 0);
static SENSOR_DEVICE_ATTR(mac_size, S_IRUGO, show_mac_size, NULL, 0);
static SENSOR_DEVICE_ATTR(mfg_name, S_IRUGO, show_mfg_name, NULL, 0);
static SENSOR_DEVICE_ATTR(mfg_country, S_IRUGO, show_mfg_country, NULL, 0);
static SENSOR_DEVICE_ATTR(vendor_name, S_IRUGO, show_vendor_name, NULL, 0);
static SENSOR_DEVICE_ATTR(diag_version, S_IRUGO, show_diag_version, NULL, 0);
static SENSOR_DEVICE_ATTR(service_tag, S_IRUGO, show_service_tag, NULL, 0);
static SENSOR_DEVICE_ATTR(vendor_ext, S_IRUGO, show_vendor_ext, NULL, 0);
static SENSOR_DEVICE_ATTR(crc, S_IRUGO, show_crc, NULL, 0);
#endif

static SENSOR_DEVICE_ATTR(read_eeprom, S_IWUSR, NULL, trigger_read_eeprom, 0);
static SENSOR_DEVICE_ATTR(part_number, S_IRUGO, show_part_number, NULL, 0);
static SENSOR_DEVICE_ATTR(serial_number, S_IRUGO, show_serial_number, NULL, 0);

static struct attribute *eeprom_attributes[] = {
    &sensor_dev_attr_read_eeprom.dev_attr.attr,
    &sensor_dev_attr_part_number.dev_attr.attr,
    &sensor_dev_attr_serial_number.dev_attr.attr,
#if VERBOSE
    &sensor_dev_attr_product_name.dev_attr.attr,
    &sensor_dev_attr_base_mac.dev_attr.attr,
    &sensor_dev_attr_mfg_date.dev_attr.attr,
    &sensor_dev_attr_device_version.dev_attr.attr,
    &sensor_dev_attr_label_version.dev_attr.attr,
    &sensor_dev_attr_platform_name.dev_attr.attr,
    &sensor_dev_attr_onie_version.dev_attr.attr,
    &sensor_dev_attr_mac_size.dev_attr.attr,
    &sensor_dev_attr_mfg_name.dev_attr.attr,
    &sensor_dev_attr_mfg_country.dev_attr.attr,
    &sensor_dev_attr_vendor_name.dev_attr.attr,
    &sensor_dev_attr_diag_version.dev_attr.attr,
    &sensor_dev_attr_service_tag.dev_attr.attr,
    &sensor_dev_attr_vendor_ext.dev_attr.attr,
    &sensor_dev_attr_crc.dev_attr.attr,
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

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_BLOCK_DATA))
    {
        dev_info(&client->dev, "i2c_check_functionality failed!\n");
        status = -EIO;
        return status;
    }

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data)
    {
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
static const unsigned short normal_i2c[] = { 0x50, I2C_CLIENT_END };

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

MODULE_DESCRIPTION("NOKIA EEPROM TLV Sysfs driver");
MODULE_AUTHOR("Nokia");
MODULE_LICENSE("GPL");

module_init(eeprom_init);
module_exit(eeprom_exit);
