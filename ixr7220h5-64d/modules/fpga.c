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
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include "fpga.h"
#include "fpga_fileio.h"

static int init_gpiodev(struct pci_dev *dev, struct fpga_dev *fpga);

//#define FPGA_PCA9548
#define FPGA_GPIO

fpga_gpio_s fpga_gpio_info[] = {
	{0, "ENABLE JTAG_0", FPGA_JTAG_MUX_REG, 8},
	{1, "ENABLE JTAG_1", FPGA_JTAG_MUX_REG, 9},
	{2, "ENABLE JTAG_2", FPGA_JTAG_MUX_REG, 10},
	{3, "JTAG_0 FPGA_CPU_JTAG_TDI", FPGA_JTAG_CTRL0_REG, 3},
	{4, "JTAG_0 FPGA_CPU_JTAG_TDO", FPGA_JTAG_CTRL0_REG, 2},
	{5, "JTAG_0 FPGA_CPU_JTAG_TMS", FPGA_JTAG_CTRL0_REG, 1},
	{6, "JTAG_0 FPGA_CPU_JTAG_TCK", FPGA_JTAG_CTRL0_REG, 0},
	{7, "JTAG_1 FPGA_MB_JTAG_TDI", FPGA_JTAG_CTRL1_REG, 3},
	{8, "JTAG_1 FPGA_MB_JTAG_TDO", FPGA_JTAG_CTRL1_REG, 2},
	{9, "JTAG_1 FPGA_MB_JTAG_TMS", FPGA_JTAG_CTRL1_REG, 1},
	{10, "JTAG_1 FPGA_MB_JTAG_TCK", FPGA_JTAG_CTRL1_REG, 0},
	{11, "JTAG_2 FPGA_MB_JTAG_TRST_N", FPGA_JTAG_CTRL2_REG, 4},
	{12, "JTAG_2 FPGA_MB_JTAG_TDI", FPGA_JTAG_CTRL2_REG, 3},
	{13, "JTAG_2 FPGA_MB_JTAG_TDO", FPGA_JTAG_CTRL2_REG, 2},
	{14, "JTAG_2 FPGA_MB_JTAG_TMS", FPGA_JTAG_CTRL2_REG, 1},
	{15, "JTAG_2 FPGA_MB_JTAG_TCK", FPGA_JTAG_CTRL2_REG, 0},
};

fpga_i2c_s fpga_i2c_info[] = {
	// ALTERA FPGA SMBus-0 - SMBus-5 // 
	{0, FPGA_I2C_MUX_DIS, 0x00, 0},
	{1, FPGA_I2C_MUX_DIS, 0x00, 0},
	{2, FPGA_I2C_MUX_DIS, 0x00, 0},
	{3, FPGA_I2C_MUX_DIS, 0x00, 0},
	{4, FPGA_I2C_MUX_DIS, 0x00, 0},
	{5, FPGA_I2C_MUX_DIS, 0x00, 0},
	// Lattice FPGA SMBus-0 - SMBus-9 //
	{6, FPGA_I2C_MUX_DIS, 0x00, 0},
	{7, FPGA_I2C_MUX_DIS, 0x00, 0},
	{8, FPGA_I2C_MUX_DIS, 0x00, 0},
	{9, FPGA_I2C_MUX_DIS, 0x00, 0},
};

static s32 dni_fpga_i2c_access(struct i2c_adapter *adap, u16 addr,
							   unsigned short flags, char read_write, u8 command, int size,
							   union i2c_smbus_data *data)
{
	struct i2c_bus_dev *i2c = adap->algo_data;
	int len;
	uint8_t i2c_data;
	int rv = 0;

