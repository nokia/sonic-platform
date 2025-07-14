/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include "platform_types.h"
#include "tmSpiDefs.h"
extern "C" {
extern SrlStatus getBitfile(char *filename, const char *fpganame, char **bitfile, tPromHeader *pHeader, uint32_t *pVersion);
}
