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

#define FPGAPCI_CMD_GET_BAR_LENGTH	1

/* Main structure to communicate any command (including read/write)
 * from user space to the driver. */
struct fpga_cmd {

	/* base address register of PCIe device. device_addr is interpreted
	 * as an offset from this BAR's start address. */
	unsigned int bar_id;

	/* Special command to execute. Only used if bar_id is set
	 * to ACLPCI_CMD_BAR. */
	unsigned int command;

	/* Address in device space where to read/write data. */
	void* device_addr;

	/* Address in user space where to write/read data.
	 * Always virtual address. */
	void* user_addr;

};

int fpga_open(struct inode *inode, struct file *file);
int fpga_close(struct inode *inode, struct file *file);
ssize_t fpga_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
ssize_t fpga_write(struct file *file, const char __user *buf, size_t count, loff_t *pos);
int fpga_mmap(struct file *file, struct vm_area_struct *vma);
ssize_t fpga_rw(struct file *file, char __user *buf, size_t count, loff_t *pos, int reading);

