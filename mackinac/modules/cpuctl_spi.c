// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia cpctl/ioctl spi bus
 *
 *  Copyright (C) 2025 Nokia
 *
 */

/*
16 channels per spi bus

4.1.3 SPI bus mapping
On CPM side:
- FPGA Flash connect to SPI Ctrl 0 bus 0  timer=1  /dev/spidev0.0
- SETS is mapped to SPI Ctrl 0 bus 1      timer=16 /dev/spidev0.1
- PCON4 is mapped to SPI Ctrl 0 bus 2     timer=6  /dev/spidev0.2
- ESPLL is mapped to SPI Ctrl 0 bus 6     /dev/spidev0.6
- Other Device Flash is mapped to SPI Ctrl 1 bus 0

On IOM side:
 (x3b)
- PCON Flash                              timer=1  /dev/spidev1.0
- PCON0 is mapped to SPI Ctrl 0 bus 1     timer=6  /dev/spidev1.1
- PCON1 is mapped to SPI Ctrl 0 bus 2     timer=6  /dev/spidev1.2
- PCON2 is mapped to SPI Ctrl 0 bus 3     timer=6  /dev/spidev1.3
 (x1b)
- PCON Flash                              timer=1  /dev/spidev1.0
- PCON0 is mapped to SPI Ctrl 0 bus 1     timer=6  /dev/spidev1.1
- PCON2 is mapped to SPI Ctrl 0 bus 4     timer=6  /dev/spidev1.4
 (x4)
- PCON Flash                              timer=1  /dev/spidev1.0
- PCON0 is mapped to SPI Ctrl 0 bus 1     timer=6  /dev/spidev1.1
- PCON1 is mapped to SPI Ctrl 0 bus 2     timer=6  /dev/spidev1.4
*/
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>
#include <linux/unaligned.h>
#include "cpuctl.h"

#define SPI_1BYTE 0
#define SPI_2BYTE 1
#define SPI_3BYTE 2
#define SPI_4BYTE 3

#define SPI_IDT_SETS_TIMER 16
#define SPI_TIMER_DEFAULT 6
#define SPI_FPI_TIMER     1
#define SPI_SPEED_HALF 0
#define SPI_SPEED_FULL 1
#define SPI_DEFAULT_SPEED (SPI_SPEED_FULL)
#define SPI_MAX_SPEED_HZ 125000000 / (2 + (2 * SPI_IDT_SETS_TIMER))
#define SPI_MIN_SPEED_HZ 125000000 / (2 + (2 * SPI_TIMER_DEFAULT))

// shift values (bit positions) for controller interface
#define S_SPI_CHANNEL 0
#define S_SPI_WR_BYTES 5
#define S_SPI_WRITE 7
#define S_SPI_RD_BYTES 8
#define S_SPI_READ 10
#define S_SPI_END 14
#define S_SPI_SPEED 15
#define S_SPI_TIMER 16
#define S_SPI_BUSY 24
#define S_SPI_NOACK 25
#define S_SPI_ERROR 26


#define S_SPI_CHANNEL_INVALID -1
int bus0_channums[brd_max][N_SPI_MINORS] = {
    {0, 1, 2, S_SPI_CHANNEL_INVALID},
    {0, 1, 2, S_SPI_CHANNEL_INVALID},
    {0, 1, 2, S_SPI_CHANNEL_INVALID}
};

int bus1_channums[brd_max][N_SPI_MINORS] = {
    {0, 1, 2, 3},
    {0, 1, 4, S_SPI_CHANNEL_INVALID},
    {0, 1, 2, S_SPI_CHANNEL_INVALID}
};

static int ctl_spi_check_status(CTLDEV *pdev)
{
    u16 val;
    int rc;

    rc = read_poll_timeout_atomic(ctl_reg16_read, val,
            ((val & ((1 << S_SPI_BUSY) >> 16)) == 0),
            5, 100 * USEC_PER_MSEC, 1, pdev, A32_SPI_CTRL_SR);

    if (rc)
    {
        dev_err(&pdev->pcidev->dev, "spi timeout 0x%04x\n", val);
        return -ETIMEDOUT;
    }

    if (val & ((1 << S_SPI_ERROR) >> 16))
    {
        dev_err(&pdev->pcidev->dev, "spi controller error 0x%04x\n", val);
        return -EIO;
    }

    return 0;
}

