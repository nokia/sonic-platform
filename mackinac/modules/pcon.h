/* SPDX-License-Identifier: GPL-2.0-or-later */

 #include <linux/kernel.h>
 
#define PCON_VERSION_ID_REG                      0x00
#define S_VERSION_ID_REG_REVISION           0
#define M_VERSION_ID_REG_REVISION           0xff
#define S_VERSION_ID_REG_VERSION            8
#define M_VERSION_ID_REG_VERSION            0xff00

#define PCON_IMBV_VOLT_VALUE_REG                 0x02
#define S_IMBV_VOLT_VALUE_REG_IMB_VOLT      0
#define M_IMBV_VOLT_VALUE_REG_IMB_VOLT      0xff
#define S_IMBV_VOLT_VALUE_REG_IMB           8
#define M_IMBV_VOLT_VALUE_REG_IMB           0x300

#define PCON_IMBV_ERROR_REG                      0x04
#define S_IMBV_ERROR_REG_IMBV_UV            0
#define M_IMBV_ERROR_REG_IMBV_UV            0x1
#define S_IMBV_ERROR_REG_IMBV_OV            1
#define M_IMBV_ERROR_REG_IMBV_OV            0x2

#define PCON_CHANNEL_SELECT_REG                  0x06
#define S_CHANNEL_SELECT_REG_INV_CH_SEL     0
#define M_CHANNEL_SELECT_REG_INV_CH_SEL     0xff
#define S_CHANNEL_SELECT_REG_CH_SEL         8
#define M_CHANNEL_SELECT_REG_CH_SEL         0xff00

#define PCON_SPI_SELECT_REG                      0x08 
#define S_SPI_SELECT_REG_SPI_I2C_SELECT     0
#define M_SPI_SELECT_REG_SPI_I2C_SELECT     0x1
#define S_SPI_SELECT_REG_EVENT_CFG_SELECT   1
#define M_SPI_SELECT_REG_EVENT_CFG_SELECT   0x2

#define PCON_UP_TIMER_LSW                        0x0a
#define PCON_UP_TIMER_MSW                        0x0c

#define PCON_VOLT_SET_INV_REG                    0x10
#define PCON_VOLT_SET_REG                        0x12
#define PCON_UNDER_VOLT_SET_INV_REG              0x14
#define PCON_UNDER_VOLT_SET_REG                  0x16
#define PCON_OVER_VOLT_SET_INV_REG               0x18
#define PCON_OVER_VOLT_SET_REG                   0x1A 
#define PCON_MEASURED_VOLT_REG                   0x1C
#define PCON_MEASURED_CURRENT_REG                0x1E
#define PCON_CURRENT_MULTIPLIER_REG              0x20
#define PCON_START_TIME_REG                      0x22
#define PCON_VOLT_RAMP_REG                       0x24 
#define PCON_MAX_CURRENT_REG                     0x28
#define PCON_PHASE_OFFSET_REG                    0x2A
#define PCON_VOLT_TRIM_ALLOWANCE_REG             0x2C
#define PCON_B0_COEFF_REG                        0x2E
#define PCON_B1_COEFF_REG                        0x30
#define PCON_B2_COEFF_REG                        0x32
#define PCON_A1_COEFF_REG                        0x34
#define PCON_A2_COEFF_REG                        0x36
#define PCON_MISC_REG                            0x3A

#define S_CURRENT_MULTIPLIER_REG_DEN        0
#define M_CURRENT_MULTIPLIER_REG_DEN        0xff
#define S_CURRENT_MULTIPLIER_REG_NUM        8
#define M_CURRENT_MULTIPLIER_REG_NUM        0xff00

#define S_A1_COEFF_REG_SIGN                 11
#define M_A1_COEFF_REG_SIGN                 0x800
#define S_A1_COEFF_REG_VALUE                0
#define M_A1_COEFF_REG_VALUE                0x7FF

#define S_A2_COEFF_REG_SIGN                 11
#define M_A2_COEFF_REG_SIGN                 0x800
#define S_A2_COEFF_REG_VALUE                0
#define M_A2_COEFF_REG_VALUE                0x7FF

#define S_MISC_REG_MASTER                   0
#define M_MISC_REG_MASTER                   0x1
#define S_MISC_REG_CH_ENABLE                1
#define M_MISC_REG_CH_ENABLE                0x2
#define S_MISC_REG_ERROR_GAIN               2
#define M_MISC_REG_ERROR_GAIN               0xC
#define S_MISC_REG_SLAVE_TO                 8
#define M_MISC_REG_SLAVE_TO                 0xff00

#define PCON_MAX_CHANNELS_PER_DEV           42
#define PCONM_MAX_CHANNELS_PER_DEV          16
