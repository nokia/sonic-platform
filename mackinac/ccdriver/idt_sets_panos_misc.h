/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include <stdint.h>
#include <cassert>
namespace srlinux::platform::bsp
{
typedef signed char tInt8;
typedef unsigned char tUint8;
typedef short tInt16;
typedef unsigned short tUint16;
typedef int tInt32;
typedef unsigned int tUint32;
typedef unsigned long ULONG;
typedef int64_t tInt64;
typedef uint64_t tUint64;
typedef bool tBoolean;
typedef int tStatus;
typedef unsigned int tPortId;
typedef unsigned int tMdaId;
typedef unsigned int taskid_t;
typedef int (*FUNCPTR) (...);
typedef unsigned int tSlot;
typedef unsigned int tHwIndex;
typedef unsigned int eI2C_ctrlr;
typedef unsigned int eI2C_type;
}
