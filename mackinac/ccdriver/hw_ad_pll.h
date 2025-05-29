/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

extern "C" {
typedef struct tAdPllConfig
{
    uint16_t offset;
    uint8_t value;
} __attribute__ ((packed)) tAdPllConfig;
static inline bool PLL_EOT(const tAdPllConfig * cfg)
{
    return ((cfg->offset == 0xffff) && (cfg->value == 0xff));
}
}
