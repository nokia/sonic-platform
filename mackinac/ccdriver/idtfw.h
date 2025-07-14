/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include "std_types.h"
extern "C" {
typedef struct tIdt8aFirmware
{
    tUint16 offset;
    tUint16 count;
    const tUint8 * data;
} tIdt8aFirmware;
typedef struct tIdt8aFirmwareDesc
{
    const char * name;
    tUint8 major;
    tUint8 minor;
    tUint8 hotfix;
    const tIdt8aFirmware *firmware;
} tIdt8aFirmwareDesc;
static inline tBoolean idt8aFirmwareEot(const tIdt8aFirmware * ptr)
{
    return ((ptr->offset == 0) && (ptr->data == 0));
}
}
