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
#include "fpga.h"
#include "fpga_fileio.h"

/* Given (bar_id, device_addr) pair, make sure they're valid and return
 * the resulting address. errno will contain error code, if any. */
void* fpga_get_checked_addr (int bar_id, void *device_addr, size_t count,
		struct fpga_dev *fdev, ssize_t *errno, int print_error_msg)
{

	if (bar_id >= 1) {
		printk(KERN_WARNING "Requested read/write from BAR #%d. Only have %d BARs!",
		       bar_id, 1);
		*errno = -EFAULT;
		return 0;
	}
	/* Make sure the final address is within range */
	if (((unsigned long)device_addr + count) > fdev->pci_size) {
		if (print_error_msg) {
			printk(KERN_WARNING "Requested read/write from BAR #%d from range (%lu, %lu).\n"
		               "Length is %lu. BAR length is only %lu!",
			       bar_id,
			       (unsigned long)device_addr,
			       (unsigned long)device_addr + count,
			       count,
			       fdev->pci_base);
		}
		*errno = -EFAULT;
		return 0;
	}

	*errno = 0;
	return (void*)(fdev->pci_base + (unsigned long)device_addr);
}
/* Response to user's open() call */
int fpga_open(struct inode *inode, struct file *file)
{
	struct fpga_dev *fdev;

	fdev = container_of(inode->i_cdev, struct fpga_dev, cdev);
	file->private_data = fdev;

	return 0;
}

/* Response to user's close() call. Will also be called by the kernel
 * if the user process dies for any reason. */
int fpga_close(struct inode *inode, struct file *file)
{
	return 0;
}

int fpga_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct fpga_dev *fdev = (struct fpga_dev *)file->private_data;
	size_t size = vma->vm_end - vma->vm_start;
	unsigned long offset;

	offset = vma->vm_pgoff << PAGE_SHIFT;

	if ((offset + size) > fdev->pci_base)
		return -EINVAL;

	offset += pci_resource_start(fdev->dev, 0);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma,
	                       vma->vm_start,
			       offset >> PAGE_SHIFT,
			       size,
			       vma->vm_page_prot)){
		return -EAGAIN;
	}
	return 0;

}
/* Response to user's read() call */
ssize_t fpga_read(struct file *file, char __user *buf, size_t count,
		loff_t *pos)
{
	return fpga_rw (file, buf, count, pos, 1 /* reading */);
}


/* Response to user's write() call */
ssize_t fpga_write(struct file *file, const char __user *buf, size_t count,
		loff_t *pos)
{
	return fpga_rw (file, (char __user *)buf, count, pos, 0 /* writing */);
}

/* Read a small number of bytes and put them into user space */
ssize_t fpga_read_small (void *read_addr, void __user* dest_addr, ssize_t len)
{

	ssize_t copy_res = 0;

	switch (len) {
	case 1:
		{
			u8 d = readb ( read_addr );
			copy_res = copy_to_user ( dest_addr, &d, sizeof(d) );
			break;
		}
	case 2:
		{
			u16 d = readw ( read_addr );
			copy_res = copy_to_user ( dest_addr, &d, sizeof(d) );
			break;
	        }
	case 4:
		{
			u32 d = readl ( read_addr );
			copy_res = copy_to_user ( dest_addr, &d, sizeof(d) );
			break;
		}
	case 8:
		{
			u64 d = readq ( read_addr );
			copy_res = copy_to_user ( dest_addr, &d, sizeof(d) );
			break;
		}
	default:
		break;
	}

	if (copy_res) {
		return -EFAULT;
	} else {
		return 0;
	}
}


/* Write a small number of bytes taken from user space */
ssize_t fpga_write_small (void *write_addr, void __user* src_addr, ssize_t len)
{

	ssize_t copy_res = 0;
	switch (len) {
	case 1:
		{
			u8 d;
			copy_res = copy_from_user ( &d, src_addr, sizeof(d) );
			writeb ( d, write_addr );
			break;
		}
	case 2:
		{
			u16 d;
			copy_res = copy_from_user ( &d, src_addr, sizeof(d) );
			writew ( d, write_addr );
			break;
		}
	case 4:
		{
			u32 d;
			copy_res = copy_from_user ( &d, src_addr, sizeof(d) );
			writel ( d, write_addr );
			break;
		}
	case 8:
		{
			u64 d;
			copy_res = copy_from_user ( &d, src_addr, sizeof(d) );
			writeq ( d, write_addr );
			break;
		}
	default:
		break;
	}

	if (copy_res) {
		return -EFAULT;
	} else {
		return 0;
	}
}



/* Read or Write arbitrary length sequency starting at read_addr and put it into
 * user space at dest_addr. if 'reading' is set to 1, doing the read. If 0, doing
 * the write. */
