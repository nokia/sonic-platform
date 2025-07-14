/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "replacements.h"
#include "fpgaImageUtils.h"
#include "platform_hw_info.h"
#include "tmSPI.h"
#include "tmSpiDefs.h"
#include "idt_sets_panos_misc.h"
#include "idt8a3xxxx.h"
#include "idt8a3xxxxDefs.h"
using namespace srlinux::platform::spi;
namespace srlinux::platform::bsp
{
typedef struct tIdt8a3xxxxBurstBuf
{
    tUint32 length;
    tUint8 * buf;
} tIdt8a3xxxxBurstBuf;
 tIdt8a3xxxxDevIndex idt8a3xxxxNumUsedDevices = 0;
 eIdt8a3xxxxInputs idt8a3xxxxNumInputsForDev[10] = { idt8a3xxxxNumInput};
 eIdt8a3xxxxDplls idt8a3xxxxNumDpllsForDev[10] = { idt8a3xxxxNumDpll};
 eIdt8a3xxxxOutputs idt8a3xxxxNumOutputsForDev[10] = { idt8a3xxxxNumOutput};
 eIdt8a3xxxxTods idt8a3xxxxNumTodsForDev[10] = { idt8a3xxxxNumTod};
 int * idt8a3xxxxRegSem[10] = { NULL};
 tBoolean idt8a3xxxx_debug[10] = { 0};
 tUint32 idt8a3xxxxCurrentEepromBlock[10] = { 0};
 tUint8 idt8a3xxxxEepromLoadStatus[10] = { 0};
 tUint16 idt8a3xxxxExpectedProductId[10] = { 0};
 tIdt8a3xxxxBurstBuf idt8a3xxxxBurstBuf[10] = { { 0 } };
 const tIdt8a3xxxxDeviceConfigInfo *idt8a3xxxxCurrentDeviceConfigInfo[10] = { NULL};
 tUint32 idt8a3xxxx_trigger_register_bitmap[128] = { 0};
 tBoolean idt8a3xxxx_trigger_register_bitmap_initialized = 0;
 tUint16 idt8a3xxxx_trigger_registers[] =
    { 0xC160, 0xC161, 0xC164, 0xC165, 0xC166, 0xC167, 0xC168, 0xC169, 0xC16C, 0xC16D, 0xC192, 0xC19B, 0xC1AD, 0xC1BD, 0xC1CD, 0xC1DD,
      0xC20D, 0xC21D, 0xC22D, 0xC23D, 0xC24D, 0xC25D, 0xC26D, 0xC28D, 0xC29D, 0xC2AD, 0xC2BD, 0xC2CD, 0xC2DD, 0xC2EB, 0xC2F7,
      0xC30B, 0xC317, 0xC323, 0xC32F, 0xC33B, 0xC347, 0xC353, 0xC35F, 0xC36B, 0xC377, 0xC38B, 0xC397, 0xC3A3, 0xC3AF, 0xC3E7,
      0xC437, 0xC46F, 0xC4B7, 0xC4EF,
      0xC537, 0xC56F, 0xC5B7, 0xC5D4,
      0xC600, 0xC601, 0xC602, 0xC603, 0xC605, 0xC607, 0xC608, 0xC609, 0xC60B, 0xC60D, 0xC60E, 0xC60F, 0xC611, 0xC613, 0xC618, 0xC619,
      0xC61B, 0xC623, 0xC627, 0xC62D, 0xC635, 0xC637, 0xC639, 0xC63A, 0xC63B, 0xC63C, 0xC63D, 0xC63E, 0xC63F, 0xC641, 0xC643, 0xC644,
      0xC645, 0xC647, 0xC649, 0xC64A, 0xC64B, 0xC64D, 0xC64F, 0xC654, 0xC655, 0xC657, 0xC65F, 0xC663, 0xC669, 0xC671, 0xC673, 0xC675,
      0xC676, 0xC677, 0xC680, 0xC681, 0xC682, 0xC683, 0xC685, 0xC687, 0xC688, 0xC689, 0xC68B, 0xC68D, 0xC68E, 0xC68F, 0xC691, 0xC693,
      0xC698, 0xC699, 0xC69B, 0xC6A3, 0xC6A7, 0xC6AD, 0xC6B5, 0xC6B7, 0xC6B9, 0xC6BA, 0xC6BB, 0xC6BC, 0xC6BD, 0xC6BE, 0xC6BF, 0xC6C1,
      0xC6C3, 0xC6C4, 0xC6C5, 0xC6C7, 0xC6C9, 0xC6CA, 0xC6CB, 0xC6CD, 0xC6CF, 0xC6D4, 0xC6D5, 0xC6D7, 0xC6DF, 0xC6E3, 0xC6E9, 0xC6F1,
      0xC6F3, 0xC6F5, 0xC6F6, 0xC6F7,
      0xC700, 0xC701, 0xC702, 0xC703, 0xC705, 0xC707, 0xC708, 0xC709, 0xC70B, 0xC70D, 0xC70E, 0xC70F, 0xC711, 0xC713, 0xC718, 0xC719,
      0xC71B, 0xC723, 0xC727, 0xC72D, 0xC735, 0xC737, 0xC739, 0xC73A, 0xC73B, 0xC73C, 0xC73D, 0xC73E, 0xC73F, 0xC741, 0xC743, 0xC744,
      0xC745, 0xC747, 0xC749, 0xC74A, 0xC74B, 0xC74D, 0xC74F, 0xC754, 0xC755, 0xC757, 0xC75F, 0xC763, 0xC769, 0xC771, 0xC773, 0xC775,
      0xC776, 0xC777, 0xC780, 0xC781, 0xC782, 0xC783, 0xC785, 0xC787, 0xC788, 0xC789, 0xC78B, 0xC78D, 0xC78E, 0xC78F, 0xC791, 0xC793,
      0xC798, 0xC799, 0xC79B, 0xC7A3, 0xC7A7, 0xC7AD, 0xC7B5, 0xC7B7, 0xC7B9, 0xC7BA, 0xC7BB, 0xC7BC, 0xC7BD, 0xC7BE, 0xC7BF, 0xC7C1,
      0xC7C3, 0xC7C4, 0xC7C5, 0xC7C7, 0xC7C9, 0xC7CA, 0xC7CB, 0xC7CD, 0xC7CF, 0xC7D4, 0xC7D5, 0xC7D7, 0xC7DF, 0xC7E3, 0xC7E9, 0xC7F1,
      0xC7F3, 0xC7F5, 0xC7F6, 0xC7F7,
      0xC800, 0xC801, 0xC802, 0xC805, 0xC807, 0xC808, 0xC809, 0xC80B, 0xC80D, 0xC80E, 0xC80F, 0xC811, 0xC813, 0xC815, 0xC816, 0xC81B,
      0xC81F, 0xC823, 0xC827, 0xC82B, 0xC82F, 0xC833, 0xC837, 0xC83D, 0xC845, 0xC84D, 0xC855, 0xC85D, 0xC865, 0xC86D, 0xC875, 0xC887,
      0xC88F, 0xC897, 0xC89F, 0xC8A7, 0xC8AF, 0xC8B7, 0xC8BF, 0xC8C0, 0xC8D2, 0xC8E4, 0xC8F6,
      0xC910, 0xC922, 0xC934, 0xC946, 0xC958, 0xC96A, 0xC990, 0xC9A2, 0xC9B4, 0xC9C6, 0xC9D8, 0xC9EA,
      0xCA10, 0xCA12, 0xCA13, 0xCA17, 0xCA1B, 0xCA1C, 0xCA1D, 0xCA23, 0xCA27, 0xCA2B, 0xCA2C, 0xCA2D, 0xCA33, 0xCA37, 0xCA3B, 0xCA3C,
      0xCA3D, 0xCA43, 0xCA47, 0xCA4B, 0xCA4C, 0xCA4D, 0xCA53, 0xCA57, 0xCA5B, 0xCA5C, 0xCA5D, 0xCA63, 0xCA67, 0xCA6B, 0xCA6C, 0xCA6D,
      0xCA73, 0xCA83, 0xCA87, 0xCA88, 0xCA89, 0xCA8F, 0xCA93, 0xCA97, 0xCA98, 0xCA99, 0xCA9F, 0xCAA3, 0xCAA7, 0xCAA8, 0xCAA9, 0xCAAF,
      0xCAB3, 0xCAB7, 0xCAB8, 0xCAB9, 0xCABF, 0xCAC3, 0xCAC7, 0xCAC8, 0xCAC9, 0xCACF, 0xCAD3, 0xCAD7, 0xCAD8, 0xCAD9, 0xCADF, 0xCAE8,
      0xCB04, 0xCB0C, 0xCB14, 0xCB1C, 0xCB24, 0xCB2C, 0xCB34, 0xCB3C, 0xCB45, 0xCB4D, 0xCB55, 0xCB5D, 0xCB65, 0xCB6D, 0xCB75, 0xCB85,
      0xCB8D, 0xCB95, 0xCB9D, 0xCBA5, 0xCBAD, 0xCBB5, 0xCBBD, 0xCBC5, 0xCBCB, 0xCBCC, 0xCBCE, 0xCBD0, 0xCBD2,
      0xCC0F, 0xCC1F, 0xCC2F, 0xCC3F, 0xCC4E, 0xCC5E, 0xCC6E, 0xCC8E, 0xCC9E, 0xCCAE, 0xCCBE, 0xCCCE, 0xCCD4,
      0xCD06, 0xCD0E, 0xCD16, 0xCD1E, 0xCD25, 0xCD82, 0xCD86, 0xCD8A, 0xCD8E, 0xCD92, 0xCD96, 0xCD9A, 0xCD9E,
      0xCE04, 0xCE0A, 0xCE10, 0xCE16, 0xCE1C, 0xCE22, 0xCE28, 0xCE2E, 0xCE34, 0xCE3A, 0xCE40, 0xCE46, 0xCE4C, 0xCE52, 0xCE58, 0xCE5E,
      0xCF4E, 0xCF5F, 0xCF67, 0xCF6D,
    };
 tBoolean idt8a3xxxxEepromIsEmpty(tIdt8a3xxxxDevIndex devIdx);
 void idt8a3xxxxSetRegIsTrigger(tUint16 regOffset)
{
    if ((regOffset & 0xF000) == 0xC000)
    {
        tUint16 bitInTable = regOffset & 0x0FFF;
        tUint16 tblEntry = bitInTable / 32;
        tUint16 bitInEntry = bitInTable & 0x1f;
        tUint32 bitMask = 1 << bitInEntry;
        idt8a3xxxx_trigger_register_bitmap[tblEntry] |= bitMask;
    }
}
 tBoolean idt8a3xxxxGetRegIsTrigger(tUint16 regOffset)
{
    if (((regOffset | 0x8000) & 0xF000) == 0xC000)
    {
        tUint16 bitInTable = regOffset & 0x0FFF;
        tUint16 tblEntry = bitInTable / 32;
        tUint16 bitInEntry = bitInTable & 0x1f;
        tUint32 bitMask = 1 << bitInEntry;
        return ((idt8a3xxxx_trigger_register_bitmap[tblEntry] & bitMask) != 0);
    }
    return 0;
}
 void idt8a3xxxxInitRegIsTriggerBitmap(void)
{
    if (!idt8a3xxxx_trigger_register_bitmap_initialized)
    {
        for (tUint16 tblIdx = 0; tblIdx < (unsigned int)(sizeof (idt8a3xxxx_trigger_registers) / sizeof ((idt8a3xxxx_trigger_registers) [0])); tblIdx++)
        {
            idt8a3xxxxSetRegIsTrigger(idt8a3xxxx_trigger_registers[tblIdx]);
        }
        idt8a3xxxx_trigger_register_bitmap_initialized = 1;
    }
}
 void idt8a3xxxxInitRegSem(tIdt8a3xxxxDevIndex devIdx)
{
    printf("devIdx %u Init idt8a3xxxx sem" "\n", devIdx);
    if (idt8a3xxxxRegSem[devIdx] == NULL)
    {
        char mutex_str[32];
        snprintf(mutex_str, sizeof(mutex_str),"idt8a3xxxx %u RegLock", devIdx);
        idt8a3xxxxRegSem[devIdx] = nullptr;
    }
}
static inline void idt8a3xxxxRegLock(tIdt8a3xxxxDevIndex devIdx)
{
    while(0);
}
static inline void idt8a3xxxxRegUnlock(tIdt8a3xxxxDevIndex devIdx)
{
    while(0);
}
 inline tUint16 idt8a3xxxxOffsetToSpiCtrl (tUint16 regOffset, tBoolean read)
{
    tUint16 spiCtrl;
    spiCtrl = regOffset & 0x7fff;
    if (read)
        spiCtrl |= 0x8000;
    return spiCtrl;
}
 inline tUint8 idt8a3xxxx1bOffsetToSpiCtrl (tUint8 regOffset, tBoolean read)
{
    tUint8 spiCtrl;
    spiCtrl = regOffset & 0x7f;
    if (read)
        spiCtrl |= 0x80;
    return spiCtrl;
}
 tUint8 idt8a3xxxxSpiRead8(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset)
{
    tUint8 data = 0;
    tUint16 spiCtrl = idt8a3xxxxOffsetToSpiCtrl(regOffset, 1);
    if (spiWriteNRead8(&(idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.spiParms), spiCtrl, sizeof(spiCtrl), &data) != 0)
        printf("devIdx %u Error reading idt8a3xxxx register 0x%x" "\n", devIdx, regOffset);
    if (idt8a3xxxx_debug[devIdx])
        printf("devIdx %u idt8a3xxxx Read 0x%04x:0x%02x" "\n", devIdx, regOffset, data);
    return data;
}
 void idt8a3xxxxGetReg(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint32 numRegs, tUint8 *data)
{
    int i;
    idt8a3xxxxRegLock(devIdx);
    eIdt8a3xxxxAddressType addrType = idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.addrType;
    if ((numRegs > 1) && (addrType == idt8a3xxxxHostSpiAddressType) && (numRegs <= 32))
    {
    }
    else
    {
        for (i=0; i<numRegs; i++)
        {
            tUint8 b = 0;
            switch (addrType)
            {
                case idt8a3xxxxSpiAddressType: b = idt8a3xxxxSpiRead8(devIdx, regOffset+i); break;
                default: b = 0; break;
            }
            data[i] = b;
        }
    }
    idt8a3xxxxRegUnlock(devIdx);
}
 tUint8 idt8a3xxxxGetReg8(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset)
{
    tUint8 regVal = 0;
    idt8a3xxxxGetReg(devIdx, regOffset, 1, &regVal);
    return regVal;
}
 tUint16 idt8a3xxxxGetReg16(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset)
{
    tUint16 regVal = 0;
    idt8a3xxxxGetReg(devIdx, regOffset, 2, (tUint8 *)&regVal);
    regVal = le16toh(regVal);
    return regVal;
}
 tUint32 idt8a3xxxxGetReg32(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset)
{
    tUint32 regVal = 0;
    idt8a3xxxxGetReg(devIdx, regOffset, 4, (tUint8 *)&regVal);
    regVal = le32toh(regVal);
    return regVal;
}
 tUint64 idt8a3xxxxGetReg40(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset)
{
    tUint64 regVal = 0;
    idt8a3xxxxGetReg(devIdx, regOffset, 5, (tUint8 *)&regVal);
    regVal = le64toh(regVal);
    return regVal;
}
 tUint64 idt8a3xxxxGetReg48(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset)
{
    tUint64 regVal = 0;
    idt8a3xxxxGetReg(devIdx, regOffset, 6, (tUint8 *)&regVal);
    regVal = le64toh(regVal);
    return regVal;
}
 void idt8a3xxxxSpiBurstWrite(tIdt8a3xxxxDevIndex devIdx, const tUint16 regOffset, const tUint8 *data, const tUint32 numRegs)
{
    tUint16 spiCtrl = idt8a3xxxxOffsetToSpiCtrl(regOffset, 0);
    tUint8 * ptr = idt8a3xxxxBurstBuf[devIdx].buf;
    assert((numRegs <= (idt8a3xxxxBurstBuf[devIdx].length - sizeof(regOffset))));
    ptr[0] = (spiCtrl >> 8) & 0xFF;
    ptr[1] = (spiCtrl >> 0) & 0xFF;
    for (tUint32 i = 0; i < numRegs; i++)
        ptr[i+2] = data[i];
    if (spiWriteBlock(&(idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.spiParms), ptr, (numRegs + sizeof(spiCtrl))) != 0)
        printf("devIdx %u Error writing idt8a3xxxx register 0x%x" "\n", devIdx, regOffset);
}
 void idt8a3xxxx1bSpiWrite8(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint8 data)
{
    tUint8 spiCtrl = idt8a3xxxx1bOffsetToSpiCtrl(regOffset, 0);
    tUint32 writeVal = (spiCtrl << 8) | data;
    if (spiWrite16(&(idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.spiParms), writeVal) != 0)
        printf("devIdx %u Error writing idt8a3xxxx register 0x%x" "\n", devIdx, regOffset);
    if (idt8a3xxxx_debug[devIdx])
        printf("devIdx %u idt8a3xxxx Write 0x%04x:0x%02x" "\n", devIdx, regOffset, data);
}
 tBoolean idt8a3xxxxRemoveCheck(tIdt8a3xxxxDevIndex devIdx)
{
    if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.customParms.customRemovedFn)
        return idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.customParms.customRemovedFn(idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.customParms.handle);
    else
        return 0;
}
 void idt8a3xxxxUsDelay(uint32_t uSecs)
{
    usleep((ULONG)uSecs*1000 / 1000);
}
 tStatus idt8a3xxxxWaitNumSecAndCheckRemoved(tIdt8a3xxxxDevIndex devIdx, tUint32 numSecToWait)
{
    tStatus rc = 0;
    tInt64 usleepInterval = 100*1000;
    tInt64 totalSleep = numSecToWait*1000*1000;
    do
    {
        if (idt8a3xxxxRemoveCheck(devIdx))
        {
            rc = (-1);
            break;
        }
        usleep(usleepInterval);
        totalSleep -= usleepInterval;
    } while (totalSleep > 0);
    return rc;
}
 void idt8a3xxxxSetReg(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint32 numRegs, tUint8 *data)
{
    int i;
    idt8a3xxxxRegLock(devIdx);
    eIdt8a3xxxxAddressType addrType = idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.addrType;
    switch (addrType)
    {
        case idt8a3xxxxSpiAddressType:
            idt8a3xxxxSpiBurstWrite(devIdx, regOffset, data, numRegs);
            break;
        default:
            assert((0));
            break;
    }
    tUint16 lastOffsetWritten = regOffset+numRegs-1;
    if (idt8a3xxxxGetRegIsTrigger(lastOffsetWritten))
    {
        idt8a3xxxxUsDelay(200);
        if ((lastOffsetWritten | 0x8000) == (0xCAE0 + 0x08))
            idt8a3xxxxUsDelay(300);
    }
    idt8a3xxxxRegUnlock(devIdx);
}
 void idt8a3xxxxSetFirmwareBuffer(tIdt8a3xxxxDevIndex devIdx, const tIdt8aFirmware * fw)
{
    idt8a3xxxxRegLock(devIdx);
    eIdt8a3xxxxAddressType addrType = idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.addrType;
    switch (addrType)
    {
        case idt8a3xxxxSpiAddressType: idt8a3xxxxSpiBurstWrite(devIdx, fw->offset, fw->data, fw->count); break;
        default: assert((0)); break;
    }
    idt8a3xxxxRegUnlock(devIdx);
}
 void idt8a3xxxx1bSetReg(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint32 numRegs, tUint8 *data)
{
    int i;
    idt8a3xxxxRegLock(devIdx);
    eIdt8a3xxxxAddressType addrType = idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.addrType;
    switch (addrType)
    {
        case idt8a3xxxxSpiAddressType:
            for (i=0; i<numRegs; i++)
                idt8a3xxxx1bSpiWrite8(devIdx, (regOffset+i), *(data + i));
            break;
        default:
            assert((0));
            break;
    }
    tUint16 lastOffsetWritten = regOffset+numRegs-1;
    if (idt8a3xxxxGetRegIsTrigger(lastOffsetWritten))
    {
        idt8a3xxxxUsDelay(200);
    }
    idt8a3xxxxRegUnlock(devIdx);
}
 void idt8a3xxxxSetReg8(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint8 regVal)
{
    tUint8 regWriteVal = regVal;
    idt8a3xxxxSetReg(devIdx, regOffset, 1, &regWriteVal);
}
 void idt8a3xxxx1bSetReg8(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint8 regVal)
{
    tUint8 regWriteVal = regVal;
    idt8a3xxxx1bSetReg(devIdx, regOffset, 1, &regWriteVal);
}
 void idt8a3xxxxSetReg8_Field(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint8 mask, tUint8 shift, tUint8 fieldVal)
{
    idt8a3xxxxRegLock(devIdx);
    tUint8 regVal = idt8a3xxxxGetReg8(devIdx, regOffset);
    regVal &= ~mask;
    regVal |= ((fieldVal << shift) & mask);
    idt8a3xxxxSetReg8(devIdx, regOffset, regVal);
    idt8a3xxxxRegUnlock(devIdx);
}
 void idt8a3xxxxSetReg16(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint16 regVal)
{
    tUint16 regWriteVal = htole16(regVal);
    idt8a3xxxxSetReg(devIdx, regOffset, 2, (tUint8 *)&regWriteVal);
}
 void idt8a3xxxxSetReg32(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint32 regVal)
{
    tUint32 regWriteVal = htole32(regVal);
    idt8a3xxxxSetReg(devIdx, regOffset, 4, (tUint8 *)&regWriteVal);
}
 void idt8a3xxxx1bSetReg32(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint32 regVal)
{
    tUint32 regWriteVal = htole32(regVal);
    idt8a3xxxx1bSetReg(devIdx, regOffset, 4, (tUint8 *)&regWriteVal);
}
 void idt8a3xxxxSetReg40(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint64 regVal)
{
    tUint64 regWriteVal = htole64(regVal);
    idt8a3xxxxSetReg(devIdx, regOffset, 5, (tUint8 *)&regWriteVal);
}
 void idt8a3xxxxSetReg48(tIdt8a3xxxxDevIndex devIdx, tUint16 regOffset, tUint64 regVal)
{
    tUint64 regWriteVal = htole64(regVal);
    idt8a3xxxxSetReg(devIdx, regOffset, 6, (tUint8 *)&regWriteVal);
}
 tIdt8a3xxxxDevIndex idt8a3xxxxInitDevInfo(tIdt8a3xxxxDevIndex devIdx,
                                                 const tIdt8a3xxxxDeviceConfigInfo * pDeviceInfo)
{
    static tBoolean hasBeenCalled = 0;
    if (!hasBeenCalled)
    {
        hasBeenCalled = 1;
        idt8a3xxxxNumUsedDevices = 0;
    }
    assert((pDeviceInfo));
    assert((idt8a3xxxxNumUsedDevices < 10));
    if (pDeviceInfo == NULL)
    {
        printf("Missing device config info" "\n");
        return 10;
    }
    if (idt8a3xxxxNumUsedDevices >= 10)
    {
        printf("Too many IDT8A3XXXX devices" "\n");
        return 10;
    }
    if (devIdx == ((tIdt8a3xxxxDevIndex) -1))
    {
        devIdx = idt8a3xxxxNumUsedDevices;
        idt8a3xxxxNumUsedDevices++;
    }
    switch (pDeviceInfo->deviceVariant)
    {
         case idt8a34001DeviceVariant:
            idt8a3xxxxNumInputsForDev[devIdx] = idt8a3xxxxNumInput;
            idt8a3xxxxNumDpllsForDev[devIdx] = idt8a3xxxxSystemDpll;
            idt8a3xxxxNumOutputsForDev[devIdx] = idt8a3xxxxNumOutput;
            idt8a3xxxxNumTodsForDev[devIdx] = idt8a3xxxxNumTod;
            idt8a3xxxxExpectedProductId[devIdx] = 0x4001;
            break;
        case idt8a34012DeviceVariant:
            idt8a3xxxxNumInputsForDev[devIdx] = idt8a3xxxxInput14;
            idt8a3xxxxNumDpllsForDev[devIdx] = idt8a3xxxxDpll4;
            idt8a3xxxxNumOutputsForDev[devIdx] = idt8a3xxxxOutput8;
            idt8a3xxxxNumTodsForDev[devIdx] = idt8a3xxxxNumTod;
            idt8a3xxxxExpectedProductId[devIdx] = 0x4012;
            break;
        case idt8a34045DeviceVariant:
            idt8a3xxxxNumInputsForDev[devIdx] = idt8a3xxxxInput4;
            idt8a3xxxxNumDpllsForDev[devIdx] = idt8a3xxxxSystemDpll;
            idt8a3xxxxNumOutputsForDev[devIdx] = idt8a3xxxxNumOutput;
            idt8a3xxxxNumTodsForDev[devIdx] = idt8a3xxxxTod0;
            idt8a3xxxxExpectedProductId[devIdx] = 0x4045;
            break;
        case idt8a35003DeviceVariant:
        default:
            idt8a3xxxxNumInputsForDev[devIdx] = idt8a3xxxxNumInput;
            idt8a3xxxxNumDpllsForDev[devIdx] = idt8a3xxxxSystemDpll;
            idt8a3xxxxNumOutputsForDev[devIdx] = idt8a3xxxxNumOutput;
            idt8a3xxxxNumTodsForDev[devIdx] = idt8a3xxxxNumTod;
            idt8a3xxxxExpectedProductId[devIdx] = 0x5003;
            break;
    }
    idt8a3xxxxInitRegIsTriggerBitmap();
    idt8a3xxxxInitRegSem(devIdx);
    idt8a3xxxxCurrentDeviceConfigInfo[devIdx] = pDeviceInfo;
    if ((idt8a3xxxxBurstBuf[devIdx].length == 0) && (idt8a3xxxxBurstBuf[devIdx].buf == NULL))
    {
        idt8a3xxxxBurstBuf[devIdx].length = ((32 * 1024) + sizeof(tUint16));
        idt8a3xxxxBurstBuf[devIdx].buf = (tUint8*) calloc(sizeof(tUint8), idt8a3xxxxBurstBuf[devIdx].length);
    }
    assert(((idt8a3xxxxBurstBuf[devIdx].length != 0) && (idt8a3xxxxBurstBuf[devIdx].buf != NULL)));
    return devIdx;
}
 void idt8a3xxxxSwitchTo2bMode(tIdt8a3xxxxDevIndex devIdx)
{
    eIdt8a3xxxxAddressType addrType = idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->addrInfo.addrType;
    tBoolean isSpi = (((addrType) == idt8a3xxxxSpiAddressType) || ((addrType) == idt8a3xxxxHostSpiAddressType) || ((addrType) == idt8a3xxxxCustomSpiAddressType)) ? 1 : 0;
    tUint32 serialModulePage = 0xCAE0 & 0x0000FF00;
    tUint32 pageValue = 0x20100080;
    pageValue |= serialModulePage;
    idt8a3xxxx1bSetReg32(devIdx, 0x7C, pageValue);
    idt8a3xxxx1bSetReg8(devIdx, (0xCAE0 + 0x02), (isSpi ? 0x06 : 0x05));
    if (isSpi)
        idt8a3xxxx1bSetReg8(devIdx, (0xCAE0 + 0x03), 0x00);
    idt8a3xxxxRegLock(devIdx);
    idt8a3xxxx1bSetReg8(devIdx, (0xCAE0 + 0x08), 0xA0);
    idt8a3xxxxUsDelay(600);
    idt8a3xxxxRegUnlock(devIdx);
}
 void idt8a3xxxxBringupByDownload(tIdt8a3xxxxDevIndex devIdx)
{
    tUint32 registers = 0;
    tUint32 bursts = 0;
    idt8a3xxxxSwitchTo2bMode(devIdx);
    if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->emptyPromOnly)
    {
        idt8a3xxxxEepromSetCurrentBlock(devIdx, 0);
        idt8a3xxxxEepromLoadStatus[devIdx] = idt8a3xxxxGetReg8(devIdx, (0xC014 + 0x26));
        if ((idt8a3xxxxEepromLoadStatus[devIdx] == 0x00) && !idt8a3xxxxEepromIsEmpty(devIdx))
        {
            printf("dev %u:  %s... PROM Is not empty so using config & firmware from it." "\n", devIdx, idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->deviceName);
            return;
        }
    }
    if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->firmware)
    {
        printf("Downloading f/w %u.%u.%u %s: productId %04x on devIdx %u" "\n",
                  idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->firmware->major,
                  idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->firmware->minor,
                  idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->firmware->hotfix,
                  idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->firmware->name,
                  idt8a3xxxxGeneralGetProductId(devIdx), devIdx);
        const tIdt8aFirmware * firmware = idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->firmware->firmware;
        while (!idt8aFirmwareEot(firmware) && !idt8a3xxxxRemoveCheck(devIdx))
        {
            idt8a3xxxxSetFirmwareBuffer(devIdx, firmware++);
            idt8a3xxxxUsDelay(200);
            bursts++;
        };
        if (idt8a3xxxxRemoveCheck(devIdx))
            return;
        printf("Did %u bursts - now delay %d seconds" "\n", bursts, 2);
        if (idt8a3xxxxWaitNumSecAndCheckRemoved(devIdx, 2) != 0)
            return;
    }
    idt8a3xxxxEepromLoadStatus[devIdx] = idt8a3xxxxGetReg8(devIdx, (0xC014 + 0x26));
    if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->configFile)
    {
        printf("Set registers: productId %04x" "\n", idt8a3xxxxGeneralGetProductId(devIdx));
        const tAdPllConfig * configFile = idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->configFile;
        bursts = 0;
        while (!PLL_EOT(configFile))
        {
            tUint16 baseAddr = configFile->offset;
            tUint8 data[256] = { 0 };
            tUint32 dataLength = 0;
            do
            {
                data[dataLength++] = (configFile++)->value;
                registers++;
            } while(((configFile->offset == (baseAddr + dataLength)) && !idt8a3xxxxGetRegIsTrigger(baseAddr + dataLength - 1) && (dataLength < (unsigned int)(sizeof (data) / sizeof ((data) [0])))));
            if (idt8a3xxxxRemoveCheck(devIdx))
                break;
            idt8a3xxxxSetReg(devIdx, baseAddr, dataLength, data);
            bursts++;
        }
        if (idt8a3xxxxRemoveCheck(devIdx))
            return;
        printf("%u registers in %u bursts - now delay %d seconds on DevIdx %u" "\n", registers, bursts, 2, devIdx);
        if (idt8a3xxxxWaitNumSecAndCheckRemoved(devIdx, 2) != 0)
            return;
    }
    printf("productId %04x" "\n", idt8a3xxxxGeneralGetProductId(devIdx));
    printf("%s" "\n", idt8a3xxxxImageVersionToString(devIdx).c_str());
}
 void idt8a3xxxxEepromSetI2cAddr(tIdt8a3xxxxDevIndex devIdx, tUint8 i2cAddr)
{
    if ((devIdx < idt8a3xxxxNumUsedDevices))
    {
        tUint8 i2cAddrReg = idt8a3xxxxGetReg8(devIdx, (0xCF68 + 0x00));
        i2cAddrReg &= 0x80;
        i2cAddrReg |= (i2cAddr & 0x7F);
        idt8a3xxxxSetReg8(devIdx, (0xCF68 + 0x00), i2cAddrReg);
    }
    else
    {
        printf("Bad parm: devIdx %u" "\n", devIdx);
    }
}
 void idt8a3xxxxEepromSetCurrentBlock(tIdt8a3xxxxDevIndex devIdx, tUint32 block)
{
    if ((devIdx < idt8a3xxxxNumUsedDevices) && (block < 2))
    {
        idt8a3xxxxEepromSetI2cAddr(devIdx,
                                   idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->eepromBlockI2cAddr[block]);
        idt8a3xxxxCurrentEepromBlock[devIdx] = block;
    }
    else
    {
        printf("Bad parm: devIdx %u block %u" "\n", devIdx, block);
    }
}
 void idt8a3xxxxEepromSetBlockForOffset(tIdt8a3xxxxDevIndex devIdx, tUint32 offsetInEeprom)
{
    tUint32 block = offsetInEeprom >> 16;
    if ((devIdx < idt8a3xxxxNumUsedDevices) && (block < 2))
    {
            idt8a3xxxxEepromSetCurrentBlock(devIdx, block);
    }
    else
    {
        printf("Bad parm: devIdx %u offset %u" "\n", devIdx, offsetInEeprom);
    }
}
 const char *fmtIdt8a3xxxxEepromStatus(tUint16 eepromStatus)
{
    switch (eepromStatus)
    {
        case 0x0000:
            return "no status";
        case 0x8000:
            return "ok";
        case 0x8001:
            return "unknown command";
        case 0x8002:
            return "wrong size";
        case 0x8003:
            return "out of range";
        case 0x8004:
            return "read failed";
        case 0x8005:
            return "write failed";
        case 0x8006:
            return "verification failed";
        default:
            return "Unknown";
    }
}
 tStatus idt8a3xxxxEepromGetBytes(tIdt8a3xxxxDevIndex devIdx, tUint32 offsetInEeprom, tUint8 numBytes)
{
    if ((devIdx < idt8a3xxxxNumUsedDevices) && (numBytes <= 128))
    {
        tStatus status = 0;
        idt8a3xxxxEepromSetBlockForOffset(devIdx, offsetInEeprom);
        tUint8 readSize = numBytes;
        idt8a3xxxxSetReg8(devIdx, (0xCF68 + 0x01), readSize);
        tUint16 offsetInBlock = offsetInEeprom & 0xFFFF;
        idt8a3xxxxSetReg16(devIdx, (0xCF68 + 0x02), offsetInBlock);
        tUint16 eepromCmd = 0xEE01;
        idt8a3xxxxSetReg16(devIdx, (0xCF68 + 0x04), eepromCmd);
        tUint32 numGets = 0;
        tUint16 eepromStatus = 0x0000;
        while ((eepromStatus == 0x0000) && (numGets < 100))
        {
            if (idt8a3xxxxRemoveCheck(devIdx))
                break;
            usleep(10 * 1000);
            eepromStatus = idt8a3xxxxGetReg16(devIdx, (0xC014 + 0x08));
            numGets++;
        }
        if (idt8a3xxxxRemoveCheck(devIdx))
            status = (-1);
        if ((eepromStatus != 0x8000) && !idt8a3xxxxRemoveCheck(devIdx))
        {
            printf("Read failed rc = 0x%04x (%s) offset %u num %u" "\n",
                      eepromStatus,
                      fmtIdt8a3xxxxEepromStatus(eepromStatus),
                      offsetInEeprom, numBytes);
            status = (-1);
        }
        return status;
    }
    else
    {
        printf("Bad parm: devIdx %u, numBytes 0x%02x" "\n", devIdx, numBytes);
    }
    return (-1);
}
 tStatus idt8a3xxxxEepromSetBytes(tIdt8a3xxxxDevIndex devIdx, tUint32 offsetInEeprom, tUint32 numBytes)
{
    if ((devIdx < idt8a3xxxxNumUsedDevices) && (numBytes <= 128))
    {
        tStatus status = 0;
        tUint32 numRetries = 0;
        idt8a3xxxxEepromSetBlockForOffset(devIdx, offsetInEeprom);
        tUint8 writeSize = numBytes;
        idt8a3xxxxSetReg8(devIdx, (0xCF68 + 0x01), writeSize);
        tUint16 offsetInBlock = offsetInEeprom & 0xFFFF;
        idt8a3xxxxSetReg16(devIdx, (0xCF68 + 0x02), offsetInBlock);
        tUint16 eepromCmd = 0xEE02;
        while (1)
        {
            idt8a3xxxxSetReg16(devIdx, (0xCF68 + 0x04), eepromCmd);
            tUint32 numGets = 0;
            tUint16 eepromStatus = 0x0000;
            while ((eepromStatus == 0x0000) && (numGets < 100))
            {
                usleep(10 * 1000);
                eepromStatus = idt8a3xxxxGetReg16(devIdx, (0xC014 + 0x08));
                numGets++;
            }
            if (eepromStatus == 0x8000)
                break;
            if (numRetries < 10)
            {
                numRetries++;
                continue;
            }
            else
            {
                printf("Write failed rc = 0x%04x (%s) numRetries %d" "\n",
                          eepromStatus,
                          fmtIdt8a3xxxxEepromStatus(eepromStatus),
                          numRetries);
                status = (-1);
                break;
            }
        }
        return status;
    }
    else
    {
        printf("Bad parm: devIdx %u, numBytes 0x%02x" "\n", devIdx, numBytes);
    }
    return (-1);
}
 tUint32 idt8a3xxxCalculateCurrentBytes(tUint32 offsetInEeprom, tUint32 bytesLeft)
{
    tUint32 offsetInBlock = offsetInEeprom & 0xFFFF;
    tUint32 bytesLeftInBlock = 0x10000 - offsetInBlock;
    tUint32 currentBytes;
    currentBytes = (bytesLeft < bytesLeftInBlock) ? bytesLeft : bytesLeftInBlock;
    currentBytes = (currentBytes < 128) ? currentBytes : 128;
    return currentBytes;
}
 tStatus idt8a3xxxxEepromGetRange(tIdt8a3xxxxDevIndex devIdx, tUint32 offsetInEeprom, tUint32 numBytes, tUint8 * dataPtr)
{
    if (((devIdx < idt8a3xxxxNumUsedDevices)) && ((offsetInEeprom + numBytes) <= 0x20000))
    {
        tStatus status = 0;
        tUint32 currentOffsetInEeprom = offsetInEeprom;
        tUint32 bytesLeft = numBytes;
        tUint8 * currentDataPtr = dataPtr;
        while (bytesLeft > 0)
        {
            tUint32 currentBytes = idt8a3xxxCalculateCurrentBytes(currentOffsetInEeprom, bytesLeft);
            status = idt8a3xxxxEepromGetBytes(devIdx, currentOffsetInEeprom, currentBytes);
            if (status != 0)
                break;
            for (int i = 0; i < currentBytes; i++)
            {
                *currentDataPtr = idt8a3xxxxGetReg8(devIdx, (0xCF80 + i ));
                currentDataPtr++;
            }
            currentOffsetInEeprom += currentBytes;
            bytesLeft -= currentBytes;
        }
        return status;
    }
    else
    {
        printf("Bad parm: devIdx %u, offset 0x%04x numBytes 0x%02x" "\n", devIdx, offsetInEeprom, numBytes);
    }
    return (-1);
}
 tBoolean idt8a3xxxxEepromIsEmpty(tIdt8a3xxxxDevIndex devIdx)
{
    if ((devIdx < idt8a3xxxxNumUsedDevices))
    {
        static const tUint32 addressesToCheck[] = { 0x0, 0x10, 0xF000, 0xF010 };
        for (tUint32 i = 0; i < (unsigned int)(sizeof (addressesToCheck) / sizeof ((addressesToCheck) [0])); i++)
        {
            tUint8 byte;
            idt8a3xxxxEepromGetRange(devIdx, addressesToCheck[i], 1, &byte);
            if (byte != 0xff)
                return (idt8a3xxxxRemoveCheck(devIdx) ? 1 : 0);
        }
    }
    return 1;
}
 tStatus idt8a3xxxxEepromSetRange(tIdt8a3xxxxDevIndex devIdx, tUint32 offsetInEeprom, tUint32 numBytes, const tUint8 * dataPtr)
{
    if (((devIdx < idt8a3xxxxNumUsedDevices)) && ((offsetInEeprom + numBytes) <= 0x20000) && (dataPtr != NULL))
    {
        tStatus status = 0;
        tUint32 currentOffsetInEeprom = offsetInEeprom;
        tUint32 bytesLeft = numBytes;
        const tUint8 * currentDataPtr = dataPtr;
        while (bytesLeft > 0)
        {
            tUint32 currentBytes = idt8a3xxxCalculateCurrentBytes(currentOffsetInEeprom, bytesLeft);
            for (int i = 0; i < currentBytes; i++)
            {
                idt8a3xxxxSetReg8(devIdx, (0xCF80 + i ), *currentDataPtr);
                currentDataPtr++;
            }
            status = idt8a3xxxxEepromSetBytes(devIdx, currentOffsetInEeprom, currentBytes);
            if (status != 0)
                break;
            currentOffsetInEeprom += currentBytes;
            bytesLeft -= currentBytes;
        }
        return status;
    }
    else
    {
        printf("Bad parm: devIdx %u, offset 0x%04x numBytes 0x%02x dataPtr %p" "\n", devIdx, offsetInEeprom, numBytes, dataPtr);
    }
    return (-1);
}
 tStatus idt8a3xxxxEepromSetAll(tIdt8a3xxxxDevIndex devIdx, const tUint8 * dataPtr)
{
    if ((devIdx < idt8a3xxxxNumUsedDevices) && (dataPtr != NULL))
    {
        return idt8a3xxxxEepromSetRange(devIdx, 0, 0x20000, dataPtr);
    }
    else
    {
        printf("Bad parm: devIdx %u dataPtr %p" "\n", devIdx, dataPtr);
    }
    return (-1);
}
 tStatus idt8a3xxxxProgramEepromFromFile(tIdt8a3xxxxDevIndex devIdx,
                                               char *filename,
                                               tBoolean verbose)
{
    tUint8 * eeprombits;
    int romErr;
    int i;
    romErr = getBitfile(filename, NULL, reinterpret_cast<char **>(&eeprombits), NULL, NULL);
    if (romErr != 0)
    {
        printf("Failed to get the bit file\n");
        return (-1);
    }
    tUint32 byteOffset = 0;
    tUint8 * dataPtr = eeprombits + byteOffset;
    printf("First 16 bytes...");
    for (i = 0; i < 16; i++)
    {
        if ((byteOffset % 16) == 0)
        {
            printf("\n");
            printf("0x%05x: ", byteOffset);
        }
        printf(" %02x", *dataPtr);
        byteOffset++;
        dataPtr++;
    }
    byteOffset = 0xfff0;
    dataPtr = eeprombits + byteOffset;
    printf("\nAt block boundary...");
    for (i = 0; i < 32; i++)
    {
        if ((byteOffset % 16) == 0)
        {
            printf("\n");
            printf("0x%05x: ", byteOffset);
        }
        printf(" %02x", *dataPtr);
        byteOffset++;
        dataPtr++;
    }
    byteOffset = 0x1fff0;
    dataPtr = eeprombits + byteOffset;
    printf("\nAt the end of the image...");
    for (i = 0; i < 16; i++)
    {
        if ((byteOffset % 16) == 0)
        {
            printf("\n");
            printf("0x%05x: ", byteOffset);
        }
        printf(" %02x", *dataPtr);
        byteOffset++;
        dataPtr++;
    }
    printf("\nProgramming the EEPROM...\n");
    tStatus status = idt8a3xxxxEepromSetAll(devIdx, eeprombits);
    free(eeprombits);
    if (status == 0)
        printf("Programming passed\n");
    else
        printf("Programming failed\n");
    return status;
}
 const char * fmtIdt8a3xxxxDeviceVariant(eIdt8a3xxxxDeviceVariants devVariant)
{
    switch (devVariant)
    {
        case idt8a34001DeviceVariant:
            return "idt8a34001";
        case idt8a34012DeviceVariant:
            return "idt8a34012";
        case idt8a34045DeviceVariant:
            return "idt8a34045";
        case idt8a35003DeviceVariant:
            return "idt8a35003";
        default:
            return "unknown";
    }
}
 std::string idt8a3xxxxNamesToString(tIdt8a3xxxxDevIndex devIdx)
{
    std::string str;
    if ((devIdx < idt8a3xxxxNumUsedDevices))
    {
        str += Format("Device Index: %u ", devIdx);
        if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx] != NULL)
        {
            str += Format("variant %s name %s\n",
                   fmtIdt8a3xxxxDeviceVariant(idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->deviceVariant),
                   idt8a3xxxxCurrentDeviceConfigInfo[devIdx]->deviceName);
        }
        else
            str += "no name info\n";
    }
    else
    {
        str += Format("Bad parm: devIdx %u\n", devIdx);
    }
    return str;
}
 tUint16 idt8a3xxxxGeneralGetProductId(tIdt8a3xxxxDevIndex devIdx)
{
    tUint16 productId = 0;
    if ((devIdx < idt8a3xxxxNumUsedDevices))
    {
        productId = idt8a3xxxxGetReg16(devIdx, (0xC014 + 0x1E));
    }
    return productId;
}
 void idt8a3xxxxDumpVersion(tIdt8a3xxxxDevIndex devIdx)
{
    printf("%s", idt8a3xxxxVersionToString(devIdx).c_str());
}
 std::string idt8a3xxxxVersionToString(tIdt8a3xxxxDevIndex devIdx)
{
    std::string str;
    if ((devIdx < idt8a3xxxxNumUsedDevices))
    {
        do { if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx] == NULL) { str += Format("There is no idt8a3xxxx device at devIdx %d\n", devIdx); return str; } } while (0);
        tUint8 productRelease;
        tUint16 productId = idt8a3xxxxGeneralGetProductId(devIdx);
        tUint8 majorRelease = idt8a3xxxxGetReg8(devIdx, (0xC014 + 0x10));
        tUint8 minorRelease = idt8a3xxxxGetReg8(devIdx, (0xC014 + 0x11));
        tUint8 hotfixRelease = idt8a3xxxxGetReg8(devIdx, (0xC014 + 0x12));
        productRelease = (majorRelease & 0x01) >> 0;
        majorRelease = (majorRelease & 0xFE) >> 1;
        str += Format("ProductId 0x%04x %s release %u.%u.%u\n",
                        productId,
                        productRelease ? "Product" : "Development",
                        majorRelease,
                        minorRelease,
                        hotfixRelease);
           str += Format("%s", idt8a3xxxxImageVersionToString(devIdx).c_str());
    }
    else
    {
        str += Format("Bad parm: devIdx %u\n", devIdx);
    }
    return str;
}
 tUint32 idt8a3xxxxGetImageVersion(tIdt8a3xxxxDevIndex devIdx)
{
    if ((devIdx < idt8a3xxxxNumUsedDevices))
    {
        return idt8a3xxxxGetReg32(devIdx, (0xCF50 + (0x4 * (idt8a3xxxxScratch0 - idt8a3xxxxScratch0))));
    }
    else
    {
        printf("Bad parm: devIdx %u\n", devIdx);
        return 0;
    }
}
 std::string idt8a3xxxxImageVersionToString(tIdt8a3xxxxDevIndex devIdx)
{
    std::string str;
    if ((devIdx < idt8a3xxxxNumUsedDevices))
    {
        do { if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx] == NULL) { str += Format("There is no idt8a3xxxx device at devIdx %d\n", devIdx); return str; } } while (0);
        tUint32 imageVersion = idt8a3xxxxGetImageVersion(devIdx);
        tUint32 number = (imageVersion & 0xFFFF0000) >> 16;
        tUint32 major = (imageVersion & 0x0000FF00) >> 8;
        tUint32 labUse = (imageVersion & 0x00000080) >> 7;
        tUint32 minor = (imageVersion & 0x0000007F) >> 0;
        str += Format("Image version 80-%04x-%02x, %s, minor rev %d\n",
                        number,
                        major,
                        labUse ? "lab use" : "released",
                        minor);
    }
    else
    {
        str += Format("Bad parm: devIdx %u\n", devIdx);
    }
    return str;
}
 void idt8a3xxxxDumpDevs(void)
{
    printf("There %s %u idt8a3xxxx device%s%s\n",
           idt8a3xxxxNumUsedDevices == 1 ? "is" : "are",
           idt8a3xxxxNumUsedDevices,
           idt8a3xxxxNumUsedDevices == 1 ? "" : "s",
           idt8a3xxxxNumUsedDevices == 0 ? "." : ":");
    if (idt8a3xxxxNumUsedDevices == 0)
        return;
    tIdt8a3xxxxDevIndex devIdx;
    for (devIdx = 0; devIdx < idt8a3xxxxNumUsedDevices; devIdx++)
    {
        if (idt8a3xxxxCurrentDeviceConfigInfo[devIdx] == NULL) continue;
        printf("\n");
        printf("%s", idt8a3xxxxNamesToString(devIdx).c_str());
        idt8a3xxxxDumpVersion(devIdx);
    }
}
}
