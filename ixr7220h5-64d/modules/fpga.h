/*
 * Copyright (C) 2014 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/acpi.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

#define FPGA_I2C_BUSNUM 5
#define FPGA_I2C_MUX_DIS 0
#define FPGA_I2C_MUX_EN 1

#define FPGA_JTAG_MUX_REG 0x100
#define FPGA_JTAG_CTRL0_REG 0x104
#define FPGA_JTAG_CTRL1_REG 0x108
#define FPGA_JTAG_CTRL2_REG 0x10C

typedef struct fpga_i2c
{
	int bus;
	int mux_en;
	int mux_addr;
	int num_ch;
} fpga_i2c_s;

typedef struct fpga_gpio
{
	int num;
	char name[50];
	int reg;
	int bit;
} fpga_gpio_s;

struct i2c_bus_dev
{
	struct i2c_adapter adapter;
	int busnum;
	int mux_ch;
	int mux_en;
	void *__iomem bar;
};

struct fpga_gpio_chip
{
	struct gpio_chip gpio_chip;
	struct mutex lock;
	void *__iomem bar;
	u32 registers;
	/*
	 * Since the registers are chained, every byte sent will make
	 * the previous byte shift to the next register in the
	 * chain. Thus, the first byte sent will end up in the last
	 * register at the end of the transfer. So, to have a logical
	 * numbering, store the bytes in reverse order.
	 */
	u8 buffer[0];
};

struct fpga_dev
{
	char name[64];
	struct pci_dev *dev;
	struct i2c_bus_dev *i2c;
	struct fpga_gpio_chip *gpio;
	int pci_base;
	int pci_size;
	struct semaphore sem;
	char *buffer;
	/* character device */
	dev_t cdev_num;
	struct cdev cdev;
	struct class *my_class;
	struct device *device;
};

static const size_t BUF_SIZE = PAGE_SIZE;
static struct pci_driver dni_fpga_driver;
static int delta_wait_i2c_complete(struct i2c_bus_dev *i2c, int ch);
static void delta_fpga_i2c_data_reg_set(struct i2c_bus_dev *i2c, int ch, int addr, int data);
static void delta_fpga_i2c_addr_reg_set(struct i2c_bus_dev *i2c, int ch, int data);
static void delta_fpga_i2c_ctrl_set(struct i2c_bus_dev *i2c, int ch, int data);
static int dni_fpga_i2c_read(struct i2c_bus_dev *i2c, int ch, int addr, int raddr, int rsize, uint8_t *readout, int dsize);
static int dni_fpga_i2c_write(struct i2c_bus_dev *i2c, int ch, int addr, int raddr, int rsize, uint8_t *data, int dsize);
static int io_read(struct i2c_bus_dev *i2c, int offset);
static void io_write(struct i2c_bus_dev *i2c, int offset, int data);

#define DELTA_I2C_WAIT_BUS_TIMEOUT 100000 /* 100000us = 100ms */
#define DELTA_I2C_OFFSET 0x1000
#define DELTA_I2C_CONF(s) ((DELTA_I2C_OFFSET) + ((0x300) * (s)))
#define DELTA_I2C_ADDR(s) ((DELTA_I2C_OFFSET) + ((0x300) * (s)) + 0x8)
#define DELTA_I2C_CTRL(s) ((DELTA_I2C_OFFSET) + ((0x300) * (s)) + 0x4)
#define DELTA_I2C_DATA(s) ((DELTA_I2C_OFFSET) + ((0x300) * (s)) + 0x100)

#define DELTA_I2C_GRABBER_OFFSET 0x1000
#define DELTA_I2C_GRABBER_CONF(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)))
#define DELTA_I2C_GRABBER_ADDR(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)) + 0x8)
#define DELTA_I2C_GRABBER_CTRL(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)) + 0x4)
#define DELTA_I2C_GRABBER_DATA(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)) + 0x100)

#define I2C_BUS_READY 0x4
#define I2C_TRANS_FAIL 0x2
#define I2C_TRANS_ENABLE 0x1
#define DELTA_FPGA_I2C_START_OFFSET 0
#define DELTA_FPGA_I2C_RW_OFFSET 3
#define DELTA_FPGA_I2C_REG_LEN_OFFSET 8
#define DELTA_FPGA_I2C_CH_SEL_OFFSET 10
#define DELTA_FPGA_I2C_CH_EN_OFFSET 13
#define DELTA_FPGA_I2C_DATA_LEN_OFFSET 15
#define DELTA_FPGA_I2C_SLAVE_OFFSET 25