static ssize_t fpga_rw_large (void *dev_addr, void __user* user_addr,
		ssize_t len, char *buffer, int reading)
{
	size_t bytes_left = len;
	size_t i, num_missed;
	u64 *ibuffer = (u64*)buffer;
	char *cbuffer;
	size_t offset, num_to_read;
	size_t chunk = BUF_SIZE;

	u64 startj, ej;
	u64 sj = 0, acc_readj = 0, acc_transfj = 0;

	startj = get_jiffies_64();

	/* Reading upto BUF_SIZE values, one int at a time, and then transfer
	 * the buffer at once to user space. Repeat as necessary. */
	while (bytes_left > 0) {
		if (bytes_left < BUF_SIZE) {
			chunk = bytes_left;
		} else {
			chunk = BUF_SIZE;
		}

		if (!reading) {
			sj = get_jiffies_64();
			if (copy_from_user (ibuffer, user_addr, chunk)) {
				return -EFAULT;
			}
			acc_transfj += get_jiffies_64() - sj;
		}

		/* Read one u64 at a time until fill the buffer. Then copy the whole
		 * buffer at once to user space. */
		sj = get_jiffies_64();
		num_to_read = chunk / sizeof(u64);
		for (i = 0; i < num_to_read; i++) {
			if (reading) {
				ibuffer[i] = readq ( ((u64*)dev_addr) + i);
			} else {
				writeq ( ibuffer[i], ((u64*)dev_addr) + i );
			}
		}

		/* If length is not a multiple of sizeof(u64), will miss last few bytes.
		 * In that case, read it one byte at a time. This can only happen on 
		 * last iteration of the while() loop. */
		offset = num_to_read * sizeof(u64);
		num_missed = chunk - offset;
		cbuffer = (char*)(ibuffer + num_to_read);

		for (i = 0; i < num_missed; i++) {
			if (reading) {
				cbuffer[i] = readb ( (u8*)(dev_addr) + offset + i );
			} else {
				writeb ( cbuffer[i], (u8*)(dev_addr) + offset + i );
			}
		}
		acc_readj += get_jiffies_64() - sj;

		if (reading) {
			sj = get_jiffies_64();
			if (copy_to_user (user_addr, ibuffer, chunk)) {
				return -EFAULT;
			}
			acc_transfj += get_jiffies_64() - sj;
		}

		dev_addr += chunk;
		user_addr += chunk;
		bytes_left -= chunk;
	}

	ej = get_jiffies_64();
	printk(KERN_DEBUG "Spent %u msec %sing %lu bytes", jiffies_to_msecs(ej - startj),
	                reading ? "read" : "writ", len);
	printk(KERN_DEBUG "  Dev access %u msec. User space transfer %u msec",
	                jiffies_to_msecs(acc_readj),
	                jiffies_to_msecs(acc_transfj));
	return 0;
}

/* High-level read/write dispatcher. */
ssize_t fpga_rw(struct file *file, char __user *buf,
		size_t count, loff_t *pos, int reading)
{
	struct fpga_dev *fdev = (struct fpga_dev *)file->private_data;
	struct fpga_cmd __user *ucmd;
	struct fpga_cmd kcmd;
	void *addr = 0;
	// int use_dma = 0;
	ssize_t result = 0;
	ssize_t errno = 0;

	if (down_interruptible(&fdev->sem)) {
		return -ERESTARTSYS;
	}

	ucmd = (struct fpga_cmd __user *) buf;
	if (copy_from_user (&kcmd, ucmd, sizeof(*ucmd))) {
		result = -EFAULT;
		goto done;
	}
#if 0
	printk(KERN_DEBUG "\n\n-----------------------");
	printk(KERN_DEBUG " kcmd = {%u, %p, %p}, count = %lu",
			kcmd.bar_id, (void*)kcmd.device_addr, (void*)kcmd.user_addr, count);
#endif
	if (kcmd.command == FPGAPCI_CMD_GET_BAR_LENGTH) {
		u32 d = fdev->pci_size;
		result = copy_to_user( kcmd.user_addr, &d, sizeof(d));
		if (result)
			result = -EFAULT;
		goto done;
	}
	addr = fpga_get_checked_addr (kcmd.bar_id, kcmd.device_addr, count, fdev, &errno, 0);

	if (errno != 0) {
		result = -EFAULT;
		goto done;
	}

	/* Offset value is always an address offset, not element offset. */
	/* printk(KERN_DEBUG "Read address is %p", addr); */
	switch (count) {
	case 1:
	case 2:
	case 4:
	case 8:
		if (reading) {
			result = fpga_read_small (addr, (void __user*) kcmd.user_addr, count);
		} else {
			result = fpga_write_small (addr, (void __user*) kcmd.user_addr, count);
		}
		break;
	default:
		result = fpga_rw_large (addr, (void __user*) kcmd.user_addr, count, fdev->buffer, reading);
		break;
	}

done:
	up (&fdev->sem);
	return result;
}


