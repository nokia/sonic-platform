/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include "tmI2C.h"
#include "tmSpiDefs.h"
#include "hw_ad_pll.h"
#include "idt_sets_panos_misc.h"
#include "idt8a3xxxxDefs.h"
#include "idtfw.h"
namespace srlinux::platform::bsp
{
typedef tUint32 tIdt8a3xxxxDevIndex;
typedef tUint32 tIdt8a3xxxxDevIndex;
typedef struct
{
    const char * deviceName;
    tUint32 speed;
} tLinuxSpiParms;
typedef enum eIdt8a3xxxxDeviceVariants
{
    idt8a34001DeviceVariant = 0,
    idt8a34012DeviceVariant,
    idt8a34045DeviceVariant,
    idt8a35003DeviceVariant,
} eIdt8a3xxxxDeviceVariants;
typedef enum eIdt8a3xxxxAddressType
{
    idt8a3xxxxI2cAddressType = 0,
    idt8a3xxxxSpiAddressType,
    idt8a3xxxxCustomSpiAddressType,
    idt8a3xxxxHostSpiAddressType,
} eIdt8a3xxxxAddressType;
typedef enum eOtdcMeasStatusType
{
   OTDC_MEASUREMENT_SUCCESS = 0,
   OTDC_MEASUREMENT_FAILURE,
   OTDC_MEASUREMENT_TIMEDOUT,
   OTDC_MEASUREMENT_NOT_READY,
   OTDC_MEASUREMENT_INPROGRESS,
} eOtdcMeasStatusType;
extern eIdt8a3xxxxInputs idt8a3xxxxNumInputsForDev[10];
extern tIdt8a3xxxxDevIndex idt8a3xxxxNumUsedDevices;
extern eIdt8a3xxxxOutputs idt8a3xxxxNumOutputsForDev[10];
extern eIdt8a3xxxxTods idt8a3xxxxNumTodsForDev[10];
extern eIdt8a3xxxxDplls idt8a3xxxxNumDpllsForDev[10];
extern tBoolean idt8a3xxxxHighPrecisionPhase[10];
typedef struct tIdt8a3xxxxI2cParms
{
    eI2C_ctrlr i2cCtrlr;
    eI2C_type i2cType;
} tIdt8a3xxxxI2cParms;
typedef struct tIdt8a3xxxxCustomReadWriteParms
{
    tUint16 burstLen;
    void *handle;
    tUint8 (*customReadFn) (void * handle, tUint16 regOffset);
    void (*customWriteFn) (void * handle, tUint16 regOffset, tUint8 data);
    void (*customBurstWriteFn) (void * handle, const tUint8 * data, tUint32 count);
    tBoolean (*customRemovedFn) (void * handle);
} tIdt8a3xxxxCustomReadWriteParms;
typedef struct tIdt8a3xxxxAddressInfo
{
    eIdt8a3xxxxAddressType addrType;
    union
    {
        tIdt8a3xxxxI2cParms i2cParms;
        tSpiParameters spiParms;
        tLinuxSpiParms linuxSpiParms;
        tIdt8a3xxxxCustomReadWriteParms customParms;
    };
} tIdt8a3xxxxAddressInfo;
typedef tUint64 eTimHwRefSrcs;
typedef struct tIdt8a3xxxxPerInputConfigInfo
{
    eIdt8a3xxxxInputTypes inputType;
    tUint64 inFreqM;
    tUint16 inFreqN;
    tUint16 inDivider;
    eTimHwRefSrcs t0HwRef;
    const char * inputName;
} tIdt8a3xxxxPerInputConfigInfo;
typedef struct tIdt8a3xxxxInputConfigInfo
{
    tIdt8a3xxxxPerInputConfigInfo perInputInfo[idt8a3xxxxNumInput];
} tIdt8a3xxxxInputConfigInfo;
typedef struct tIdt8a3xxxxPerDpllConfigInfo
{
    eIdt8a3xxxxDplls comboPrimary;
    eIdt8a3xxxxDplls comboSecondary;
    tUint64 fodFreqM;
    tUint16 fodFreqN;
    tUint8 pllMode;
    const char * dpllName;
} tIdt8a3xxxxPerDpllConfigInfo;
typedef struct tIdt8a3xxxxDpllConfigInfo
{
    union
    {
        eIdt8a3xxxxDplls t0Dpll;
        eIdt8a3xxxxDplls mdaMainDpll;
    };
    eIdt8a3xxxxDplls t4Dpll;
    eIdt8a3xxxxDplls esDpll;
    eIdt8a3xxxxDplls gnssDpll;
    eIdt8a3xxxxDplls localZDpll;
    eIdt8a3xxxxDplls ts1ZDpll;
    tIdt8a3xxxxPerDpllConfigInfo perDpllInfo[idt8a3xxxxNumDpll];
} tIdt8a3xxxxDpllConfigInfo;
typedef struct tIdt8a3xxxxPerOutputConfigInfo
{
    tUint32 divider;
    tUint8 padMode;
    const char * outputName;
} tIdt8a3xxxxPerOutputConfigInfo;
typedef struct tIdt8a3xxxxOutputConfigInfo
{
    eIdt8a3xxxxOutputs bitsOutput;
    tUint32 bitsDivForT1;
    tUint32 bitsDivForE1;
    tUint32 bitsDivForSq;
    eIdt8a3xxxxDplls output8Dpll;
    eIdt8a3xxxxDplls output11Dpll;
    tIdt8a3xxxxPerOutputConfigInfo perOutputInfo[idt8a3xxxxNumOutput];
} tIdt8a3xxxxOutputConfigInfo;
typedef struct tIdt8a3xxxxBroadsyncOtdcMeasureInfo
{
    tBoolean valid;
    eIdt8a3xxxxOutputTdcs otdcIdx;
} tIdt8a3xxxxBroadsyncOtdcMeasureInfo;
typedef struct tIdt8a3xxxxDeviceConfigInfo
{
    tIdt8a3xxxxAddressInfo addrInfo;
    CtlFpgaId fpga_id;
    eIdt8a3xxxxDeviceVariants deviceVariant;
    const char * deviceName;
    const struct tAdPllConfig * configFile;
    const struct tIdt8aFirmwareDesc *
                                 firmware;
    tBoolean emptyPromOnly;
    tBoolean eraseProm;
    tUint8 eepromBlockI2cAddr[2];
    union
    {
        tIdt8a3xxxxBroadsyncOtdcMeasureInfo
                                     broadscynOtdc;
        tIdt8a3xxxxBroadsyncOtdcMeasureInfo
                                     ppsOtdc;
    };
    tIdt8a3xxxxInputConfigInfo inputConfig;
    tIdt8a3xxxxDpllConfigInfo dpllConfig;
    tIdt8a3xxxxOutputConfigInfo outputConfig;
} tIdt8a3xxxxDeviceConfigInfo;
typedef tUint8 tIdt8a3xxxxInputPriorityTable[idt8a3xxxxNumInput];
extern const tIdt8a3xxxxDeviceConfigInfo *idt8a3xxxxCurrentDeviceConfigInfo[10];
extern tUint8 idt8a3xxxxDpllGetState(tIdt8a3xxxxDevIndex devIdx, eIdt8a3xxxxDplls dpll);
extern std::string idt8a3xxxxDpllToString(tIdt8a3xxxxDevIndex devIdx, eIdt8a3xxxxDplls dpll, tBoolean detail);
extern std::string idt8a3xxxxImageVersionToString(tIdt8a3xxxxDevIndex devIdx);
void idt8a3xxxxDumpDevs(void);
extern void idt8a3xxxxDumpVersion(tIdt8a3xxxxDevIndex devIdx);
extern std::string idt8a3xxxxVersionToString(tIdt8a3xxxxDevIndex devIdx);
extern void idt8a3xxxxBringupByDownload(tIdt8a3xxxxDevIndex devIdx);
extern tUint16 idt8a3xxxxGeneralGetProductId(tIdt8a3xxxxDevIndex devIdx);
extern tIdt8a3xxxxDevIndex idt8a3xxxxInitDevInfo(tIdt8a3xxxxDevIndex devIdx,
                                                 const tIdt8a3xxxxDeviceConfigInfo * pDeviceInfo);
extern tUint8 idt8a3xxxxGetReg8(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset);
extern tUint16 idt8a3xxxxGetReg16(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset);
extern tUint32 idt8a3xxxxGetReg32(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset);
extern void idt8a3xxxxSetReg8(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint8 regVal);
extern void idt8a3xxxxSetReg16(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint16 regVal);
extern void idt8a3xxxxSetReg32(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint32 regVal);
extern void idt8a3xxxxSetReg40(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint64 regVal);
extern void idt8a3xxxxSetReg48(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint64 regVal);
extern void idt8a3xxxxEepromSetCurrentBlock(tIdt8a3xxxxDevIndex devIdx, tUint32 block);
extern tStatus idt8a3xxxxProgramEepromFromFile(tIdt8a3xxxxDevIndex devIdx, char *filename, tBoolean verbose);
extern tBoolean idt8a3xxxxGetRegIsTrigger(tUint16 regOffset);
extern void idt8a3xxxxInitRegIsTriggerBitmap(void);
extern std::string idt8a3xxxxInputsToString(tIdt8a3xxxxDevIndex devIdx, tBoolean detail);
}
