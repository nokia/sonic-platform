/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include "fpga_if.h"
#include "platform_types.h"
extern "C" {
typedef uint8_t tSpiType;
typedef struct tSpiParameters
{
    CtlFpgaId fpga_id;
    HwSlotNumType hwSlot;
    uint8_t timer;
    uint8_t channel : 5;
    uint8_t edge : 1;
    uint8_t numProms : 2;
    tSpiType type : 1;
    uint8_t spictrlNum;
    uint8_t ioctrlNum;
} tSpiParameters;
typedef struct tImageSelect
{
    union
    {
        uint8_t data8;
        struct
        {
            uint8_t promIndex :3;
            uint8_t imageIndex :2;
            uint8_t spareBits :3;
        } info;
    } u;
} tImageSelect;
typedef struct tPromHeader
{
    uint32_t size;
    union
    {
        uint32_t data32;
        struct
        {
            tImageSelect imageSelect;
            uint8_t spareByte1;
            uint8_t spareByte2;
            uint8_t version;
        } info;
    } u;
} tPromHeader;
typedef struct
{
    uint16_t regaddr;
    uint8_t value;
} tSpiRegData;
typedef uint32_t tSpiProgramMask;
}
