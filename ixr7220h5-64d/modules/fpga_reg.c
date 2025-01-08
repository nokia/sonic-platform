// SPDX-License-Identifier: GPL-2.0-only
/*
* FPGA driver
* 
* Copyright (C) 2024 Nokia Corporation.
* Copyright (C) 2024 Delta Networks, Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include "fpga.h"
#include "fpga_attr.h"
#include "fpga_reg.h"

const sys_fpga_reg_st sys_fpga_reg_table[FPGA_REG_TAB_LEN] = {
    { "scratch", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x00, 0, 32 },
    { "code_ver", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x04, 0, 8 },
    { "code_day", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x08, 8, 8 },
    { "code_month", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x08, 16, 8 },
    { "code_year", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x08, 24, 8 },
    { "board_ver", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x0C, 0, 3 },
    { "sys_pwr", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x1c, 2, 1 },
    { "psu1_pres", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x20, 16, 1 },
    { "psu2_pres", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x20, 20, 1 },
    { "psu1_ok", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x20, 17, 1 },
    { "psu2_ok", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x20, 21, 1 },
    { "led_sys", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x84, 0, 3 },
    { "led_fan", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x88, 0, 3 },
    { "led_psu1", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x8c, 0, 3 },
    { "led_psu2", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0x90, 0, 3 },
    { "fan1_led", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0xa0, 12, 3 },
    { "fan2_led", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0xa0, 8, 3 },
    { "fan3_led", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0xa0, 4, 3 },
    { "fan4_led", NULL, I2C_DEV_ATTR_SHOW_DEFAULT, I2C_DEV_ATTR_STORE_DEFAULT, 0xa0, 0, 3 },
};