	//	printk(KERN_INFO "size=%d, read_write=%d addr=0x%x command=0x%x fpga_bus=%d \n",size,read_write,addr,command,i2c->busnum);
	switch (size)
	{

	case I2C_SMBUS_QUICK:
	//	printk(KERN_INFO "I2C_SMBUS_QUICK");
		rv = dni_fpga_i2c_write(i2c, i2c->busnum, addr, command, 0, &i2c_data, 0);
		return rv;
		break;
	case I2C_SMBUS_BYTE:
	//	printk(KERN_INFO "I2C_SMBUS_BYTE");
		if (read_write == I2C_SMBUS_WRITE)
		{
			rv = dni_fpga_i2c_write(i2c, i2c->busnum, addr, command, 1, &i2c_data, 0);
			return rv;
		}
		else
		{
			rv = dni_fpga_i2c_read(i2c, i2c->busnum, addr, command, 1, &data->byte, 1);
			return rv;
		}

		break;
	case I2C_SMBUS_BYTE_DATA:
		if (&data->byte == NULL)
			return -1;
		if (read_write == I2C_SMBUS_WRITE)
		{
			rv = dni_fpga_i2c_write(i2c, i2c->busnum, addr, command, 1, &data->byte, 1);
			return rv;
		}
		else
		{
			rv = dni_fpga_i2c_read(i2c, i2c->busnum, addr, command, 1, &data->byte, 1);
			return rv;
		}
		break;
	case I2C_SMBUS_WORD_DATA:
		if (&data->word == NULL)
			return -1;
		if (read_write == I2C_SMBUS_WRITE)
		{
			/* TODO: not verify */
			rv = dni_fpga_i2c_write(i2c, i2c->busnum, addr, command, 1, (uint8_t *)&data->word, 2);
			//			printk(KERN_INFO "I2C_FUNC_SMBUS_WORD_DATA write");
			return rv;
		}
		else
		{
			rv = dni_fpga_i2c_read(i2c, i2c->busnum, addr, command, 1, (uint8_t *)&data->word, 2);
			return rv;
		}
		break;

	case I2C_SMBUS_BLOCK_DATA:
		if (&data->block[1] == NULL)
			return -1;
		if (read_write == I2C_SMBUS_WRITE)
		{

			len = min_t(u8, data->block[0], I2C_SMBUS_BLOCK_MAX);
			rv = dni_fpga_i2c_write(i2c, i2c->busnum, addr, command, 1, &data->block[0], len + 1);
			return rv;
		}
		else
		{
			//			dni_fpga_i2c_read(i2c, i2c->busnum, addr, command, 1, &data->byte, 1);
			//			len = min_t(u8, data->byte, I2C_SMBUS_BLOCK_MAX);
			len = I2C_SMBUS_BLOCK_MAX;
			rv = dni_fpga_i2c_read(i2c, i2c->busnum, addr, command, 1, &data->block[0], len + 1);
			return rv;
		}
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		if (&data->block[1] == NULL)
			return -1;
		if (read_write == I2C_SMBUS_WRITE)
		{
			len = min_t(u8, data->block[0], I2C_SMBUS_BLOCK_MAX);
			rv = dni_fpga_i2c_write(i2c, i2c->busnum, addr, command, 1, &data->block[1], len);
			return rv;
		}
		else
		{
			len = min_t(u8, data->block[0], I2C_SMBUS_BLOCK_MAX);
			rv = dni_fpga_i2c_read(i2c, i2c->busnum, addr, command, 1, &data->block[1], len);
			return rv;
		}
		break;
	case I2C_SMBUS_PROC_CALL:

		break;
	case I2C_SMBUS_BLOCK_PROC_CALL:
		break;
	}

	return -1;
}

static int delta_wait_i2c_complete(struct i2c_bus_dev *i2c, int ch)
{
	unsigned long timeout = 0;
	uint64_t status;
	int offset = DELTA_I2C_CTRL(ch);

	while ((status = io_read(i2c, offset)) & I2C_TRANS_ENABLE)
	{
		if (timeout > DELTA_I2C_WAIT_BUS_TIMEOUT)
		{
			printk(KERN_INFO "i2c wait for complete timeout: time=%lu us status=0x%llx", timeout, status);
			return -ETIMEDOUT;
		}
		udelay(1000);
		timeout += 1000;
	}

	return 0;
}

