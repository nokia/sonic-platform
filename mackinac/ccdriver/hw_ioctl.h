/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include <stdint.h>
typedef enum
{
    IOCTRL_NUM_MIN = 0,
    IOCTRL_NUM_BASE = IOCTRL_NUM_MIN,
    IOCTRL_NUM_LIF_MIN,
    IOCTRL_NUM_LIF_1 = IOCTRL_NUM_LIF_MIN,
    IOCTRL_NUM_LIF_2,
    IOCTRL_NUM_LIF_3,
    IOCTRL_NUM_LIF_4,
    IOCTRL_NUM_LIF_5,
    IOCTRL_NUM_LIF_6,
    IOCTRL_NUM_LIF_MAX = IOCTRL_NUM_LIF_6,
    MAX_IOCTRL_NUM_LIF,
    IOCTRL_NUM_SFM_MIN = MAX_IOCTRL_NUM_LIF,
    IOCTRL_NUM_SFM_1 = IOCTRL_NUM_SFM_MIN,
    IOCTRL_NUM_SFM_2,
    IOCTRL_NUM_SFM_3,
    IOCTRL_NUM_SFM_4,
    IOCTRL_NUM_SFM_5,
    IOCTRL_NUM_SFM_6,
    IOCTRL_NUM_SFM_7,
    IOCTRL_NUM_SFM_8,
    IOCTRL_NUM_SFM_9,
    IOCTRL_NUM_SFM_10,
    IOCTRL_NUM_SFM_11,
    IOCTRL_NUM_SFM_12,
    IOCTRL_NUM_SFM_13,
    IOCTRL_NUM_SFM_14,
    IOCTRL_NUM_SFM_15,
    IOCTRL_NUM_SFM_16,
    IOCTRL_NUM_SFM_MAX = IOCTRL_NUM_SFM_16,
    MAX_IOCTRL_NUM_SFM,
    IOCTRL_NUM_FPI_MIN = MAX_IOCTRL_NUM_SFM,
    IOCTRL_NUM_FPI_1 = IOCTRL_NUM_FPI_MIN,
    IOCTRL_NUM_FPI_2,
    IOCTRL_NUM_FPI_MAX = IOCTRL_NUM_FPI_2,
    MAX_IOCTRL_NUM_FPI,
    MAX_IOCTRL_NUM = MAX_IOCTRL_NUM_FPI
 } eIoctrlNum;
