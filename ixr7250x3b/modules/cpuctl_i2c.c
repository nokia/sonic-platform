// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia cpctl/ioctl i2c bus adapter/multiplexer
 *
 *  Copyright (C) 2024 Nokia
 *
 */

#include <linux/iopoll.h>
#include <asm/unaligned.h>
#include "cpuctl.h"

#define CTL_I2C_DATA    0x02700018
#define CTL_I2C_CNTR    0x0270001C
#define CTL_I2C_CNTR_seq_err_det   (1 << 26)
#define CTL_I2C_CNTR_seq_err_det16 (CTL_I2C_CNTR_seq_err_det >> 16)
#define CTL_I2C_CNTR_slave_ack_not (1 << 25)
#define CTL_I2C_CNTR_busy          (1 << 24)
#define CTL_I2C_CNTR_busy16        (CTL_I2C_CNTR_busy >> 16)
#define CTL_I2C_CNTR_base_timer_m  0x00ff0000
#define CTL_I2C_CNTR_base_timer_o  16
#define CTL_I2C_CNTR_freq_400_o    15
#define CTL_I2C_CNTR_gen_end_b     (1 << 14)
#define CTL_I2C_CNTR_gen_start_b   (1 << 13)
#define CTL_I2C_CNTR_restart_m     0x1800
#define CTL_I2C_CNTR_restart_b     (11)
#define CTL_I2C_CNTR_read_req_b    (1 << 10)
#define CTL_I2C_CNTR_rcv_cnt_m     0x0300
#define CTL_I2C_CNTR_rcv_cnt_o     8
#define CTL_I2C_CNTR_write_req_b   (1 << 7)
#define CTL_I2C_CNTR_xmt_cnt_m     0x60
#define CTL_I2C_CNTR_xmt_cnt_o     (5)
#define CTL_I2C_CNTR_bus_sel_m     0x001f


#define CTL_I2C_CNTR_no_restart (3 << CTL_I2C_CNTR_restart_b)

static void ctl_i2c_abort(CTLDEV *pdev)
{
	u16 rval, wval;
	rval = ctl_reg16_read(pdev, CTL_I2C_CNTR);
	if (rval & (CTL_I2C_CNTR_busy16 | CTL_I2C_CNTR_seq_err_det16))
	{
		wval = rval & 0x00ff;
		wval |= (1 << 12); /* disables clock stretching hold-off */
		ctl_reg16_write(pdev, CTL_I2C_CNTR, wval);
		mdelay(1);
		dev_warn(&pdev->pcidev->dev, "%s cntr 0x%04x\n", __FUNCTION__, rval);
	}
}

static int ctl_i2c_check_status(CTLDEV *pdev)
{
	u32 val;
	int rc;

	rc = read_poll_timeout_atomic(ctl_reg_read, val,
								  ((val & CTL_I2C_CNTR_busy) == 0),
								  5, 100 * USEC_PER_MSEC, 1, pdev, CTL_I2C_CNTR);

	if (rc)
	{
		dev_err(&pdev->pcidev->dev, "i2c timeout error 0x%08x\n", val);
		ctl_i2c_abort(pdev);
		return -ETIMEDOUT;
	}

	val = ctl_reg_read(pdev, CTL_I2C_CNTR);
	if (val & CTL_I2C_CNTR_seq_err_det)
	{
		dev_err(&pdev->pcidev->dev, "i2c CTL_I2C_CNTR_seq_err_det 0x%08x\n", val);
		ctl_i2c_abort(pdev);
		return -EIO;
	}
	if (val & CTL_I2C_CNTR_slave_ack_not)
	{
		dev_dbg(&pdev->pcidev->dev, "i2c CTL_I2C_CNTR_slave_ack_not 0x%08x\n", val);
		return -ENXIO;
	}

	return 0;
}

static inline int ctl_i2c_bus_speed_get(CTLDEV *pdev)
{
	return (pdev->ctlv->bus400 >> pdev->phys_chan) & 1;
}

