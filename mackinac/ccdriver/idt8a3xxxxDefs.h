/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

namespace srlinux::platform::bsp
{
typedef enum eIdt8a3xxxxInputs
{
    idt8a3xxxxInput0 = 0,
    idt8a3xxxxInput1,
    idt8a3xxxxInput2,
    idt8a3xxxxInput3,
    idt8a3xxxxInput4,
    idt8a3xxxxInput5,
    idt8a3xxxxInput6,
    idt8a3xxxxInput7,
    idt8a3xxxxInput8,
    idt8a3xxxxInput9,
    idt8a3xxxxInput10,
    idt8a3xxxxInput11,
    idt8a3xxxxInput12,
    idt8a3xxxxInput13,
    idt8a3xxxxInput14,
    idt8a3xxxxInput15,
    idt8a3xxxxNumInput,
    idt8a3xxxxInvalidInput,
} eIdt8a3xxxxInputs;
typedef enum eIdt8a3xxxxInputTypes
{
    idt8a3xxxxDifferentialInput,
    idt8a3xxxxSingleEndedInput,
    idt8a3xxxxGpioSingleInput,
    idt8a3xxxxUnusedInput,
} eIdt8a3xxxxInputTypes;
typedef enum eIdt8a3xxxxDplls
{
    idt8a3xxxxDpll0 = 0,
    idt8a3xxxxDpll1,
    idt8a3xxxxDpll2,
    idt8a3xxxxDpll3,
    idt8a3xxxxDpll4,
    idt8a3xxxxDpll5,
    idt8a3xxxxDpll6,
    idt8a3xxxxDpll7,
    idt8a3xxxxNumDpll,
    idt8a3xxxxSystemDpll = 8,
    idt8a3xxxxInvalidDpll,
} eIdt8a3xxxxDplls;
typedef enum eIdt8a3xxxxOutputs
{
    idt8a3xxxxOutput0 = 0,
    idt8a3xxxxOutput1,
    idt8a3xxxxOutput2,
    idt8a3xxxxOutput3,
    idt8a3xxxxOutput4,
    idt8a3xxxxOutput5,
    idt8a3xxxxOutput6,
    idt8a3xxxxOutput7,
    idt8a3xxxxOutput8,
    idt8a3xxxxOutput9,
    idt8a3xxxxOutput10,
    idt8a3xxxxOutput11,
    idt8a3xxxxNumOutput,
    idt8a3xxxxInvalidOutput,
} eIdt8a3xxxxOutputs;
typedef enum eIdt8a3xxxxScratchRegs
{
    idt8a3xxxxScratch0 = 0,
    idt8a3xxxxScratch1,
    idt8a3xxxxScratch2,
    idt8a3xxxxScratch3,
    idt8a3xxxxNumScratchReg,
} eIdt8a3xxxxScratchRegs;
typedef enum eIdt8a3xxxxTods
{
    idt8a3xxxxTod0 = 0,
    idt8a3xxxxTod1,
    idt8a3xxxxTod2,
    idt8a3xxxxTod3,
    idt8a3xxxxNumTod,
} eIdt8a3xxxxTods;
typedef enum eIdt8a3xxxxOutputTdcs
{
    idt8a3xxxxOutputTdc0 = 0,
    idt8a3xxxxOutputTdc1,
    idt8a3xxxxOutputTdc2,
    idt8a3xxxxOutputTdc3,
    idt8a3xxxxNumOutputTdc,
} eIdt8a3xxxxOutputTdcs;
}
