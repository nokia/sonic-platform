/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include "math.h"
#include "hw_instance.h"
#include "tmSPI.h"
#include "platform_hw_info.h"
#include <map>
#include <array>
#include <vector>
#include <functional>
#include "platform_types.h"
using StringPairMap = std::map<std::string, std::pair<std::string, std::string>>;
extern "C" {
typedef uint8_t tPconChan;
typedef uint16_t tPconReg;
typedef enum
{
    PCON_DEVICE_MAIN = 0,
    PCON_DEVICE_0 = PCON_DEVICE_MAIN,
    PCON_DEVICE_MEZANINE,
    PCON_DEVICE_1 = PCON_DEVICE_MEZANINE,
    PCON_DEVICE_2,
    PCON_DEVICE_3,
    PCON_DEVICE_4,
    PCON_MAX_DEVICES_PER_IOCTRL,
} ePconIndex;
typedef uint16_t tPconBoardResetType;
typedef struct
{
    const char * name;
    uint16_t voltage;
    uint8_t master : 1;
    uint8_t masterChan;
} tPconChanConfig;
typedef struct
{
    const char * name;
    uint8_t masterChan;
    int8_t voltOffset;
} tPconRailConfig;
typedef struct
{
    const tPconChanConfig * channels;
    tPconRailConfig * rails;
    uint8_t channelCount;
    uint8_t railCount;
} tPconConfig;
typedef struct tPconDeviceProfile
{
    const char * name;
    const char * desc;
    CtlFpgaId fpga_id;
    uint8_t regI2cAddr;
    uint8_t promI2cAddr;
    uint8_t spiChannel : 5;
    uint8_t mini : 1;
    uint8_t spiIfInit : 1;
    uint8_t resetBit;
    uint32_t resetReg;
    uint8_t spiTimer;
    uint8_t index;
    I2CFpgaCtrlrDeviceParams dev_params;
} tPconDeviceProfile;
typedef struct tPconEvent
{
    tPconBoardResetType lastResetReason;
    uint32_t numPowerCycles;
    uint32_t numResetSincePowerUp;
    time_t lastPowerUpTime;
    time_t lastPowerDownTime;
    time_t lastBootUpTime;
    time_t lastPowerOnDuration;
} tPconEvent;
typedef struct
{
    tPconDeviceProfile & dev;
    tPconConfig & config;
} tPconDevice;
typedef struct __attribute__ ((__packed__)) {
     uint16_t crc8 : 8;
     uint16_t prm_file : 1;
     uint16_t ev1_to : 1;
     uint16_t ev0_to : 1;
     uint16_t c_a2d : 1;
     uint16_t v_a2d : 1;
     uint16_t oc : 1;
     uint16_t ov : 1;
     uint16_t uv : 1;
} tChannelStatus;
typedef struct __attribute__ ((__packed__)) {
    uint32_t hdrCrc : 8;
    uint32_t eventPtr : 24;
} tPconEventHeader;
typedef struct __attribute__ ((__packed__)) {
    uint32_t powerCycleNum;
    uint32_t resetCycleNum;
    uint8_t resetReason;
    char reserved0[7];
    uint64_t epochTime;
    char reserved1[7];
    uint8_t crc8;
} tPconEventLogSoftware;
typedef struct __attribute__ ((__packed__)) {
    tChannelStatus channelStatus[42];
    uint32_t upTimeInSeconds;
    uint16_t rawImbvVoltValue;
    uint32_t invUpTimeInSeconds;
    char reserved0[2];
    tPconEventLogSoftware softwareReserved;
} tPconEventLogMemory;
typedef struct __attribute__ ((__packed__)) {
    tChannelStatus channelStatus[16];
    uint32_t upTimeInSeconds;
    uint16_t rawImbvVoltValue;
    uint32_t invUpTimeInSeconds;
    char reserved0[2];
    tPconEventLogSoftware softwareReserved;
    char reserved1[(42-16)<<1];
} tPconMiniEventLogMemory;
typedef struct {
    uint32_t days : 15;
    uint32_t hours : 5;
    uint32_t minutes : 6;
    uint32_t seconds : 6;
} tPoweredOnTime;
typedef uint32_t tRailsampleValue;
enum class CtlrTypeId {cpuctl = 0, ioctl = 1, sfctl = 15};
typedef struct BringupPconRailSamples {
    HwInstance instance;
    uint32_t device_index;
    uint8_t rail_num;
    uint32_t rail_count;
    uint32_t sampling_rate;
    uint32_t sampling_time;
    BringupPconRailSamples(void) : instance({.id = HW_INSTANCE_CARD, .card = { .card_type = GetMyCardType() } }),
                                   device_index{0}, rail_num{0}, rail_count{0}, sampling_rate{1000}, sampling_time{5} {}
} tHwPconRailSamplingParams;
typedef struct {
    uint32_t rail_num;
    const char *rail_name;
    double mean_mv;
    double ripple_ptop_mv;
    double ripple_rms_mv;
    tRailsampleValue sample_min_value;
    tRailsampleValue sample_max_value;
    std::vector<tRailsampleValue> sample_values;
} tHwPconRailSamplingResults;
struct tPconAccessApi
{
    SrlStatus (*hwSpiRead8) (const struct tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata);
    SrlStatus (*hwSpiReadBlock) (const struct tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata, uint32_t rdcount);
    SrlStatus (*hwSpiWrite8) (const struct tSpiParameters *parms, uint32_t data);
    SrlStatus (*hwSpiWrite8ReadBlock) (const struct tSpiParameters *parms, uint32_t wrdata, uint8_t* rddata, uint8_t rdcount);
    SrlStatus (*hwSpiWriteBlock) (const struct tSpiParameters *parms, const uint8_t *wrdata, uint32_t wrcount);
    SrlStatus (*hwSpiPromReadId) (const struct tSpiParameters *parms, tFlashDeviceId *pDeviceId);
    SrlStatus (*hwSpiPromReadByte) (const struct tSpiParameters *parms, uint32_t address, uint8_t *data);
    SrlStatus (*hwSpiPromDump) (const struct tSpiParameters *parms, uint32_t offset, tFlashDeviceId flashDevice);
    SrlStatus (*hwSpiProgramProm) (const struct tSpiParameters *parms, uint8_t *bitfile, uint32_t fileSize, uint32_t location, tSpiProgramMask options);
};
inline constexpr std::array<uint32_t, 8> kJ2RovVolatge = {820, 820, 760, 780,
                                                          800, 840, 860, 880};
inline constexpr std::array<uint32_t, 8> kJ2CPlusRovVolatge = {840, 800, 700, 720,
                                                               740, 760, 780, 820};
inline std::map<uint32_t, uint32_t> kJ3RamonRovVoltage = {{{0x7A, 850}, {0x7c, 837}, {0x7E, 825}, {0x80, 812}, {0x82, 800}, {0x84, 787}, {0x86, 775},
                                                           {0x88, 762}, {0x8A, 750}, {0x8C, 737}, {0x8E, 725}, {0x90, 712}, {0x92, 700}, {0x94, 687},
                                                           {0x96, 675}, {0x98, 662}, {0x9A, 650}}};
tI2cStatus pconReadGlobalReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t reg, uint16_t *value);
tI2cStatus pconWriteGlobalReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t reg, uint16_t *value);
tI2cStatus pconSetPconChan(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint8_t channel);
SrlStatus pconReadChanReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint8_t channel, uint32_t reg, uint16_t *value);
SrlStatus pconWriteChanReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint8_t channel, uint32_t reg, uint16_t *value);
uint32_t hwPconRailApplyScaleFactor(bool scale_up, uint32_t conf_mvolt, uint32_t meas_mvolt);
SrlStatus hwPconReadChannelVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, tPconChan chan, uint32_t * voltage, tPconChanConfig pconChanConfig, bool verbose);
SrlStatus hwPconReadChannelCurrent(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, tPconChan chan, uint32_t * current, bool verbose);
SrlStatus hwPconReadRailVoltage(I2CCtrlr *ctrlr, tPconDevice *pcon_info, uint32_t rail, uint32_t *voltage, bool verbose);
SrlStatus hwPconReadRailCurrent(I2CCtrlr *ctrlr, tPconDevice *pcon_info, uint32_t rail, uint32_t *current, bool verbose);
SrlStatus pconGetMiscInfo(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, tPconChan chan, bool * enable, bool * master, tPconChan * slaveTo);
SrlStatus hwPconSetTargetVoltageInt(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t milli_volt);
SrlStatus getChannelInfo(tPconConfig *pconConfig, uint32_t idx, uint32_t rail_num, uint32_t * channel_num, uint32_t * conf_mvolt);
SrlStatus hwPconSetUnderVoltageInt(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t milli_volt);
SrlStatus hwPconSetOverVoltageInt(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t milli_volt);
SrlStatus hwPconGetMeasuredVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t *milli_volt);
SrlStatus hwPconGetConfiguredVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t *milli_volt);
void hwPconCntrlExtSpiMasterToProm(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, bool enable);
extern SrlStatus hwPconSetTargetVoltageSel(HwInstance instance, uint8_t index, uint32_t rail_num, uint32_t milli_volt);
extern SrlStatus hwPconSetUnderVoltageSel(HwInstance instance, uint8_t index, uint32_t rail_num, uint32_t milli_volt);
extern SrlStatus hwPconSetOverVoltageSel(HwInstance instance, uint8_t index, uint32_t rail_num, uint32_t milli_volt);
extern bool hwPconIdxIsValid(HwInstance instance, uint8_t idx);
extern SrlStatus hwPconReadEventLogMemory(HwInstance instance, uint8_t idx, uint32_t offset, uint8_t *buf, int length);
extern SrlStatus hwPconGetEventLogMemory(HwInstance instance, uint8_t idx, int event_num, tPconEventLogMemory * pEventRam);
extern SrlStatus hwPconGetEventLogMemory(HwInstance instance, uint8_t idx, int event_num, tPconEventLogMemory * pEventRam);
extern void hwPconDumpEvents(HwInstance instance, uint8_t idx, int num_past_events, bool verbose);
extern uint32_t hwPconMaxCurDif(HwInstance instance, uint8_t idx);
extern tSpiParameters getPconSPIParams(HwInstance instance, uint32_t index);
extern I2CFpgaCtrlrDeviceParams getPconI2CParams(HwInstance instance, uint32_t index, I2CCtrlr *ctrlr);
void fromSecondsCalulateOnTime(uint32_t seconds, tPoweredOnTime *pOntimeData);
tPconDevice * hwPconGetPconInfo(HwInstance instance, uint32_t index, bool log_on_failure=1);
I2CCtrlr hwPconGetI2CCtrlr(HwInstance instance, const tPconDevice& card_info);
const tPconDeviceProfile * hwPconGetProfile(const tPconDevice * dev);
bool hwPconIsMini(HwInstance instance, uint32_t index);
}
extern uint16_t hwPconReadGlobalReg(HwInstance instance, uint32_t index, uint8_t reg);
extern void hwPconWriteGlobalReg(HwInstance instance, uint32_t index, uint8_t reg, uint16_t value);
extern uint16_t hwPconReadChannelReg(HwInstance instance, uint32_t index, uint8_t channel, uint8_t reg);
extern void hwPconWriteChannelReg(HwInstance instance, uint32_t index, uint8_t channel, uint8_t reg, uint16_t value);
extern SrlStatus hwPconGetInputVoltage(HwInstance instance, uint32_t index, uint32_t *milli_volt);
extern void hwPconShowDevices(HwInstance instance, bool verbose = 0);
extern std::string hwPconGetDevices(HwInstance instance, bool verbose = 0);
extern SrlStatus hwPconShowChannelsAll(HwInstance instance);
void hwPconShowRailConfigAll(HwInstance instance);
extern std::string hwPconGetRailConfigAll(HwInstance instance);
extern void hwPconShowChannelVoltage(HwInstance instance, uint32_t idx, uint32_t chan, bool verbose = 0);
extern void hwPconShowChannelCurrent(HwInstance instance, uint32_t idx, uint32_t chan, bool verbose = 0);
extern void hwPconShowRailVoltages(HwInstance instance, uint8_t idx, bool verbose);
extern std::string hwPconGetRailVoltages(HwInstance instance, uint8_t idx, bool verbose);
SrlStatus hwPconGetClearEventLogResetReason(HwInstance instance, uint8_t idx);
SrlStatus hwPconGetClearEventLogResetReasonAll(HwInstance instance);
std::string hwPconGetResetReason(HwInstance instance, uint8_t idx);
std::string hwPconGetResetReasonAll(HwInstance instance);
SrlStatus hwPconGetOverVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, int32_t rail_num, uint32_t *milli_volt);
SrlStatus hwPconGetUnderVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t *milli_volt);
int hwPconGetCardPconInfo(HwInstance instance, tPconDevice ** card_info);