static void delta_fpga_i2c_data_reg_set(struct i2c_bus_dev *i2c, int ch, int addr, int data)
{

	uint64_t hi_cmd, lo_cmd, i2c_cmd;
	int offset;
	hi_cmd = 0;
	lo_cmd = data;
	i2c_cmd = (hi_cmd << 32) | lo_cmd;
	offset = DELTA_I2C_DATA(ch) + addr;
	io_write(i2c, offset, i2c_cmd);
}

static void delta_fpga_i2c_addr_reg_set(struct i2c_bus_dev *i2c, int ch, int data)
{
	uint64_t hi_cmd, lo_cmd, i2c_cmd;
	int offset;
	hi_cmd = 0;
	lo_cmd = data;
	i2c_cmd = (hi_cmd << 32) | lo_cmd;
	offset = DELTA_I2C_ADDR(ch);
	io_write(i2c, offset, i2c_cmd);
}

static void delta_fpga_i2c_conf_reg_set(struct i2c_bus_dev *i2c, int ch, int data)
{
	uint64_t hi_cmd, lo_cmd, i2c_cmd;
	int offset;
	if (ch == 2)
	{
		hi_cmd = 0;
		lo_cmd = (data << 25) | 0x5A; //100Khz
		i2c_cmd = (hi_cmd << 32) | lo_cmd;
		offset = DELTA_I2C_CONF(ch);
		io_write(i2c, offset, i2c_cmd);
	}
}

static void delta_fpga_i2c_ctrl_set(struct i2c_bus_dev *i2c, int ch, int data)
{
	uint64_t hi_cmd, lo_cmd, i2c_cmd;
	int offset;
	hi_cmd = 0;
	lo_cmd = data;
	i2c_cmd = (hi_cmd << 32) | lo_cmd;
	offset = DELTA_I2C_CTRL(ch);
	io_write(i2c, offset, i2c_cmd);
}

static int dni_fpga_i2c_write(struct i2c_bus_dev *i2c, int ch, int addr, int raddr, int rsize, uint8_t *data, int dsize)
{
	int status;
	uint32_t ctrl_data, addr_data, rw_data;

	int rv = -1;
	int offset, i;
	int four_byte, one_byte;

	four_byte = dsize / 4;
	one_byte = dsize % 4;

	if (i2c->mux_en == FPGA_I2C_MUX_EN)
	{
		if (addr < 0x50 || addr > 0x58)
			goto fail;
	}

	for (i = 0; i < four_byte; i++)
	{
		offset = i * 4;
		rw_data = (data[i * 4 + 3] << 24) | (data[i * 4 + 2] << 16) | (data[i * 4 + 1] << 8) | data[i * 4 + 0];
		delta_fpga_i2c_data_reg_set(i2c, ch, offset, rw_data);
	}
	//	printk(KERN_INFO "four_byte : rw_data = 0x%lx",rw_data);
	rw_data = 0;
	for (i = 0; i < one_byte; i++)
	{
		rw_data |= (data[four_byte * 4 + i] << (i * 8));
	}
	offset = four_byte * 4;
	delta_fpga_i2c_data_reg_set(i2c, ch, offset, rw_data);

	//	printk(KERN_INFO "one_byte : rw_data = 0x%lx",rw_data);

	/* Set address register */
	if (rsize == 0)
		addr_data = 0;
	else if (rsize == 1)
	{
		addr = addr + (raddr / 0x100);
		addr_data = (raddr & 0xff);
	}
	else if (rsize == 2)
		addr_data = (raddr & 0xffff);
	delta_fpga_i2c_addr_reg_set(i2c, ch, addr_data);
#ifdef FPGA_PCA9548
	delta_fpga_i2c_conf_reg_set(i2c, ch, 0x70);
#endif
	ctrl_data = 0;
	/* Set ctrl reg */
	ctrl_data |= ((addr & 0x7f) << DELTA_FPGA_I2C_SLAVE_OFFSET);
	ctrl_data |= ((rsize & 0x3) << DELTA_FPGA_I2C_REG_LEN_OFFSET);
	ctrl_data |= ((dsize & 0x1ff) << DELTA_FPGA_I2C_DATA_LEN_OFFSET);
	ctrl_data |= 1 << DELTA_FPGA_I2C_RW_OFFSET;
	ctrl_data |= 1 << DELTA_FPGA_I2C_START_OFFSET;
#ifdef FPGA_PCA9548
	if (i2c->mux_en == FPGA_I2C_MUX_EN)
	{
		ctrl_data |= ((i2c->mux_ch & 0x7) << DELTA_FPGA_I2C_CH_SEL_OFFSET);
		ctrl_data |= ((1 & 0x1) << DELTA_FPGA_I2C_CH_EN_OFFSET);
	}
#endif
	/* check rsize */
	if (rsize > 2)
	{
		return -1;
	}

	delta_fpga_i2c_ctrl_set(i2c, ch, ctrl_data);
	/* wait for i2c transaction completion */
	if (delta_wait_i2c_complete(i2c, ch))
	{
		printk(KERN_INFO "i2c transaction completion timeout");
		rv = -EBUSY;
		return rv;
	}
	/* check status */
	//DNI_LOG_INFO("check i2c transaction status ...");
	status = io_read(i2c, DELTA_I2C_CTRL(ch));
	//	udelay(500);
	if ((status & I2C_TRANS_FAIL))
	{
		//		printk(KERN_INFO "I2C read failed: status=0x%lx", status);
		rv = -EIO; /* i2c transfer error */
		goto fail;
	}
	return 0;
fail:

	return rv;
}