static int ctl_i2c_read(CTLDEV *pdev, u8 addr, u8 *data, u16 len)
{
	int status;
	u32 val;
	int rlen = len;

	if (len > 4)
	{
		rlen = 4;
	}

	/* address to data reg */
	val = ((addr << 1) | 1) << 24; /* read */
	ctl_reg_write(pdev, CTL_I2C_DATA, val);

	/* controller */
	val = pdev->phys_chan & CTL_I2C_CNTR_bus_sel_m;
	val |= ctl_i2c_bus_speed_get(pdev) << CTL_I2C_CNTR_freq_400_o;
	val |= (0x1f << CTL_I2C_CNTR_base_timer_o);
	val |= CTL_I2C_CNTR_no_restart | (0 << CTL_I2C_CNTR_xmt_cnt_o) |
		   CTL_I2C_CNTR_write_req_b | CTL_I2C_CNTR_gen_start_b | CTL_I2C_CNTR_gen_end_b;
	if (rlen > 0)
	{
		val |= CTL_I2C_CNTR_read_req_b | (((rlen - 1) << CTL_I2C_CNTR_rcv_cnt_o) & CTL_I2C_CNTR_rcv_cnt_m);
	}
	ctl_reg_write(pdev, CTL_I2C_CNTR, val);
	dev_dbg(&pdev->pcidev->dev, "%s cntr 0x%08x rlen %d\n", __FUNCTION__, val, rlen);
	udelay(10);

	status = ctl_i2c_check_status(pdev);
	if (status < 0)
	{
		dev_dbg(&pdev->pcidev->dev, "%s status %d\n", __FUNCTION__, status);
		return status;
	}

	val = ctl_reg_read(pdev, CTL_I2C_DATA);
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

	/* num bytes read */
	return rlen;
}

static int ctl_i2c_write(CTLDEV *pdev, u8 addr, u8 *buf, u16 len, u32 start, u32 end)
{
	int rc;
	u32 val;
	u32 data = 0;
	u32 shift = 24;
	u32 wlen = len;
	u32 xlen = 0;

	if (start) {
		/* first byte for device address */
		start = CTL_I2C_CNTR_gen_start_b;
		data = (addr << 1) << 24;
		shift -= 8;
		xlen += 1;
		if(len && len > 3)
			wlen = 3;
	}
	else {
		if(len && len > 4)
			wlen = 4;
	}

	xlen += wlen;

	if (end)
		end = CTL_I2C_CNTR_gen_end_b;

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

	ctl_reg_write(pdev, CTL_I2C_DATA, data);
	dev_dbg(&pdev->pcidev->dev, "%s data 0x%08x \n", __FUNCTION__, data);

	val = pdev->phys_chan & CTL_I2C_CNTR_bus_sel_m;
	val |= ctl_i2c_bus_speed_get(pdev) << CTL_I2C_CNTR_freq_400_o;
	val |= (0x1f << CTL_I2C_CNTR_base_timer_o);
	val |= ((xlen - 1) << CTL_I2C_CNTR_xmt_cnt_o) | CTL_I2C_CNTR_write_req_b |
		   CTL_I2C_CNTR_no_restart | start | end;
	ctl_reg_write(pdev, CTL_I2C_CNTR, val);
	udelay(10);
	dev_dbg(&pdev->pcidev->dev, "%s cntr 0x%08x \n", __FUNCTION__, val);

	rc = ctl_i2c_check_status(pdev);
	if (rc == 0)
		rc = wlen;

	return rc;
}

static int ctl_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	int i;
	CTLDEV *pdev = i2c_get_adapdata(adap);
	int rc;

	for (i = 0; i < num; i++)
	{
		dev_dbg(&pdev->pcidev->dev, "%s msg%d: addr 0x%02x %s len %d flags 0x%04x\n", __FUNCTION__,
				i, msgs[i].addr, (msgs[i].flags & 1) ? "rd" : "wr",
				msgs[i].len, msgs[i].flags);
		if (msgs[i].flags & I2C_M_RD)
		{
			unsigned rdbytes = msgs[i].len;
			u8 *rbuf = msgs[i].buf;
			while(rdbytes > 0) {
				/* read */
				rc = ctl_i2c_read(pdev, msgs[i].addr, rbuf, rdbytes);
				if (rc < 0)
					break;
				rdbytes -= rc;
				rbuf += rc;
			}
		}
		else
		{
			/* write */
			unsigned wrbytes = msgs[i].len;
			u8 *wbuf = msgs[i].buf;
			if (wrbytes == 0) {
				/* ack poll */
				rc = ctl_i2c_write(pdev, msgs[i].addr, wbuf, wrbytes, 1, 1);
			}
			else
			{
				u32 start = 1;
				while(wrbytes > 0)
				{
					u32 end = 0;
					if ((start == 1 && wrbytes <= 3) || (start == 0 && wrbytes <= 4)) {
						/* single xfer or last xfer */
						end = 1;
					}
					else {
						end = 0;
					}
					rc = ctl_i2c_write(pdev, msgs[i].addr, wbuf, wrbytes, start, end);
					if (rc < 0)
						break;
					start = 0;
					wrbytes -= rc;
					wbuf += rc;
				}
			}
		}
		if (rc < 0)
			break;
	}

	if (rc >= 0)
	{
		/* num msgs processed */
		rc = i;
	}

	dev_dbg(&pdev->pcidev->dev, "%s returning %d\n", __FUNCTION__, rc);
	return rc;
}

