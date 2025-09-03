// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Nokia cpctl/ioctl i2c bus adapter/multiplexer
 *
 *  Copyright (C) 2024 Nokia
 *
 */

#include <linux/iopoll.h>
#include <linux/unaligned.h>
#include "cpuctl.h"

#define CTL_I2C_DATA    0x02700018
#define CTL_I2C_CNTR    0x0270001C
#define CTL_I2C_CNTR_seq_err_det   (1 << 26)
#define CTL_I2C_CNTR_seq_err_det16 (CTL_I2C_CNTR_seq_err_det >> 16)
#define CTL_I2C_CNTR_slave_ack_not (1 << 25)
#define CTL_I2C_CNTR_slave_ack_not16 (CTL_I2C_CNTR_slave_ack_not >> 16)
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
	u16 val;
	int rc;

	rc = read_poll_timeout_atomic(ctl_reg16_read, val,
		((val & CTL_I2C_CNTR_busy16 ) == 0),
		10, 100 * USEC_PER_MSEC, 1, pdev, CTL_I2C_CNTR);

	if (rc)
	{
		dev_err(&pdev->pcidev->dev, "i2c timeout error 0x%04x\n", val);
		ctl_i2c_abort(pdev);
		return -ETIMEDOUT;
	}

	if (val & CTL_I2C_CNTR_seq_err_det16)
	{
		dev_err(&pdev->pcidev->dev, "i2c CTL_I2C_CNTR_seq_err_det 0x%04x\n", val);
		ctl_i2c_abort(pdev);
		return -EIO;
	}

	if (val & CTL_I2C_CNTR_slave_ack_not16)
	{
		if(debug & CTL_DEBUG_I2C)
			dev_dbg(&pdev->pcidev->dev, "i2c CTL_I2C_CNTR_slave_ack_not 0x%04x\n",val);
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
	if (debug & CTL_DEBUG_I2C)
		dev_dbg(&pdev->pcidev->dev, "%s cntr 0x%08x rlen %d\n", __FUNCTION__, val, rlen);

	status = ctl_i2c_check_status(pdev);
	if (status < 0)
	{
		if (debug & CTL_DEBUG_I2C)
			dev_dbg(&pdev->pcidev->dev, "%s status %d\n", __FUNCTION__, status);
		return status;
	}

	val = ctl_reg_read(pdev, CTL_I2C_DATA);
	if (debug & CTL_DEBUG_I2C)
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

static int ctl_i2c_write(CTLDEV *pdev, u8 addr, u8 *buf, u16 len, u32 start, u32 end, unsigned bus, unsigned delay_before_check)
{
	int rc;
	u32 val;
	u32 data = 0;
	u32 shift = 24;
	u32 wlen = len;
	u32 xlen = 0;
	u64 wstart;
	static const unsigned max_backoff = 20;

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
	if (debug & CTL_DEBUG_I2C)
		dev_dbg(&pdev->pcidev->dev, "%s data 0x%08x \n", __FUNCTION__, data);

	val = pdev->phys_chan & CTL_I2C_CNTR_bus_sel_m;
	val |= ctl_i2c_bus_speed_get(pdev) << CTL_I2C_CNTR_freq_400_o;
	val |= (0x1f << CTL_I2C_CNTR_base_timer_o);
	val |= ((xlen - 1) << CTL_I2C_CNTR_xmt_cnt_o) | CTL_I2C_CNTR_write_req_b |
		   CTL_I2C_CNTR_no_restart | start | end;
	wstart = ktime_get_ns();
	ctl_reg_write(pdev, CTL_I2C_CNTR, val);
	if (debug & CTL_DEBUG_I2C)
		dev_dbg(&pdev->pcidev->dev, "%s cntr 0x%08x \n", __FUNCTION__, val);

	if (delay_before_check)
		udelay(delay_before_check);

	rc = ctl_i2c_check_status(pdev);

	if ((rc == -ENXIO) && pdev->modsel_active) {
		/* special optic handling */
		if (delay_before_check <= max_backoff) {
			u64 now = ktime_get_ns();
			unsigned dur = (now - wstart)/1000;
			unsigned since_last = (wstart - pdev->chan_stats[pdev->virt_chan].last_xfer)/1000;
			dev_warn(&pdev->pcidev->dev, "i2c NOACK after %dus dev %d-%04x dbc %d sl %dus\n",
				dur, bus, addr, delay_before_check, since_last);
			/* for condition where NOACK is indicated falsely,
			   we will re-enter and retry the cmd but with a delay before ctl_i2c_check_status
			*/
			pdev->chan_stats[pdev->virt_chan].backoff_cnt++;
			if (pdev->chan_stats[pdev->virt_chan].throttle_min >= CTL_THROTTLE_MIN && pdev->chan_stats[pdev->virt_chan].throttle_min < CTL_THROTTLE_MAX)
				pdev->chan_stats[pdev->virt_chan].throttle_min++;
			rc = ctl_i2c_write(pdev,addr,buf,len,start,end,bus,(delay_before_check+5));
		}
		else {
			dev_dbg(&pdev->pcidev->dev, "%s NOACK final for dev %d-%04x \n", __FUNCTION__,
				bus, addr);
		}
	}

	if (rc == 0)
		rc = wlen;

	return rc;
}

static int ctl_i2c_write_read(CTLDEV *pdev, u8 addr, u8 *wbuf, u16 wlen, u8* rdbuf, u16 rlen)
{
	int rc;
	u32 val;
	u32 data = 0;
	u32 shift = 24;
	u32 xlen = 0;
	u32 start,end,restart;
	u32 rcvlen;

	/* first byte for device address */
	start = CTL_I2C_CNTR_gen_start_b;
	data = (addr << 1) << 24;
	shift -= 8;
	xlen += 1;
	xlen += wlen;
	end = CTL_I2C_CNTR_gen_end_b;

	switch (wlen)
	{
	case 1:
		data |= wbuf[0] << shift;
		shift -= 8;
		break;
	case 2:
		data |= wbuf[0] << shift;
		shift -= 8;
		data |= wbuf[1] << shift;
		shift -= 8;
		break;
	default:
		break;
	}

	/* subsequent read req */
	data |= ((addr << 1) | 1) << shift;
	restart = (xlen - 1) << CTL_I2C_CNTR_restart_b;
	xlen += 1;
	rcvlen = (rlen - 1) << CTL_I2C_CNTR_rcv_cnt_o;

	ctl_reg_write(pdev, CTL_I2C_DATA, data);
	if (debug & CTL_DEBUG_I2C)
		dev_dbg(&pdev->pcidev->dev, "%s data 0x%08x\n", __FUNCTION__, data);

	val = pdev->phys_chan & CTL_I2C_CNTR_bus_sel_m;
	val |= ctl_i2c_bus_speed_get(pdev) << CTL_I2C_CNTR_freq_400_o;
	val |= (0x1f << CTL_I2C_CNTR_base_timer_o);
	val |= ((xlen - 1) << CTL_I2C_CNTR_xmt_cnt_o) | CTL_I2C_CNTR_write_req_b |
		   CTL_I2C_CNTR_read_req_b | rcvlen | restart | start | end;
	ctl_reg_write(pdev, CTL_I2C_CNTR, val);
	if (debug & CTL_DEBUG_I2C)
		dev_dbg(&pdev->pcidev->dev, "%s cntr 0x%08x\n", __FUNCTION__, val);

	rc = ctl_i2c_check_status(pdev);
	if (rc == 0) {
		val = ctl_reg_read(pdev, CTL_I2C_DATA);
		if (debug & CTL_DEBUG_I2C)
			dev_dbg(&pdev->pcidev->dev, "%s data 0x%08x\n", __FUNCTION__, val);
		switch (rlen)
		{
		case 1:
			rdbuf[0] = (u8)val;
			break;
		case 2:
			rdbuf[0] = (u8)(val >> 8);
			rdbuf[1] = (u8)(val);
			break;
		case 3:
			put_unaligned_be24(val, rdbuf);
			break;
		case 4:
			put_unaligned_be32(val, rdbuf);
			break;
		default:
			break;
		}
		rc = 2; /* num messages processed */
	}

	return rc;
}

static int ctl_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	int i;
	CTLDEV *pdev = i2c_get_adapdata(adap);
	int rc;
	u64 start;
	unsigned dur;
	unsigned bus = adap->nr + pdev->virt_chan + 1;

	if ((num == 2) && ((msgs[0].flags & I2C_M_RD) == 0) && (msgs[0].flags & I2C_CLIENT_SCCB) &&
	    (msgs[0].len <= 2) && ((msgs[1].flags & I2C_M_RD) == 1) && (msgs[1].len <= 4) )
	{
		/* if SCCB flag special access for PSU, then combine write of 2 bytes or less and
		 read of 4 bytes or less into one xfer */
		dev_dbg(&pdev->pcidev->dev, "%s msg1/2: dev %d-%04x wlen %d rlen %d\n", __FUNCTION__,
			bus, msgs[i].addr, msgs[0].len, msgs[1].len);
		start = ktime_get_ns();
		rc = ctl_i2c_write_read(pdev,msgs[0].addr,msgs[0].buf,msgs[0].len,msgs[1].buf,msgs[1].len);
		dur = (ktime_get_ns() - start)/1000;
		dev_dbg(&pdev->pcidev->dev, "%s msg1/2: dev %d-%04x time %dus rc=%d\n", __FUNCTION__,
			bus, msgs[i].addr, dur,rc);
		return rc;
	}

	for (i = 0; i < num; i++)
	{
		dev_dbg(&pdev->pcidev->dev, "%s msg%d: dev %d-%04x %s len %d flags=0x%x\n", __FUNCTION__,
				i, bus, msgs[i].addr, (msgs[i].flags & 1) ? "rd" : "wr",
				msgs[i].len,msgs[i].flags);
		start = ktime_get_ns();
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
				/* ackpoll */
				rc = ctl_i2c_write(pdev, msgs[i].addr, wbuf, wrbytes, 1, 1, bus, 0);
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
					rc = ctl_i2c_write(pdev, msgs[i].addr, wbuf, wrbytes, start, end, bus, 0);
					if (rc < 0)
						break;
					start = 0;
					wrbytes -= rc;
					wbuf += rc;
				}
			}
		}
		dur = (ktime_get_ns() - start)/1000;
		dev_dbg(&pdev->pcidev->dev, "%s msg%d: dev %d-%04x time %dus rc=%d\n", __FUNCTION__, i, 
			bus, msgs[i].addr, dur,rc);
		if (rc < 0)
			break;
	}

	if (rc >= 0)
	{
		/* num msgs processed */
		rc = i;
	}

	if (debug & CTL_DEBUG_I2C)
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

	if (debug & CTL_DEBUG_I2C)
		dev_dbg(&pdev->pcidev->dev, "%s chan %d\n", __FUNCTION__, chan);

	pchan = &pdev->ctlv->pchanmap[chan];
	pdev->phys_chan = pchan->phys_chan;
	pdev->virt_chan = chan;
	if (pchan->modsel >= 0) {
		/* phys_chan has modsel */
		pdev->modsel_active=1;
		if (pdev->chan_stats[chan].throttle_min == 0)
			pdev->chan_stats[chan].throttle_min = CTL_THROTTLE_MIN;
		if (pchan->modsel != pdev->current_modsel) {
			unsigned int offset;
			u32 val;
			if (debug & CTL_DEBUG_I2C)
				dev_dbg(&pdev->pcidev->dev, "%s chan %d modsel %d\n", __FUNCTION__, chan,pchan->modsel);
			offset = pchan->modsel < 32 ? CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE : (CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE + 4);
			/* deselect all */
			ctl_reg_write(pdev,CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE,0xffffffff);
			ctl_reg_write(pdev,CTL_A32_VX_IMM_QSFP_MODSEL_N_BASE + 4,0xffffffff);
			val = ~(1 << (pchan->modsel % 32));
			ctl_reg_write(pdev,offset,val);
			msleep(5);
			pdev->current_modsel = pchan->modsel;
		} else {
			/* same device */
			unsigned dur = (ktime_get_ns() - pdev->chan_stats[chan].last_xfer)/1000;
			if (dur < pdev->chan_stats[chan].throttle_min) {
				/* need min time between subsequent cmds */
				pdev->chan_stats[chan].throttle_cnt++;
				dev_dbg(&pdev->pcidev->dev, "%s chan %d dur %d\n", __FUNCTION__,chan,dur);
				udelay(pdev->chan_stats[chan].throttle_min - dur);
			}
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
	
	if (debug & CTL_DEBUG_I2C)
		dev_dbg(&pdev->pcidev->dev, "%s chan %d\n", __FUNCTION__, chan);

	pchan = &pdev->ctlv->pchanmap[chan];
	pdev->phys_chan = 0;
	pdev->modsel_active=0;
	pdev->chan_stats[chan].last_xfer = ktime_get_ns();
	return rc;
}

int ctl_i2c_probe(CTLDEV *pdev)
{
	int rc;

	/* add ctl adapter (i2c host controller) */
	i2c_set_adapdata(&pdev->adapter, pdev);
	pdev->adapter.owner = THIS_MODULE;
	pdev->adapter.class = I2C_CLASS_HWMON;
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
		if (nchans > CTL_MAX_I2C_CHANS){
			dev_err(&pdev->pcidev->dev, "%s nchans %d > %d\n", __FUNCTION__, nchans,CTL_MAX_I2C_CHANS);
			return -EINVAL;
		}

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
			rc = i2c_mux_add_adapter(pdev->ctlmuxcore, 0, i);
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