static int dni_fpga_i2c_read(struct i2c_bus_dev *i2c, int ch, int addr, int raddr, int rsize, uint8_t *readout, int dsize)
{
	int status;
	uint32_t ctrl_data, addr_data, rw_data;
	int offset;
	int rv = -1;
	int i;

	if (i2c->mux_en == FPGA_I2C_MUX_EN)
	{
		if (addr < 0x50 || addr > 0x58)
			goto fail;
	}

	/* Set address register */
	if (rsize == 0)
		addr_data = 0;
	else if (rsize == 1)
	{
		addr = addr + (raddr / 0x100);
		addr_data = (raddr & 0xff);
	}
	else if (rsize == 2)
		addr_data = (raddr & 0xffff);
	delta_fpga_i2c_addr_reg_set(i2c, ch, addr_data);
#ifdef FPGA_PCA9548
	delta_fpga_i2c_conf_reg_set(i2c, ch, 0x70);
#endif
	ctrl_data = 0;
	/* Set ctrl reg */
	ctrl_data |= ((addr & 0x7f) << DELTA_FPGA_I2C_SLAVE_OFFSET);
	ctrl_data |= ((rsize & 0x3) << DELTA_FPGA_I2C_REG_LEN_OFFSET);
	ctrl_data |= ((dsize & 0x1ff) << DELTA_FPGA_I2C_DATA_LEN_OFFSET);
	ctrl_data |= 0 << DELTA_FPGA_I2C_RW_OFFSET;
	ctrl_data |= 1 << DELTA_FPGA_I2C_START_OFFSET;
#ifdef FPGA_PCA9548
	if (i2c->mux_en == FPGA_I2C_MUX_EN)
	{
		ctrl_data |= ((i2c->mux_ch & 0x7) << DELTA_FPGA_I2C_CH_SEL_OFFSET);
		ctrl_data |= ((1 & 0x1) << DELTA_FPGA_I2C_CH_EN_OFFSET);
	}
#endif
	/* check rsize */
	if (rsize > 2)
	{
		goto fail;
	}

	delta_fpga_i2c_ctrl_set(i2c, ch, ctrl_data);
	/* wait for i2c transaction completion */
	if (delta_wait_i2c_complete(i2c, ch))
	{
		printk(KERN_ERR "i2c transaction completion timeout");
		rv = -EBUSY;
		goto fail;
	}
	/* check status */
	//DNI_LOG_INFO("check i2c transaction status ...\n");
	udelay(100);
	status = io_read(i2c, DELTA_I2C_CTRL(ch));
	if ((status & I2C_TRANS_FAIL))
	{
		//printk(KERN_ERR "I2C read failed:addr_data=0x%lx,  ctrl_data=0x%lx, status=0x%lx ",addr_data, ctrl_data ,status);
		rv = -EIO; /* i2c transfer error */
		goto fail;
	}

	for (i = 1; i <= dsize; i++)
	{
		if ((i - 1) % 4 == 0)
		{
			offset = DELTA_I2C_DATA(ch) + (i - 1);
			rw_data = io_read(i2c, offset);
		}
		readout[i - 1] = (uint8_t)(rw_data >> (((i - 1) % 4) * 8));
	}
	return 0;
fail:
	return -1;
}