static int __spi_read(CTLDEV *pdev, u8 *data, unsigned len, int endop, unsigned channel, unsigned timer)
{
    int status;
    u32 val;
    int rlen = len;

    if (len > 4)
    {
        rlen = 4;
    }

    val = (channel << S_SPI_CHANNEL);
    val |= SPI_DEFAULT_SPEED << S_SPI_SPEED;
    val |= (timer << S_SPI_TIMER);
    val |= (endop << S_SPI_END);
    if (rlen > 0)
    {
        val |= (1 << S_SPI_READ) | ((rlen - 1) << S_SPI_RD_BYTES);
    }
    ctl_reg_write(pdev, A32_SPI_CTRL_SR, val);
    if (debug & CTL_DEBUG_SPI)
        dev_dbg(&pdev->pcidev->dev, "%s cntr 0x%08x rlen %d\n", __FUNCTION__, val, rlen);
    udelay(10);

    status = ctl_spi_check_status(pdev);
    if (status < 0)
    {
        return status;
    }

    val = ctl_reg_read(pdev, A32_SPI_DATA_SR);
    if (debug & CTL_DEBUG_SPI)
        dev_dbg(&pdev->pcidev->dev, "%s data 0x%08x \n", __FUNCTION__, val);
    switch (rlen)
    {
    case 1:
        data[0] = (u8)val;
        break;
    case 2:
        data[0] = (u8)(val >> 8);
        data[1] = (u8)(val);
        break;
    case 3:
        put_unaligned_be24(val, data);
        break;
    case 4:
        put_unaligned_be32(val, data);
        break;
    default:
        break;
    }

    return rlen; /* bytes read */
}

static int __spi_write(CTLDEV *pdev, const u8 *buf, unsigned len, int endop, unsigned channel, unsigned timer)
{
    int rc;
    u32 val;
    u32 data = 0;
    u32 shift = 24;
    u32 wlen = len;
    u32 xlen = 0;

    if (len && len > 4)
        wlen = 4;

    xlen += wlen;

    if (endop)
        endop = (1 << S_SPI_END);

    switch (wlen)
    {
    case 1:
        data |= buf[0] << shift;
        break;
    case 2:
        data |= buf[0] << shift;
        data |= buf[1] << (shift - 8);
        break;
    case 3:
        data |= buf[0] << shift;
        data |= buf[1] << (shift - 8);
        data |= buf[2] << (shift - 16);
        break;
    case 4:
        data |= buf[0] << shift;
        data |= buf[1] << (shift - 8);
        data |= buf[2] << (shift - 16);
        data |= buf[3];
        break;
    default:
        break;
    }

    ctl_reg_write(pdev, A32_SPI_DATA_SR, data);
    if (debug & CTL_DEBUG_SPI)
        dev_dbg(&pdev->pcidev->dev, "%s data 0x%08x \n", __FUNCTION__, data);

    val = (channel << S_SPI_CHANNEL);
    val |= SPI_DEFAULT_SPEED << S_SPI_SPEED;
    val |= (timer << S_SPI_TIMER);
    val |= ((xlen - 1) << S_SPI_WR_BYTES) | (1 << S_SPI_WRITE) | endop;

    ctl_reg_write(pdev, A32_SPI_CTRL_SR, val);
    udelay(10);
    if (debug & CTL_DEBUG_SPI)
        dev_dbg(&pdev->pcidev->dev, "%s cntr 0x%08x \n", __FUNCTION__, val);

    rc = ctl_spi_check_status(pdev);
    if (rc == 0)
        rc = wlen;  /* bytes written */

    return rc;
}