static u32 ctl_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_QUICK |
		   I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA |
		   I2C_FUNC_SMBUS_WORD_DATA;
}

static const struct i2c_algorithm ctl_i2c_algo = {
	.master_xfer = ctl_i2c_xfer,
	.smbus_xfer = NULL,
	.functionality = ctl_i2c_func
};

static const struct i2c_adapter_quirks ctl_i2c_quirks = {
	.max_write_len = 128,
	.max_comb_1st_msg_len = 128,
	.max_comb_2nd_msg_len = 128,
	.flags = I2C_AQ_COMB_WRITE_THEN_READ,
};

static int ctl_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
	int rc = 0;
	struct ctlmux *data = i2c_mux_priv(muxc);
	CTLDEV *pdev = data->pdev;
	struct chan_map *pchan;

	dev_dbg(&pdev->pcidev->dev, "%s chan %d\n", __FUNCTION__, chan);

	pchan = &pdev->ctlv->pchanmap[chan];
	pdev->phys_chan = pchan->phys_chan;
	if (pchan->modsel >= 0) {
		/* phys_chan has modsel */
		if (pchan->modsel != pdev->current_modsel) {
			unsigned int offset;
			u32 val;
			dev_dbg(&pdev->pcidev->dev, "%s chan %d modsel %d\n", __FUNCTION__, chan,pchan->modsel);
			offset = pchan->modsel < 32 ? CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE : (CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE + 4);
			/* deselect all */
			ctl_reg_write(pdev,CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE,0xffffffff);
			ctl_reg_write(pdev,CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE + 4,0xffffffff);
			val = ~(1 << (pchan->modsel % 32));
			ctl_reg_write(pdev,offset,val);
			msleep(5);
			pdev->current_modsel = pchan->modsel;
		}
	}

	return rc;
}

static int ctl_deselect_mux(struct i2c_mux_core *muxc, u32 chan)
{
	int rc = 0;
	struct ctlmux *data = i2c_mux_priv(muxc);
	CTLDEV *pdev = data->pdev;
	struct chan_map *pchan;
	
	dev_dbg(&pdev->pcidev->dev, "%s chan %d\n", __FUNCTION__, chan);

	pchan = &pdev->ctlv->pchanmap[chan];
	pdev->phys_chan = 0;
	return rc;
}

int ctl_i2c_probe(CTLDEV *pdev)
{
	int rc;

	/* add ctl adapter (i2c host controller) */
	i2c_set_adapdata(&pdev->adapter, pdev);
	pdev->adapter.owner = THIS_MODULE;
	pdev->adapter.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	pdev->adapter.algo = &ctl_i2c_algo;
	pdev->adapter.quirks = &ctl_i2c_quirks;
	pdev->adapter.dev.parent = &pdev->pcidev->dev;
	sprintf(pdev->adapter.name, "Nokia %s adapter", pdev->ctlv->name);
	rc = i2c_add_adapter(&pdev->adapter);
	if (rc)
	{
		return -ENODEV;
	}

	/* create a logical mux on this adapter
	   to handle bus select reg */
	{
		struct ctlmux *data;
		int i, nchans;

		/* make a dummy mux device */
		nchans = pdev->ctlv->nchans;
		pdev->current_modsel = -1;
		pdev->ctlmuxcore = i2c_mux_alloc(&pdev->adapter, &pdev->pcidev->dev, nchans, sizeof(struct ctlmux), 0,
										 ctl_select_chan, ctl_deselect_mux);
		if (!pdev->ctlmuxcore)
		{
			return -ENOMEM;
		}
		data = i2c_mux_priv(pdev->ctlmuxcore);
		data->pdev = pdev;

		/* make nchan adapters on the mux */
		for (i = 0; i < nchans; i++)
		{
			rc = i2c_mux_add_adapter(pdev->ctlmuxcore, 0, i, 0);
			if (rc)
			{
				i2c_mux_del_adapters(pdev->ctlmuxcore);
				break;
			}
		}
	}

	return rc;
}

void ctl_i2c_remove(CTLDEV *pdev)
{
	i2c_mux_del_adapters(pdev->ctlmuxcore);
	i2c_del_adapter(&pdev->adapter);
}
