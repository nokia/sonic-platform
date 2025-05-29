/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include <string>
#include "hw_ioctl.h"
#include "platform_types.h"
typedef enum
{
    HW_INSTANCE_CARD,
    HW_INSTANCE_SFM,
    HW_INSTANCE_MDA,
} HwInstanceId;
typedef struct
{
    CardType card_type;
} HwInstanceCard;
typedef struct
{
    HwInstanceId id;
    union
    {
        HwInstanceCard card;
    };
} HwInstance;
std::string HwInstanceToString(HwInstance instance);
