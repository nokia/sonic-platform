/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include <stdint.h>
#include "tmFlash.h"
namespace srlinux::platform::spi
{
extern SrlStatus spiWrite16(const tSpiParameters *parms, uint32_t data);
extern SrlStatus spiWrite8(const tSpiParameters *parms, uint32_t data);
extern SrlStatus spiRead8(const tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata);
extern SrlStatus spiWrite8BlockRead(const tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata, uint8_t rdbytes);
extern SrlStatus spiWriteNRead8(const tSpiParameters *parms, uint32_t wrdata, uint8_t wrbytes, uint8_t *rddata);
extern SrlStatus spiWriteBlock(const tSpiParameters *parms, const uint8_t *wrdata, uint32_t wrcount);
extern SrlStatus spiReadBlock(const tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata, uint32_t rdcount);
}
