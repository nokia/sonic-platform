/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include <stdbool.h>
#include "fpga_if.h"
#include "platform_types.h"
#include "i2c_base.h"
extern "C" {
typedef uint32_t tI2cStatus;
typedef enum eI2C_class {
    I2C_CLASS_UNKNOWN = 0,
    NUM_I2C_CLASSES
} eI2C_class;
typedef struct I2CFpgaCtrlrDeviceParams
{
    uint8_t channel;
    uint8_t device;
    uint32_t blksz;
    uint32_t maxsz;
    uint8_t speed : 1;
    uint8_t devclass:7;
} I2CFpgaCtrlrDeviceParams;
typedef struct I2CCtrlr
{
    CtlFpgaId fpga_id;
    uint8_t is_remote;
    SlotNumType hw_slot;
} I2CCtrlr;
}