static int ctlspi_spi_controller_transfer(struct spi_controller *spicon, struct spi_message *message)
{
    CTLDEV *pdev = spi_controller_get_devdata(spicon);
    int rc;
    struct spi_transfer *xfer = NULL;
    u32 wrbytes = 0, rdbytes = 0;
    u32 txlen = 0, rxlen = 0;
    u64 start;
    unsigned dur;
    int bus = spicon->bus_num;
    unsigned timer = SPI_TIMER_DEFAULT;
    unsigned channel = message->spi->chip_select[0];

    if (spicon->bus_num == 0 && channel == 1)
        timer = SPI_IDT_SETS_TIMER;
    else if (channel == 0)
        timer = SPI_FPI_TIMER;

    start = ktime_get_ns();
    list_for_each_entry(xfer, &message->transfers, transfer_list)
    {

        if (xfer->tx_buf)
        {
            int endop = 0;
            u8 *buf = (u8 *)xfer->tx_buf;
            wrbytes += xfer->len;
            txlen += xfer->len;

            dev_dbg(&pdev->pcidev->dev, "spidev%d.%d tx len %d\n", bus,
                channel, xfer->len);
        
            while (wrbytes)
            {
                if (xfer->cs_change && wrbytes <= 4)
                {
                    endop = 1;
                }
                rc = __spi_write(pdev, buf, wrbytes, endop, channel, timer);
                if (rc < 0)
                {
                    message->status = rc;
                    return rc;
                }
                wrbytes -= rc;
                buf += rc;
            }
        }
    }

    list_for_each_entry(xfer, &message->transfers, transfer_list)
    {

        if (xfer->rx_buf)
        {
            int endop = 0;
            u8 *buf = (u8 *)xfer->rx_buf;
            rdbytes = xfer->len;
            rxlen += xfer->len;

            dev_dbg(&pdev->pcidev->dev, "spidev%d.%d rx len %d\n", bus,
                channel, xfer->len);

            while (rdbytes > 0)
            {
                /* read */
                if (xfer->cs_change && rdbytes <= 4)
                {
                    endop = 1;
                }
                rc = __spi_read(pdev, buf, rdbytes, endop, channel, timer);
                if (rc < 0)
                    break;
                rdbytes -= rc;
                buf += rc;
            }
            if (rc < 0)
            {
                message->status = rc;
                return rc;
            }
        }

    }

    dur = (ktime_get_ns() - start) / 1000;
    dev_dbg(&pdev->pcidev->dev, "spidev%d.%d time %dus\n", bus,
        channel, dur);

    if (rc >= 0)
    {
        message->status = rc = 0;
    }

    message->actual_length = txlen + rxlen;
    spi_finalize_current_message(spicon);

    return rc;
}

static int ctlspi_spi_controller_setup(struct spi_device *spi)
{
    return 0;
}

static size_t ctl_spi_max_transfer_size(struct spi_device *spi)
{
    return 128;
}

int spi_device_create(CTLDEV *pdev)
{
    struct device *dev = &pdev->pcidev->dev;
    struct spi_controller *spicon;
    int rc = 0, i;
    int bus_num = pdev->ctlv->spi_bus;

    spicon = devm_spi_alloc_master(dev, sizeof(CTLDEV));
    if (!spicon)
        return -ENOMEM;

    /* Initialize the spi_controller fields */
    spicon->dev.parent = dev;
    spicon->bus_num = bus_num;
    spicon->num_chipselect = N_SPI_MINORS;
    spicon->flags = SPI_CONTROLLER_HALF_DUPLEX;
    mutex_init(&spicon->bus_lock_mutex);
    // spicon->max_speed_hz = SPI_MAX_SPEED_HZ;
    // spicon->min_speed_hz = SPI_MIN_SPEED_HZ;
    spicon->setup = ctlspi_spi_controller_setup;
    spicon->transfer_one_message = ctlspi_spi_controller_transfer;
    spicon->max_transfer_size = ctl_spi_max_transfer_size;
    spicon->max_message_size = ctl_spi_max_transfer_size;

    rc = devm_spi_register_controller(dev, spicon);
    if (rc)
    {
        dev_err(dev, "error devm_spi_register_master spi rc=%d\n", rc);
    }
    else
    {
        dev_info(dev, "devm_spi_register_master\n");
    }

    spi_controller_set_devdata(spicon, pdev);

    if (bus_num == 0) {
        for (i = 0; i < N_SPI_MINORS; i++)
        {
            struct spi_board_info sbi = {{0}};

            if (bus0_channums[board][i] == S_SPI_CHANNEL_INVALID)
                continue;

            sbi.bus_num = bus_num;
            strcpy(sbi.modalias, "ltc2488");
            sbi.chip_select = bus0_channums[board][i];
            spi_new_device(spicon, &sbi);
        }
    }
    else {
        for (i = 0; i < N_SPI_MINORS; i++)
        {
            struct spi_board_info sbi = {{0}};

            if (bus1_channums[board][i] == S_SPI_CHANNEL_INVALID)
                continue;

            sbi.bus_num = bus_num;
            strcpy(sbi.modalias, "ltc2488");
            sbi.chip_select = bus1_channums[board][i];
            spi_new_device(spicon, &sbi);
        }
    }

    return rc;
}

void spi_device_remove(CTLDEV *pdev)
{

}
