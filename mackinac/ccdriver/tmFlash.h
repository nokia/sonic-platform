/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

typedef union tFlashDeviceId
{
    uint32_t u32;
    struct sFlashDeviceId
    {
        uint8_t manufId;
        uint8_t memType;
        uint8_t memCapacity;
        uint8_t cfiLength;
    } s;
} tFlashDeviceId;
namespace srlinux::platform::spi
{
}
