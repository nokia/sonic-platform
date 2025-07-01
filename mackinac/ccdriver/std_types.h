/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include <stdbool.h>
#include <stdint.h>
extern "C" {
typedef signed char tInt8;
typedef unsigned char tUint8;
typedef short tInt16;
typedef unsigned short tUint16;
typedef int tInt32;
typedef unsigned int tUint32;
typedef int64_t tInt64 __attribute__((aligned(8)));
typedef uint64_t tUint64 __attribute__((aligned(8)));
typedef int64_t tInt64_a4 __attribute__((aligned(4)));
typedef uint64_t tUint64_a4 __attribute__((aligned(4)));
typedef unsigned char tBoolean;
typedef tUint32 tHwCardType;
typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef uint8_t UINT8;
typedef uint8_t UCHAR;
typedef uint16_t UINT16;
typedef uint16_t USHORT;
typedef uint32_t UINT32;
typedef uint32_t UINT;
typedef uint64_t ULONG;
}