static int io_read(struct i2c_bus_dev *i2c, int offset)
{
	return ioread32(i2c->bar + offset);
}
static void io_write(struct i2c_bus_dev *i2c, int offset, int data)
{
	iowrite32(data, i2c->bar + offset);
}

static u32 dni_fpga_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE |
		   I2C_FUNC_SMBUS_BYTE_DATA |
		   I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_BLOCK_DATA |
		   I2C_FUNC_SMBUS_PROC_CALL | I2C_FUNC_SMBUS_BLOCK_PROC_CALL |
		   I2C_FUNC_SMBUS_I2C_BLOCK | I2C_FUNC_SMBUS_PEC;
}

static const struct i2c_algorithm smbus_algorithm = {
	.smbus_xfer = dni_fpga_i2c_access,
	.functionality = dni_fpga_i2c_func,
};

struct file_operations fpga_fileops = {
	.owner = THIS_MODULE,
	.read = fpga_read,
	.write = fpga_write,
	.mmap = fpga_mmap,
	/*.ioctl = fpga_ioctl,*/
	.open = fpga_open,
	.release = fpga_close,
};

/* Allocate /dev/ device */
static int init_chrdev(struct fpga_dev *fdev)
{
	int rc;
	int dev_minor = 0;
	int dev_major = 0;
	int devno = -1;

	/* request major number for device */
	snprintf(fdev->name, sizeof(fdev->name), "sysfpga");
	sema_init(&fdev->sem, 1);
	rc = alloc_chrdev_region(&fdev->cdev_num, dev_minor, 1 /* one device*/, fdev->name);
	if (rc)
	{
		printk(KERN_WARNING "can't get major ID");
		goto fail_alloc;
	}

	dev_major = MAJOR(fdev->cdev_num);

	devno = MKDEV(dev_major, dev_minor);
	cdev_init(&fdev->cdev, &fpga_fileops);
	fdev->cdev.owner = THIS_MODULE;
	fdev->cdev.ops = &fpga_fileops;
	rc = cdev_add(&fdev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (rc)
	{
		dev_err(&fdev->dev->dev, "Error %d adding char dev (%d, %d)", rc, dev_major, dev_minor);
		goto fail_cdev_add;
	}

	/* create /sys/class/sysfpga/ */
	fdev->my_class = class_create(THIS_MODULE, fdev->name);
	if (IS_ERR(fdev->my_class))
	{
		rc = PTR_ERR(fdev->my_class);
		printk(KERN_NOTICE "Can't create class\n");
		goto fail_create_class;
	}
	/* create /sys/class/sysfpga/sysfpga */
	fdev->device = device_create(fdev->my_class, NULL, fdev->cdev_num, NULL, fdev->name);
	if (IS_ERR(fdev->device))
	{
		rc = PTR_ERR(fdev->device);
		printk(KERN_NOTICE "Can't create device\n");
		goto fail_dev_create;
	}

	return 0;

fail_dev_create:
	class_destroy(fdev->my_class);
fail_create_class:
	cdev_del(&fdev->cdev);
fail_cdev_add:
	unregister_chrdev_region(devno, 1);
fail_alloc:
	return rc;
}

static int fpga_gpio_get(struct gpio_chip *gc, unsigned gpio)
{
	struct fpga_gpio_chip *chip = gpiochip_get_data(gc);
	int rdata;
	int ret;
	int reg = fpga_gpio_info[gpio].reg;
	int bit = fpga_gpio_info[gpio].bit;

	mutex_lock(&chip->lock);
	//ret = (chip->buffer[bank] >> pin) & 0x1;
	rdata = ioread32(chip->bar + reg);
	ret = (rdata >> bit) & 0x1;
	//printk(KERN_INFO "gpio = 0x%x chip->bar = 0x%x value = 0x%x", gpio, chip->bar, rdata);

	mutex_unlock(&chip->lock);

	return ret;
}

static void fpga_gpio_set(struct gpio_chip *gc,
						  unsigned gpio, int val)
{
	struct fpga_gpio_chip *chip = gpiochip_get_data(gc);
	int io_offset = 0x218;
	int rdata, wdata;
	int reg = fpga_gpio_info[gpio].reg;
	int bit = fpga_gpio_info[gpio].bit;

	mutex_lock(&chip->lock);

	rdata = ioread32(chip->bar + reg);

	if (val)
		wdata = rdata | (1 << bit);
	else
		wdata = rdata & ~(1 << bit);

	iowrite32(wdata, chip->bar + reg);

	mutex_unlock(&chip->lock);
}

static int init_gpiodev(struct pci_dev *dev, struct fpga_dev *fpga)
{
	char name[20] = "fpga-gpio chip";
	int err;
	fpga->gpio = kzalloc(sizeof(struct fpga_gpio_chip), GFP_KERNEL);
	if (!fpga->gpio)
		return -ENOMEM;

	fpga->gpio->bar = ioremap(pci_resource_start(dev, 0), pci_resource_len(dev, 0));
	fpga->gpio->gpio_chip.base = -1;
	fpga->gpio->gpio_chip.label = name;
	fpga->gpio->gpio_chip.owner = THIS_MODULE;
	fpga->gpio->gpio_chip.ngpio = 32;
	fpga->gpio->gpio_chip.parent = &dev->dev; //(struct devices) fpga

	fpga->gpio->gpio_chip.get = fpga_gpio_get;
	fpga->gpio->gpio_chip.set = fpga_gpio_set;
	err = gpiochip_add_data(&fpga->gpio->gpio_chip, fpga->gpio);
	if (err)
	{
		printk(KERN_INFO "GPIO registering failed ");
		return err;
	}
	return 0;
	//	mutex_init(&chip->lock);
}

int num_i2c_adapter;
static int init_i2c_adapter(struct pci_dev *dev, struct fpga_dev *fpga)
{
	int pci_base, pci_size;
	int i, j, error, bus = 0;
	int num_i2c_master;

	num_i2c_master = sizeof(fpga_i2c_info) / sizeof(fpga_i2c_s);
	for (i = 0; i < num_i2c_master; i++)
	{
		num_i2c_adapter++;
		if (fpga_i2c_info[i].mux_en == FPGA_I2C_MUX_EN)
			num_i2c_adapter += fpga_i2c_info[i].num_ch;
	}
	pci_base = pci_resource_start(dev, 0);
	pci_size = pci_resource_len(dev, 0);

	fpga->i2c = kzalloc(sizeof(struct i2c_bus_dev) * num_i2c_adapter, GFP_KERNEL);

	if (!fpga)
		return -ENOMEM;

	pci_set_drvdata(dev, fpga);
	printk(KERN_INFO "fpga = 0x%x, pci_size = 0x%x \n", fpga, pci_size);
	/* Create PCIE device */
	for (i = 0; i < num_i2c_master; i++)
	{
		fpga->dev = dev;
		fpga->pci_base = pci_base;
		fpga->pci_size = pci_size;

		(fpga->i2c + bus)->bar = ioremap(pci_resource_start(dev, 0), pci_resource_len(dev, 0));

		printk(KERN_INFO "BAR0 Register[0x%x] = 0x%x\n",
			   (fpga->i2c + bus)->bar, ioread32((fpga->i2c + bus)->bar));
		/* Create I2C adapter */
		printk(KERN_INFO "dev-%d, pci_base = 0x%x", i, fpga->pci_base + DELTA_I2C_ADDR(i));
		(fpga->i2c + bus)->adapter.owner = THIS_MODULE;
		snprintf((fpga->i2c + bus)->adapter.name, sizeof((fpga->i2c + bus)->adapter.name),
				 "DNI FPGA SMBus-%d ", i);
		(fpga->i2c + bus)->adapter.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
		(fpga->i2c + bus)->adapter.algo = &smbus_algorithm;
		(fpga->i2c + bus)->adapter.algo_data = fpga->i2c + bus;
		/* set up the sysfs linkage to our parent device */
		(fpga->i2c + bus)->adapter.dev.parent = &dev->dev;
		/* set i2c adapter number */
		//		(fpga->i2c+bus)->adapter.nr = i+10;
		/* set up the busnum */
		(fpga->i2c + bus)->busnum = i;
		/* set up i2c mux */
		(fpga->i2c + bus)->mux_ch = 0;
		(fpga->i2c + bus)->mux_en = FPGA_I2C_MUX_DIS;

		//		error = i2c_add_numbered_adapter(&(fpga->i2c+bus)->adapter);
		error = i2c_add_adapter(&(fpga->i2c + bus)->adapter);
		if (error)
			goto out_release_region;

		bus++;
		if (fpga_i2c_info[i].mux_en == FPGA_I2C_MUX_EN)
		{
			for (j = 0; j < fpga_i2c_info[i].num_ch; j++)
			{
				(fpga->i2c + bus)->bar = ioremap(pci_resource_start(dev, 0), pci_resource_len(dev, 0));

				printk(KERN_INFO "BAR0 Register[0x%x] = 0x%x\n",
					   (fpga->i2c + bus)->bar, ioread32((fpga->i2c + bus)->bar));

				/* Create I2C adapter */
				printk(KERN_INFO "dev-%d, pci_base = 0x%x", i, fpga->pci_base + DELTA_I2C_ADDR(i));
				(fpga->i2c + bus)->adapter.owner = THIS_MODULE;
				snprintf((fpga->i2c + bus)->adapter.name, sizeof((fpga->i2c + bus)->adapter.name),
						 "DNI FPGA SMBus-%d (CH-%d)", i, j);
				(fpga->i2c + bus)->adapter.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
				(fpga->i2c + bus)->adapter.algo = &smbus_algorithm;
				(fpga->i2c + bus)->adapter.algo_data = fpga->i2c + bus;
				/* set up the sysfs linkage to our parent device */
				(fpga->i2c + bus)->adapter.dev.parent = &dev->dev;
				/* set i2c adapter number */
				//		(fpga->i2c+bus)->adapter.nr = i+10;
				/* set up the busnum */
				(fpga->i2c + bus)->busnum = i;
				/* set up i2c mux */
				(fpga->i2c + bus)->mux_ch = j;
				(fpga->i2c + bus)->mux_en = FPGA_I2C_MUX_EN;
				error = i2c_add_adapter(&(fpga->i2c + bus)->adapter);
				if (error)
					goto out_release_region;
				bus++;
			}
		}
	}
	return 0;
out_release_region:
	return error;
}

static int dni_fpga_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	struct fpga_dev *fpga;
	int error = -1;
	int ret;

	dev_info(&dev->dev, "probe");

	// enabling the device
	ret = pci_enable_device(dev);
	if (ret)
	{
		printk(KERN_ERR "Failed to enable PCI device.\n");
		return ret;
	}

	if (!(pci_resource_flags(dev, 0) & IORESOURCE_MEM))
	{
		printk(KERN_ERR "Incorrect BAR configuration.\n");
		ret = -ENODEV;
		goto fail_map_bars;
	}

	fpga = kzalloc(sizeof(struct fpga_dev), GFP_KERNEL);
	if (!fpga)
	{
		dev_warn(&dev->dev, "Couldn't allocate memory for fpga!\n");
		return -ENOMEM;
	}

	fpga->buffer = kmalloc(BUF_SIZE * sizeof(char), GFP_KERNEL);
	if (!fpga->buffer)
	{
		dev_warn(&dev->dev, "Couldn't allocate memory for buffer!\n");
		return -ENOMEM;
	}
#ifdef FPGA_GPIO
	/* init gpiodev */
	ret = init_gpiodev(dev, fpga);
	if (ret)
	{
		dev_err(&dev->dev, "Couldn't create gpiodev!\n");
		return ret;
	}
#endif
#ifdef FPGA_CHRDEV
	/* init chrdev */
	ret = init_chrdev(fpga);

	if (ret)
	{
		dev_err(&dev->dev, "Couldn't create chrdev!\n");
		return ret;
	}
#endif

	ret = init_i2c_adapter(dev, fpga);
	if (ret)
	{
		dev_err(&dev->dev, "Couldn't create i2c_adapter!\n");
		goto out_release_region;
	}

	/* MSI  default disable*/
#if 0  
	ret = pci_enable_msi(dev);
        if( ret != 0 )
        {
                printk("error pci enable rc = %d \n",ret);
                return error;
        }
//       ret = pci_alloc_irq_vectors_affinity(dev , 1 , 32, PCI_IRQ_LEGACY | PCI_IRQ_MSI | PCI_IRQ_MSIX | PCI_IRQ_AFFINITY, NULL);
//       if(ret < 0)
//       {
//               printk("error pci msi nvesc = 0x%x \n",ret);
//       }
#endif
	return 0;

fail_map_bars:
	pci_disable_device(dev);
out_release_region:
	release_region(fpga->pci_base, fpga->pci_size);

out_kfree:
	kfree((fpga));
	return error;
}

static void dni_fpga_remove(struct pci_dev *dev)
{
	struct fpga_dev *fpga = pci_get_drvdata(dev);
	printk(KERN_INFO "fpga = 0x%x\n", fpga);
	/* release fpga-i2c */
	int i = 0;

	for (i = 0; i < num_i2c_adapter; i++)
	{
		i2c_del_adapter(&(fpga->i2c + i)->adapter);
		printk(KERN_INFO "Goodbye- again %d\n", i);
	}
#ifdef FPGA_CHRDEV
	/* unregister chrdev */
	cdev_del(&fpga->cdev);
	device_del(fpga->device);
	class_destroy(fpga->my_class);
	unregister_chrdev_region(fpga->cdev_num, 1);
#endif
#ifdef FPGA_GPIO
	/* remove gpio */
	gpiochip_remove(&fpga->gpio->gpio_chip);
#endif
	/* release pci */
	pci_disable_device(dev);
	pci_release_regions(dev);
	/* msi disable */
	//pci_disable_msi(dev);
	kfree(fpga);
	printk(KERN_INFO "Goodbye- again\n");
}

enum chiptype
{
	LATTICE_9C1D
};

static const struct of_device_id dni_fpga_of_match[] = {
	{.compatible = "dni,fpga-i2c"},
	{/* sentinel */}};

static const struct pci_device_id dni_fpga_ids[] = {
	{PCI_DEVICE(0x1172, 0x6964)},
	// ALTERA //
	{PCI_DEVICE(0x1172, 0xc001)},
	// Lattice //
	{PCI_DEVICE(0x1204, 0x9c1d)},
	{0},
};

MODULE_DEVICE_TABLE(pci, dni_fpga_ids);

static struct pci_driver dni_fpga_driver = {
	.name = "dni-fpga-i2c",
	.id_table = dni_fpga_ids,
	.probe = dni_fpga_probe,
	.remove = dni_fpga_remove,
};

//module_pci_driver(dni_fpga_driver);

static int hello_init(void)
{
	printk(KERN_INFO "Hello kernel-1\n");
	return pci_register_driver(&dni_fpga_driver);
}

static void hello_exit(void)
{
	printk(KERN_INFO "Goodbye\n");
	pci_unregister_driver(&dni_fpga_driver);
}

MODULE_AUTHOR("amos.lin@deltaww.com");
MODULE_DESCRIPTION("DNI FPGA I2C Driver");
MODULE_LICENSE("GPL");

module_init(hello_init);
module_exit(hello_exit);
