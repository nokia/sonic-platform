/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include <array>
#include <stdio.h>
#include <endian.h>
#include <map>
#include <time.h>
#include "replacements.h"
#include <fmt/format.h>
#include "hw_instance.h"
#include "fpga_if.h"
#include <cstring>
#include "platform_hw_info.h"
#include "platform_types.h"
#include "hwPcon.h"
#include "tmSPI.h"
#include "tmSpiDefs.h"
static const tPconChanConfig caribouPcon0Channels[] =
{
[0]= {"D0_VDDC_P1", 750, 1, ((tPconChan)-1) },
[1]= {"D0_VDDC_P2", 0xffff, 0, 0 },
[2]= {"D0_VDDC_P3", 0xffff, 0, 0 },
[3]= {"D0_VDDC_P4", 0xffff, 0, 0 },
[4]= {"D0_VDDC_P5", 0xffff, 0, 0 },
[5]= {"D0_VDDC_P6", 0xffff, 0, 0 },
[6]= {"D0_VDDC_P7", 0xffff, 0, 0 },
[7]= {"D0_VDDC_P8", 0xffff, 0, 0 },
[8]= {"D0_VDDC_P9", 0xffff, 0, 0 },
[9]= {"D0_VDDC_P10", 0xffff, 0, 0 },
[10]= {"D0_VDDC_P11", 0xffff, 0, 0 },
[11]= {"D0_VDDC_P12", 0xffff, 0, 0 },
[12]= {"D0_VDDC_P13", 0xffff, 0, 0 },
[13]= {"D0_VDDC_P14", 0xffff, 0, 0 },
[14]= {"D0_VDDC_P15", 0xffff, 0, 0 },
[15]= {"D0_VDDC_P16", 0xffff, 0, 0 },
[16]= {"D0_VDDC_P17", 0xffff, 0, 0 },
[17]= {"D0_VDDC_P18", 0xffff, 0, 0 },
[18]= {"D0_NIF_TRVDD0P75_P1", 750, 1, ((tPconChan)-1) },
[19]= {"D0_NIF_TRVDD0P75_P2", 0xffff, 0, 18 },
[20]= {"D0_NIF_TRVDD0P9_P1", 900, 1, ((tPconChan)-1) },
[21]= {"D0_NIF_TRVDD0P9_P2", 0xffff, 0, 20 },
[22]= {"D0_HBM_VDD1P2_P1", 1200, 1, ((tPconChan)-1) },
[23]= {"D0_HBM_VDD1P2_P2", 0xffff, 0, 22 },
[24]= {"D0_VDDO_1P8_P1", 1800, 1, ((tPconChan)-1) },
[25]= {"PB_VDD_P1", 850, 1, ((tPconChan)-1) },
[26]= {"PB_VDD_P2", 0xffff, 0, 25 },
[27]= {"PB_VDD_P3", 0xffff, 0, 25 },
[28]= {"NIF_PVDD_1V15_P1", 1150, 1, ((tPconChan)-1) },
[29]= {"OPT_G3_VDD_3V3_P1", 3300, 1, ((tPconChan)-1) },
[30]= {"OPT_G3_VDD_3V3_P2", 0xffff, 0, 29 },
[31]= {"OPT_G3_VDD_3V3_P3", 0xffff, 0, 29 },
[32]= {"OPT_G4_VDD_3V3_P1", 3300, 1, ((tPconChan)-1) },
[33]= {"OPT_G4_VDD_3V3_P2", 0xffff, 0, 32 },
[34]= {"OPT_G4_VDD_3V3_P3", 0xffff, 0, 32 },
};
static tPconRailConfig caribouPcon0Rails[] =
{
[0]= {"D0_VDDC", 0, 0},
[1]= {"D0_NIF_TRVDD0P75", 18, 0},
[2]= {"D0_NIF_TRVDD0P9", 20, 0},
[3]= {"D0_HBM_VDD1P2", 22, 0},
[4]= {"D0_VDDO_1P8", 24, 0},
[5]= {"PB_VDD", 25, 0},
[6]= {"NIF_PVDD_1V15", 28, 0},
[7]= {"OPT_G3_VDD_3V3", 29, 0},
[8]= {"OPT_G4_VDD_3V3", 32, 0},
};
tPconConfig caribouPcon0 =
{
      .channels = caribouPcon0Channels,
      .rails = caribouPcon0Rails,
      .channelCount = (unsigned int)(sizeof (caribouPcon0Channels) / sizeof ((caribouPcon0Channels) [0])),
      .railCount = (unsigned int)(sizeof (caribouPcon0Rails) / sizeof ((caribouPcon0Rails) [0])),
};
static tPconDeviceProfile caribouPcon0Profile =
{
      .name = "CARIBOU PCON 0",
      .desc = "located on IOCTL",
      .fpga_id = CTL_FPGA_IOCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (1),
      .mini = 0,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 0,
      .dev_params = {.channel = 0x5, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static const tPconChanConfig caribouPcon1Channels[] =
{
[0]= {"D1_VDDC_P1", 750, 1, ((tPconChan)-1) },
[1]= {"D1_VDDC_P2", 0xffff, 0, 0 },
[2]= {"D1_VDDC_P3", 0xffff, 0, 0 },
[3]= {"D1_VDDC_P4", 0xffff, 0, 0 },
[4]= {"D1_VDDC_P5", 0xffff, 0, 0 },
[5]= {"D1_VDDC_P6", 0xffff, 0, 0 },
[6]= {"D1_VDDC_P7", 0xffff, 0, 0 },
[7]= {"D1_VDDC_P8", 0xffff, 0, 0 },
[8]= {"D1_VDDC_P9", 0xffff, 0, 0 },
[9]= {"D1_VDDC_P10", 0xffff, 0, 0 },
[10]= {"D1_VDDC_P11", 0xffff, 0, 0 },
[11]= {"D1_VDDC_P12", 0xffff, 0, 0 },
[12]= {"D1_VDDC_P13", 0xffff, 0, 0 },
[13]= {"D1_VDDC_P14", 0xffff, 0, 0 },
[14]= {"D1_VDDC_P15", 0xffff, 0, 0 },
[15]= {"D1_VDDC_P16", 0xffff, 0, 0 },
[16]= {"D1_VDDC_P17", 0xffff, 0, 0 },
[17]= {"D1_VDDC_P18", 0xffff, 0, 0 },
[18]= {"D1_NIF_TRVDD0P75_P1", 750, 1, ((tPconChan)-1) },
[19]= {"D1_NIF_TRVDD0P75_P2", 0xffff, 0, 18 },
[20]= {"D1_NIF_TRVDD0P9_P1", 900, 1, ((tPconChan)-1) },
[21]= {"D1_NIF_TRVDD0P9_P2", 0xffff, 0, 20 },
[22]= {"D1_HBM_VDD1P2_P1", 1200, 1, ((tPconChan)-1) },
[23]= {"D1_HBM_VDD1P2_P2", 0xffff, 0, 22 },
[24]= {"OPT_G1_VDD_3V3_P1", 3300, 1, ((tPconChan)-1) },
[25]= {"OPT_G1_VDD_3V3_P2", 0xffff, 0, 24 },
[26]= {"OPT_G1_VDD_3V3_P3", 0xffff, 0, 24 },
[27]= {"OPT_G2_VDD_3V3_P1", 3300, 1, ((tPconChan)-1) },
[28]= {"OPT_G2_VDD_3V3_P2", 0xffff, 0, 27 },
[29]= {"OPT_G2_VDD_3V3_P3", 0xffff, 0, 27 },
[30]= {"D1_VDDO_1P8_P1", 1800, 1, ((tPconChan)-1) },
};
static tPconRailConfig caribouPcon1Rails[] =
{
[0]= {"D1_VDDC", 0, 0},
[1]= {"D1_NIF_TRVDD0P75", 18, 0},
[2]= {"D1_NIF_TRVDD0P9", 20, 0},
[3]= {"D1_HBM_VDD1P2", 22, 0},
[4]= {"OPT_G1_VDD_3V3", 24, 0},
[5]= {"OPT_G2_VDD_3V3", 27, 0},
[6]= {"D1_VDDO_1P8", 30, 0},
};
tPconConfig caribouPcon1 =
{
      .channels = caribouPcon1Channels,
      .rails = caribouPcon1Rails,
      .channelCount = (unsigned int)(sizeof (caribouPcon1Channels) / sizeof ((caribouPcon1Channels) [0])),
      .railCount = (unsigned int)(sizeof (caribouPcon1Rails) / sizeof ((caribouPcon1Rails) [0])),
};
static tPconDeviceProfile caribouPcon1Profile =
{
      .name = "CARIBOU PCON 1",
      .desc = "located on IOCTL",
      .fpga_id = CTL_FPGA_IOCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (2),
      .mini = 0,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 1,
      .dev_params = {.channel = 0x6, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static const tPconChanConfig caribouPcon4Channels[] =
{
[0]= {"CPU_VDD_DDR_P1", 1200, 1, ((tPconChan)-1) },
[1]= {"VDD1_0_P1", 1000, 1, ((tPconChan)-1) },
[2]= {"VDD1_8_P1", 1800, 1, ((tPconChan)-1) },
[3]= {"VDD1_8_S5_P1", 1800, 1, ((tPconChan)-1) },
[4]= {"VDD3_3_P1", 3300, 1, ((tPconChan)-1) },
[5]= {"VDD3_3_S5_P1", 3300, 1, ((tPconChan)-1) },
[6]= {"VDD5_0_P1", 5000, 1, ((tPconChan)-1) },
};
static tPconRailConfig caribouPcon4Rails[] =
{
[0]= {"CPU_VDD_DDR", 0, 0},
[1]= {"VDD1_0", 1, 0},
[2]= {"VDD1_8", 2, 0},
[3]= {"VDD1_8_S5", 3, 0},
[4]= {"VDD3_3", 4, 0},
[5]= {"VDD3_3_S5", 5, 0},
[6]= {"VDD5_0", 6, 0},
};
tPconConfig caribouPcon4 =
{
      .channels = caribouPcon4Channels,
      .rails = caribouPcon4Rails,
      .channelCount = (unsigned int)(sizeof (caribouPcon4Channels) / sizeof ((caribouPcon4Channels) [0])),
      .railCount = (unsigned int)(sizeof (caribouPcon4Rails) / sizeof ((caribouPcon4Rails) [0])),
};
static tPconDeviceProfile caribouPcon4Profile =
{
      .name = "CARIBOU PCON 4",
      .desc = "located on CPUCTL",
      .fpga_id = CTL_FPGA_CPUCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (2),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 2,
      .dev_params = {.channel = 0x13, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static std::array<tPconDevice, 3> caribouPconDevices
{
      caribouPcon0Profile, caribouPcon0,
      caribouPcon1Profile, caribouPcon1,
      caribouPcon4Profile, caribouPcon4,
};
static const tPconChanConfig fireflyPcon0Channels[] =
{
[0]= {"J2CP1_VDDC_P1", 800, 1, ((tPconChan)-1) },
[1]= {"J2CP1_VDDC_P2", 0xffff, 0, 0 },
[2]= {"J2CP1_VDDC_P3", 0xffff, 0, 0 },
[3]= {"J2CP1_VDDC_P4", 0xffff, 0, 0 },
[4]= {"J2CP1_VDDC_P5", 0xffff, 0, 0 },
[5]= {"J2CP1_VDDC_P6", 0xffff, 0, 0 },
[6]= {"J2CP1_VDDC_P7", 0xffff, 0, 0 },
[7]= {"J2CP1_VDDC_P8", 0xffff, 0, 0 },
[8]= {"J2CP1_VDDC_P9", 0xffff, 0, 0 },
[9]= {"J2CP1_VDDC_P10", 0xffff, 0, 0 },
[10]= {"J2CP1_VDDC_P11", 0xffff, 0, 0 },
[11]= {"J2CP1_VDDC_P12", 0xffff, 0, 0 },
[12]= {"J2CP1_VDDC_P13", 0xffff, 0, 0 },
[13]= {"J2CP1_VDDC_P14", 0xffff, 0, 0 },
[14]= {"J2CP1_VDDC_P15", 0xffff, 0, 0 },
[15]= {"J2CP1_VDDC_P16", 0xffff, 0, 0 },
};
static tPconRailConfig fireflyPcon0Rails[] =
{
[0]= {"J2CP1_VDDC", 0, 0},
};
tPconConfig fireflyPcon0 =
{
      .channels = fireflyPcon0Channels,
      .rails = fireflyPcon0Rails,
      .channelCount = (unsigned int)(sizeof (fireflyPcon0Channels) / sizeof ((fireflyPcon0Channels) [0])),
      .railCount = (unsigned int)(sizeof (fireflyPcon0Rails) / sizeof ((fireflyPcon0Rails) [0])),
};
static tPconDeviceProfile fireflyPcon0Profile =
{
      .name = "FIREFLY PCON 0",
      .desc = "located on IOCTL",
      .fpga_id = CTL_FPGA_IOCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (1),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 0,
      .dev_params = {.channel = 0x5, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static const tPconChanConfig fireflyPcon1Channels[] =
{
[0]= {"J2CP1_SRD_0V75_P1", 770, 1, ((tPconChan)-1) },
[1]= {"J2CP1_SRD_0V75_P2", 0xffff, 0, 0 },
[2]= {"J2CP1_SRD_0V75_P3", 0xffff, 0, 0 },
[3]= {"J2CP1_SRD_PLL0V75_P1", 770, 1, ((tPconChan)-1) },
[4]= {"J2CP1_SRD_1V2_P1", 1200, 1, ((tPconChan)-1) },
[5]= {"J2CP1_HBM0_VDD1V2_P1", 1200, 1, ((tPconChan)-1) },
[6]= {"J2CP1_HBM1_VDD1V2_P1", 1200, 1, ((tPconChan)-1) },
[7]= {"J2CP1_VDD3_3_P1", 3300, 1, ((tPconChan)-1) },
[8]= {NULL, 0xffff, 1, ((tPconChan)-1) },
[9]= {"VDD5_0_P1", 5000, 1, ((tPconChan)-1) },
[10]= {"VDD1_0_P1", 1000, 1, ((tPconChan)-1) },
[11]= {"VDD1_8_P1", 1800, 1, ((tPconChan)-1) },
[12]= {"VDD3_3_P1", 3300, 1, ((tPconChan)-1) },
[13]= {"VDD3_3_S5_P1", 3300, 1, ((tPconChan)-1) },
[14]= {"VDD1_8_S5_P1", 1800, 1, ((tPconChan)-1) },
[15]= {"CPU_VDD_DDR_P1", 1210, 1, ((tPconChan)-1) },
};
static tPconRailConfig fireflyPcon1Rails[] =
{
[0]= {"J2CP1_SRD_0V75", 0, 0},
[1]= {"J2CP1_SRD_PLL0V75", 3, 0},
[2]= {"J2CP1_SRD_1V2", 4, 0},
[3]= {"J2CP1_HBM0_VDD1V2", 5, 0},
[4]= {"J2CP1_HBM1_VDD1V2", 6, 0},
[5]= {"J2CP1_VDD3_3", 7, 0},
[6]= {"VDD5_0", 9, 0},
[7]= {"VDD1_0", 10, 0},
[8]= {"VDD1_8", 11, 0},
[9]= {"VDD3_3", 12, 0},
[10]= {"VDD3_3_S5", 13, 0},
[11]= {"VDD1_8_S5", 14, 0},
[12]= {"CPU_VDD_DDR", 15, 0},
};
tPconConfig fireflyPcon1 =
{
      .channels = fireflyPcon1Channels,
      .rails = fireflyPcon1Rails,
      .channelCount = (unsigned int)(sizeof (fireflyPcon1Channels) / sizeof ((fireflyPcon1Channels) [0])),
      .railCount = (unsigned int)(sizeof (fireflyPcon1Rails) / sizeof ((fireflyPcon1Rails) [0])),
};
static tPconDeviceProfile fireflyPcon1Profile =
{
      .name = "FIREFLY PCON 1",
      .desc = "located on CPUCTL",
      .fpga_id = CTL_FPGA_CPUCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (2),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 1,
      .dev_params = {.channel = 0x13, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static const tPconChanConfig fireflyPcon3Channels[] =
{
[0]= {"OPT_QSFP28_VDD_P1", 3325, 1, ((tPconChan)-1) },
[1]= {"OPT_QSFP28_VDD_P2", 0xffff, 0, 0 },
[2]= {"OPT_QSFP28_VDD_P3", 0xffff, 0, 0 },
[3]= {"OPT_QSFPDD_VDD_P1", 3325, 1, ((tPconChan)-1) },
[4]= {"OPT_QSFPDD_VDD_P2", 0xffff, 0, 3 },
[5]= {"OPT_QSFPDD_VDD_P3", 0xffff, 0, 3 },
[6]= {"OPT_QSFPDD_VDD_P4", 0xffff, 0, 3 },
[7]= {"PHY_G1_AVDD0P8_P1", 800, 1, ((tPconChan)-1) },
[8]= {"PHY_G1_AVDD0P8_P2", 0xffff, 0, 7 },
[9]= {"PHY_G2_AVDD0P8_P1", 800, 1, ((tPconChan)-1) },
[10]= {"PHY_G2_AVDD0P8_P2", 0xffff, 0, 9 },
[11]= {"PHY_G1_DVDD0P8_P1", 800, 1, ((tPconChan)-1) },
[12]= {"PHY_G1_DVDD0P8_P2", 0xffff, 0, 11 },
[13]= {"PHY_G2_DVDD0P8_P1", 800, 1, ((tPconChan)-1) },
[14]= {"PHY_G2_DVDD0P8_P2", 0xffff, 0, 13 },
[15]= {"PHY_AVDD1P0_P1", 1000, 1, ((tPconChan)-1) },
};
static tPconRailConfig fireflyPcon3Rails[] =
{
[0]= {"OPT_QSFP28_VDD", 0, 0},
[1]= {"OPT_QSFPDD_VDD", 3, 0},
[2]= {"PHY_G1_AVDD0P8", 7, 0},
[3]= {"PHY_G2_AVDD0P8", 9, 0},
[4]= {"PHY_G1_DVDD0P8", 11, 0},
[5]= {"PHY_G2_DVDD0P8", 13, 0},
[6]= {"PHY_AVDD1P0", 15, 0},
};
tPconConfig fireflyPcon3 =
{
      .channels = fireflyPcon3Channels,
      .rails = fireflyPcon3Rails,
      .channelCount = (unsigned int)(sizeof (fireflyPcon3Channels) / sizeof ((fireflyPcon3Channels) [0])),
      .railCount = (unsigned int)(sizeof (fireflyPcon3Rails) / sizeof ((fireflyPcon3Rails) [0])),
};
static tPconDeviceProfile fireflyPcon3Profile =
{
      .name = "FIREFLY PCON 3",
      .desc = "located on IOCTL",
      .fpga_id = CTL_FPGA_IOCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (4),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 2,
      .dev_params = {.channel = 0x8, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static std::array<tPconDevice, 3> fireflyPconDevices
{
      fireflyPcon0Profile, fireflyPcon0,
      fireflyPcon1Profile, fireflyPcon1,
      fireflyPcon3Profile, fireflyPcon3,
};
static const tPconChanConfig saltydogPcon0Channels[] =
{
[0]= {"J2CP1_VDDC_P1", 800, 1, ((tPconChan)-1) },
[1]= {"J2CP1_VDDC_P2", 0xffff, 0, 0 },
[2]= {"J2CP1_VDDC_P3", 0xffff, 0, 0 },
[3]= {"J2CP1_VDDC_P4", 0xffff, 0, 0 },
[4]= {"J2CP1_VDDC_P5", 0xffff, 0, 0 },
[5]= {"J2CP1_VDDC_P6", 0xffff, 0, 0 },
[6]= {"J2CP1_VDDC_P7", 0xffff, 0, 0 },
[7]= {"J2CP1_VDDC_P8", 0xffff, 0, 0 },
[8]= {"J2CP1_VDDC_P9", 0xffff, 0, 0 },
[9]= {"J2CP1_VDDC_P10", 0xffff, 0, 0 },
[10]= {"J2CP1_VDDC_P11", 0xffff, 0, 0 },
[11]= {"J2CP1_VDDC_P12", 0xffff, 0, 0 },
[12]= {"J2CP1_VDDC_P13", 0xffff, 0, 0 },
[13]= {"J2CP1_VDDC_P14", 0xffff, 0, 0 },
[14]= {"J2CP1_VDDC_P15", 0xffff, 0, 0 },
[15]= {"J2CP1_VDDC_P16", 0xffff, 0, 0 },
};
static tPconRailConfig saltydogPcon0Rails[] =
{
[0]= {"J2CP1_VDDC", 0, 0},
};
tPconConfig saltydogPcon0 =
{
      .channels = saltydogPcon0Channels,
      .rails = saltydogPcon0Rails,
      .channelCount = (unsigned int)(sizeof (saltydogPcon0Channels) / sizeof ((saltydogPcon0Channels) [0])),
      .railCount = (unsigned int)(sizeof (saltydogPcon0Rails) / sizeof ((saltydogPcon0Rails) [0])),
};
static tPconDeviceProfile saltydogPcon0Profile =
{
      .name = "SALTYDOG PCON 0",
      .desc = "located on IOCTL",
      .fpga_id = CTL_FPGA_IOCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (1),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 0,
      .dev_params = {.channel = 0x5, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static const tPconChanConfig saltydogPcon1Channels[] =
{
[0]= {"J2CP1_SRD_0V75_P1", 769, 1, ((tPconChan)-1) },
[1]= {"J2CP1_SRD_0V75_P2", 0xffff, 0, 0 },
[2]= {"J2CP1_SRD_0V75_P3", 0xffff, 0, 0 },
[3]= {"J2CP1_SRD_PLL0V75_P1", 769, 1, ((tPconChan)-1) },
[4]= {"J2CP1_SRD_1V2_P1", 1220, 1, ((tPconChan)-1) },
[5]= {"J2CP1_HBM0_VDD1V2_P1", 1200, 1, ((tPconChan)-1) },
[6]= {"J2CP1_HBM1_VDD1V2_P1", 1200, 1, ((tPconChan)-1) },
[7]= {"J2CP1_VDD3V3_P1", 3300, 1, ((tPconChan)-1) },
[8]= {"J2CP2_SRD_0V75_P1", 769, 1, ((tPconChan)-1) },
[9]= {"J2CP2_SRD_0V75_P2", 0xffff, 0, 8 },
[10]= {"J2CP2_SRD_0V75_P3", 0xffff, 0, 8 },
[11]= {"J2CP2_SRD_PLL0V75_P1", 769, 1, ((tPconChan)-1) },
[12]= {"J2CP2_SRD_1V2_P1", 1220, 1, ((tPconChan)-1) },
[13]= {"J2CP2_HBM0_VDD1V2_P1", 1200, 1, ((tPconChan)-1) },
[14]= {"J2CP2_HBM1_VDD1V2_P1", 1200, 1, ((tPconChan)-1) },
[15]= {"J2CP2_VDD3V3_P1", 3300, 1, ((tPconChan)-1) },
};
static tPconRailConfig saltydogPcon1Rails[] =
{
[0]= {"J2CP1_SRD_0V75", 0, 0},
[1]= {"J2CP1_SRD_PLL0V75", 3, 0},
[2]= {"J2CP1_SRD_1V2", 4, 0},
[3]= {"J2CP1_HBM0_VDD1V2", 5, 0},
[4]= {"J2CP1_HBM1_VDD1V2", 6, 0},
[5]= {"J2CP1_VDD3V3", 7, 0},
[6]= {"J2CP2_SRD_0V75", 8, 0},
[7]= {"J2CP2_SRD_PLL0V75", 11, 0},
[8]= {"J2CP2_SRD_1V2", 12, 0},
[9]= {"J2CP2_HBM0_VDD1V2", 13, 0},
[10]= {"J2CP2_HBM1_VDD1V2", 14, 0},
[11]= {"J2CP2_VDD3V3", 15, 0},
};
tPconConfig saltydogPcon1 =
{
      .channels = saltydogPcon1Channels,
      .rails = saltydogPcon1Rails,
      .channelCount = (unsigned int)(sizeof (saltydogPcon1Channels) / sizeof ((saltydogPcon1Channels) [0])),
      .railCount = (unsigned int)(sizeof (saltydogPcon1Rails) / sizeof ((saltydogPcon1Rails) [0])),
};
static tPconDeviceProfile saltydogPcon1Profile =
{
      .name = "SALTYDOG PCON 1",
      .desc = "located on IOCTL",
      .fpga_id = CTL_FPGA_IOCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (2),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 1,
      .dev_params = {.channel = 0x6, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static const tPconChanConfig saltydogPcon2Channels[] =
{
[0]= {"J2CP2_VDDC_P1", 800, 1, ((tPconChan)-1) },
[1]= {"J2CP2_VDDC_P2", 0xffff, 0, 0 },
[2]= {"J2CP2_VDDC_P3", 0xffff, 0, 0 },
[3]= {"J2CP2_VDDC_P4", 0xffff, 0, 0 },
[4]= {"J2CP2_VDDC_P5", 0xffff, 0, 0 },
[5]= {"J2CP2_VDDC_P6", 0xffff, 0, 0 },
[6]= {"J2CP2_VDDC_P7", 0xffff, 0, 0 },
[7]= {"J2CP2_VDDC_P8", 0xffff, 0, 0 },
[8]= {"J2CP2_VDDC_P9", 0xffff, 0, 0 },
[9]= {"J2CP2_VDDC_P10", 0xffff, 0, 0 },
[10]= {"J2CP2_VDDC_P11", 0xffff, 0, 0 },
[11]= {"J2CP2_VDDC_P12", 0xffff, 0, 0 },
[12]= {"J2CP2_VDDC_P13", 0xffff, 0, 0 },
[13]= {"J2CP2_VDDC_P14", 0xffff, 0, 0 },
[14]= {"J2CP2_VDDC_P15", 0xffff, 0, 0 },
[15]= {"J2CP2_VDDC_P16", 0xffff, 0, 0 },
};
static tPconRailConfig saltydogPcon2Rails[] =
{
[0]= {"J2CP2_VDDC", 0, 0},
};
tPconConfig saltydogPcon2 =
{
      .channels = saltydogPcon2Channels,
      .rails = saltydogPcon2Rails,
      .channelCount = (unsigned int)(sizeof (saltydogPcon2Channels) / sizeof ((saltydogPcon2Channels) [0])),
      .railCount = (unsigned int)(sizeof (saltydogPcon2Rails) / sizeof ((saltydogPcon2Rails) [0])),
};
static tPconDeviceProfile saltydogPcon2Profile =
{
      .name = "SALTYDOG PCON 2",
      .desc = "located on IOCTL",
      .fpga_id = CTL_FPGA_IOCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (3),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 2,
      .dev_params = {.channel = 0x7, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static const tPconChanConfig saltydogPcon4Channels[] =
{
[0]= {"OPT_G1_VDD_P1", 3330, 1, ((tPconChan)-1) },
[1]= {"OPT_G1_VDD_P2", 0xffff, 0, 0 },
[2]= {"OPT_G1_VDD_P3", 0xffff, 0, 0 },
[3]= {"OPT_G2_VDD_P1", 3330, 1, ((tPconChan)-1) },
[4]= {"OPT_G2_VDD_P2", 0xffff, 0, 3 },
[5]= {"OPT_G2_VDD_P3", 0xffff, 0, 3 },
[6]= {"OPT_G3_VDD_P1", 3330, 1, ((tPconChan)-1) },
[7]= {"OPT_G3_VDD_P2", 0xffff, 0, 6 },
[8]= {"OPT_G3_VDD_P3", 0xffff, 0, 6 },
[9]= {"VDD5_0_P1", 5000, 1, ((tPconChan)-1) },
[10]= {"VDD1_0_P1", 1000, 1, ((tPconChan)-1) },
[11]= {"VDD1_8_P1", 1800, 1, ((tPconChan)-1) },
[12]= {"VDD3_3_P1", 3300, 1, ((tPconChan)-1) },
[13]= {"VDD3_3_S5_P1", 3300, 1, ((tPconChan)-1) },
[14]= {"VDD1_8_S5_P1", 1800, 1, ((tPconChan)-1) },
[15]= {"CPU_VDD_DDR_P1", 1210, 1, ((tPconChan)-1) },
};
static tPconRailConfig saltydogPcon4Rails[] =
{
[0]= {"OPT_G1_VDD", 0, 0},
[1]= {"OPT_G2_VDD", 3, 0},
[2]= {"OPT_G3_VDD", 6, 0},
[3]= {"VDD5_0", 9, 0},
[4]= {"VDD1_0", 10, 0},
[5]= {"VDD1_8", 11, 0},
[6]= {"VDD3_3", 12, 0},
[7]= {"VDD3_3_S5", 13, 0},
[8]= {"VDD1_8_S5", 14, 0},
[9]= {"CPU_VDD_DDR", 15, 0},
};
tPconConfig saltydogPcon4 =
{
      .channels = saltydogPcon4Channels,
      .rails = saltydogPcon4Rails,
      .channelCount = (unsigned int)(sizeof (saltydogPcon4Channels) / sizeof ((saltydogPcon4Channels) [0])),
      .railCount = (unsigned int)(sizeof (saltydogPcon4Rails) / sizeof ((saltydogPcon4Rails) [0])),
};
static tPconDeviceProfile saltydogPcon4Profile =
{
      .name = "SALTYDOG PCON 4",
      .desc = "located on CPUCTL",
      .fpga_id = CTL_FPGA_CPUCTL,
      .regI2cAddr = 0xe8,
      .promI2cAddr = 0xe6,
      .spiChannel = (2),
      .mini = 1,
      .spiIfInit = 1,
      .resetBit = 10,
      .resetReg = (0x02700000+0x08),
      .spiTimer = (6),
      .index = 3,
      .dev_params = {.channel = 0x13, .device = 0xe8, .blksz = 0, .maxsz = 2, .speed = 1, .devclass = I2C_CLASS_UNKNOWN}
};
static std::array<tPconDevice, 4> saltydogPconDevices
{
      saltydogPcon0Profile, saltydogPcon0,
      saltydogPcon1Profile, saltydogPcon1,
      saltydogPcon2Profile, saltydogPcon2,
      saltydogPcon4Profile, saltydogPcon4,
};
using namespace std;
using namespace srlinux::platform;
using namespace srlinux::platform::spi;
static tPconEvent cardPconEventInfo[PCON_MAX_DEVICES_PER_IOCTRL];
static tPconEvent *hwPconGetPconEventInfo(HwInstance instance, uint8_t idx)
{
    tPconEvent *event;
    switch (instance.id)
    {
        case HW_INSTANCE_CARD:
            event = &cardPconEventInfo[idx];
            break;
    }
    return event;
}
const tPconAccessApi default_access_api =
{
    .hwSpiRead8 = spiRead8,
    .hwSpiReadBlock = spiReadBlock,
    .hwSpiWrite8 = spiWrite8,
    .hwSpiWrite8ReadBlock = spiWrite8BlockRead,
    .hwSpiWriteBlock = spiWriteBlock,
};
const tPconAccessApi* hwPconGetAccessApis(HwInstance instance)
{
    const tPconAccessApi* access_api = &default_access_api;
    return access_api;
}
int hwPconGetCardPconInfo(HwInstance instance, tPconDevice ** card_info)
{
    int size = 0;
    switch (instance.id)
    {
        case HW_INSTANCE_CARD:
        {
            switch (instance.card.card_type)
            {
                case 0x1b:
                    *card_info = saltydogPconDevices.data();
                    size = saltydogPconDevices.size();
                    break;
                case 0x20:
                    *card_info = fireflyPconDevices.data();
                    size = fireflyPconDevices.size();
                    break;
                case 0x3c:
                    *card_info = caribouPconDevices.data();
                    size = caribouPconDevices.size();
                    break;
                default:
                    size = 0;
                    break;
            }
        }
        break;
        default:
            printf("Invalid hw_instance id %d" "\n", instance.id);
            break;
    }
    return size;
}
I2CCtrlr hwPconGetI2CCtrlr(HwInstance instance, const tPconDevice& card_info)
{
    I2CCtrlr ctrlr;
    ctrlr.fpga_id = card_info.dev.fpga_id;
    switch (instance.id)
    {
        case HW_INSTANCE_CARD:
            ctrlr.is_remote = 0;
            ctrlr.hw_slot = 0;
            break;
        default:
            printf("Invalid instance id %d" "\n", instance.id);
            break;
    }
    return ctrlr;
}
tPconDevice * hwPconGetPconInfo(HwInstance instance, uint32_t index, bool log_on_failure)
{
    tPconDevice *pcon_info;
    if (int size = hwPconGetCardPconInfo(instance, &pcon_info))
    {
        for (int i = 0; i < size; i++)
        {
            if (pcon_info[i].dev.index == index)
            {
                return &pcon_info[i];
            }
        }
        if (log_on_failure)
        {
            switch (instance.id)
            {
                case HW_INSTANCE_CARD:
                    printf("Invalid index %u for card %u PCON" "\n", index, instance.card.card_type);
                    break;
                default:
                    break;
            }
        }
    }
    return nullptr;
}
const tPconDeviceProfile * hwPconGetProfile(const tPconDevice * dev)
{
    return (dev ? &dev->dev : NULL);
}
bool hwPconIsMini(HwInstance instance, uint32_t index)
{
    const tPconDeviceProfile *pcon_profile = hwPconGetProfile(hwPconGetPconInfo(instance, index));
    return (pcon_profile ? pcon_profile->mini : 1);
}
void hwPconShowDevices(HwInstance instance, bool verbose)
{
    tPconDevice *pcon_info;
    if (int size = hwPconGetCardPconInfo(instance, &pcon_info))
    {
        for (int i = 0; i < size; i++)
        {
            printf("\nDevice Index %u => Name: %s  IsMini: %s   Description: %s\n", i, pcon_info[i].dev.name, pcon_info[i].dev.mini ? "yes" : "no",
                   pcon_info[i].dev.desc);
            uint32_t imbv_milli_volt = 0;
            hwPconGetInputVoltage(instance, pcon_info[i].dev.index, &imbv_milli_volt);
            printf("IMBV bus voltage = %u millivolt\n", imbv_milli_volt);
            if (verbose)
                hwPconShowRailVoltages(instance, i, 0);
        }
    }
}
std::string hwPconGetDevices(HwInstance instance, bool verbose)
{
    tPconDevice *pcon_info;
    std::string output;
    if (int size = hwPconGetCardPconInfo(instance, &pcon_info))
    {
        for (int i = 0; i < size; i++)
        {
            output.append(fmt::format("\nDevice Index {} => Name: {}  IsMini: {}   Description: {}\n", i, pcon_info[i].dev.name, pcon_info[i].dev.mini ? "yes" : "no",
                   pcon_info[i].dev.desc));
            uint32_t imbv_milli_volt = 0;
            hwPconGetInputVoltage(instance, pcon_info[i].dev.index, &imbv_milli_volt);
            output.append(fmt::format("IMBV bus voltage = {} millivolt\n", imbv_milli_volt));
            if (verbose)
                output.append(hwPconGetRailVoltages(instance, i, 0));
        }
    }
    return output;
}
uint32_t hwPconRailApplyScaleFactor(bool scale_up, uint32_t conf_mvolt, uint32_t meas_mvolt)
{
    uint32_t scale_num;
    uint32_t scale_den;
    uint32_t multiplier;
    uint32_t divisor;
    if (conf_mvolt >= 4500)
    {
        scale_num = (1);
        scale_den = (2);
    }
    else if (conf_mvolt > (3000))
    {
        scale_num = (332);
        scale_den = (432);
    }
    else
    {
        scale_num = 1;
        scale_den = 1;
    }
    if (scale_up)
    {
        multiplier = scale_den;
        divisor = scale_num;
    }
    else
    {
        multiplier = scale_num;
        divisor = scale_den;
    }
    uint32_t scaled_value_int = (meas_mvolt * multiplier) / divisor;
    uint32_t scaled_value_frac = (((meas_mvolt * multiplier) % divisor) + (divisor >> 1)) / divisor;
    return (scaled_value_int + scaled_value_frac);
}
SrlStatus pconGetMiscInfo(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, tPconChan chan, bool * enable, bool * master, tPconChan * slaveTo)
{
    uint16_t miscReg;
    if (pconReadChanReg(ctrlr, pDev, chan, 0x3A, &miscReg) == 0)
    {
        *master = (bool)((miscReg & 0x01) >> 0);
        *enable = (bool)((miscReg & 0x02) >> 1);
        *slaveTo = (tPconChan)((miscReg & 0xff00) >> 8);
        return 0;
    }
    return (-1);
}
SrlStatus hwPconReadChannelVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, tPconChan chan, uint32_t * pconVoltage32, tPconChanConfig pconChanConfig, bool verbose)
{
    uint32_t voltage32 = 0;
    uint16_t hwVoltage;
    uint32_t conf_mvolt;
    if (pconReadChanReg(ctrlr, pDev, chan, 0x1C, &hwVoltage) == 0)
    {
        if (verbose)
            printf ("channel %u:  hwVoltage 0x%04x ", chan, hwVoltage);
        voltage32 = hwVoltage;
        conf_mvolt = pconChanConfig.voltage;
        voltage32 *= 3000;
        if (verbose)
            printf ("mult VREF 0x%08x ", voltage32);
        voltage32 /= 1024;
        if (verbose)
            printf ("div 0x%08x ", voltage32);
        if (conf_mvolt > (3000))
            voltage32 = hwPconRailApplyScaleFactor(1, conf_mvolt, voltage32);
        if (verbose)
            printf ("answer %umV\n", voltage32);
        if (pconVoltage32) *pconVoltage32 = voltage32;
        return 0;
    }
    return (-1);
}
SrlStatus hwPconReadChannelCurrent(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, tPconChan chan, uint32_t * current, bool verbose = 0)
{
    uint16_t hwCurrent;
    uint16_t multipliers;
    uint8_t numerator = 0;
    uint8_t denominator=1;
    uint64_t current64 = 0;
    uint32_t samples[16];
    uint32_t sample;
    *current = 0;
    if (pconReadChanReg(ctrlr, pDev, chan, 0x20, &multipliers) != 0)
        return (-1);
    numerator = (multipliers & 0xff00) >> 8;
    denominator = (multipliers & 0x00ff) >> 0;
    for (sample = 0; sample < (unsigned int)(sizeof (samples) / sizeof ((samples) [0])); sample++)
    {
        if (pconReadChanReg(ctrlr, pDev, chan, 0x1E, &hwCurrent) != 0)
            return (-1);
        current64 = (0x1AB <= hwCurrent) ? (hwCurrent - 0x1AB) : 0ULL;
        if (verbose) printf ("sub iOffs 0x%04lx ", current64);
        current64 *= (3000 * 1000);
        if (verbose) printf ("mult VREF*1000 0x%lx ", current64);
        current64 *= numerator;
        current64 /= denominator;
        if (verbose) printf ("mult(%u/%u) 0x%lx ", numerator, denominator, current64);
        current64 /= 1024;
        if (verbose) printf ("div 0x%lx ", current64);
        if (verbose) printf ("answer %lumA\n", current64);
        samples[sample] = (uint32_t)current64;
    }
    for (sample = 0; sample < (unsigned int)(sizeof (samples) / sizeof ((samples) [0])); sample++)
        *current += samples[sample];
    if (verbose) printf ("Current SUM %umA\n", *current);
    *current /= (unsigned int)(sizeof (samples) / sizeof ((samples) [0]));
    if (verbose) printf ("answer %umA\n", *current);
    return 0;
}
SrlStatus hwPconGetMeasuredCurrent(I2CCtrlr & ctrlr, const tPconDevice & pconDevConfig, uint32_t rail_num, uint32_t & current)
{
    SrlStatus status = 0;
    if (rail_num >= pconDevConfig.config.railCount)
    {
        printf("%s: Rail %u is out of range" "\n", pconDevConfig.dev.name, rail_num);
        return (-1);
    }
    current = 0;
    for (uint32_t chan = 0; chan < pconDevConfig.config.channelCount; chan++)
    {
        if (pconDevConfig.config.channels[chan].name)
        {
            if ((chan == pconDevConfig.config.rails[rail_num].masterChan) ||
                (pconDevConfig.config.channels[chan].masterChan == pconDevConfig.config.rails[rail_num].masterChan))
            {
                uint32_t channelCurrent = 0;
                if ((status |= hwPconReadChannelCurrent(&ctrlr, &pconDevConfig.dev.dev_params, chan, &channelCurrent)) == 0)
                    current += channelCurrent;
            }
        }
    }
    return status;
}
void hwPconShowChannels(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint16_t idx, tPconConfig pconConfig)
{
    uint8_t spiChannel = pDev->channel;
    uint16_t version;
    pconReadGlobalReg(ctrlr, pDev, 0x00, &version);
    printf("Versions %x\r\n", version);
    printf("PCON Device %02d  SPI Channel %02d\n", idx, spiChannel);
    printf ("%-4s %-8s %-36s %-8s %-8s %-12s %-12s %-12s\n", "SPI", "CHANNEL", "NAME", "ENABLE", "MASTER", "SLAVE TO", "VOLTAGE", "CURRENT");
    printf ("%-4s %-8s %-36s %-8s %-8s %-12s %-12s %-12s\n", "====", "=======", "====", "======", "======", "========", "=======", "=======");
    int i;
    bool enable;
    bool master;
    tPconChan slaveTo;
    for (i = 0; i < pconConfig.channelCount; i++)
    {
        if (pconConfig.channels[i].name)
        {
            uint32_t current = 0;
            uint32_t voltage = 0;
            if (pconGetMiscInfo(ctrlr, pDev, i, &enable, &master, &slaveTo) != 0)
            {
                break;
            }
            if (pconConfig.channels[i].name == NULL) continue;
            if (pconConfig.channels[i].master == 1)
            {
                hwPconReadChannelVoltage(ctrlr, pDev, i, &voltage, pconConfig.channels[i], 0);
            }
            hwPconReadChannelCurrent(ctrlr, pDev, i, &current);
            char voltage_str[20] = { '\0' };
            char current_str[20] = { '\0' };
            if (master != 1)
                snprintf(voltage_str, sizeof(voltage_str), "%7s", "N/A");
            else
                snprintf(voltage_str, sizeof(voltage_str), "%5umV", voltage);
            snprintf(current_str, sizeof(current_str), "%5umA", current);
            printf ("%02u%2s %02u%6s %-36s %u%7s %u%7s %2u%10s %-12s %-12s\n",
                    spiChannel, " ", i, " ", pconConfig.channels[i].name, enable, " ", master, " ", slaveTo, " ",
                    voltage_str, current_str);
        }
    }
}
SrlStatus hwPconSetTargetVoltageInt(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t milli_volt)
{
    SrlStatus status = 0;
    uint16_t trim_allow;
    uint32_t mvolt_trim_allow;
    uint16_t volt_reg, inv_volt_reg, cur_volt_reg;
    uint32_t chan_num, conf_mvolt;
    if ((status = getChannelInfo(pconConfig, idx, rail_num, &chan_num, &conf_mvolt)) == 0)
    {
        tPconRailConfig * rail_config = pconConfig->rails;
        if ((status = pconReadChanReg(ctrlr, pDev, chan_num, 0x2C, &trim_allow)) == 0)
        {
            mvolt_trim_allow = ((3000) * (trim_allow & 0xff)) / (1 << 10);
            if ((milli_volt > (conf_mvolt + mvolt_trim_allow)) || (milli_volt < (conf_mvolt - mvolt_trim_allow)))
            {
                printf("Target voltage %u mv is not allowed on rail %s, max trim +/-%u" "\n",
                               milli_volt, rail_config[rail_num].name, mvolt_trim_allow);
                return (-1);
            }
            if (conf_mvolt > (3000))
                milli_volt = hwPconRailApplyScaleFactor(0, conf_mvolt, milli_volt);
            volt_reg = ((((milli_volt + rail_config[rail_num].voltOffset) * (1 << 10)) / (3000)) + (((((milli_volt + rail_config[rail_num].voltOffset) * (1 << 10)) % (3000)) + ((3000) >> 1)) / (3000)));
            inv_volt_reg = ~volt_reg;
            status = pconReadChanReg(ctrlr, pDev, chan_num, 0x12, &cur_volt_reg);
            if (status == 0)
            {
                int req_reg_change = volt_reg - cur_volt_reg;
                int num_steps = ((req_reg_change) < 0 ? -(req_reg_change) : (req_reg_change)) / (3);
                int change_last_step = req_reg_change % (3);
                for(; num_steps > 0; num_steps--)
                {
                    if (req_reg_change < 0)
                        cur_volt_reg -= (3);
                    else
                        cur_volt_reg += (3);
                    inv_volt_reg = ~cur_volt_reg;
                    status |= pconWriteChanReg(ctrlr, pDev, chan_num, 0x10, &inv_volt_reg);
                    status |= pconWriteChanReg(ctrlr, pDev, chan_num, 0x12, &cur_volt_reg);
                }
                if (change_last_step != 0)
                {
                    cur_volt_reg += change_last_step;
                    inv_volt_reg = ~cur_volt_reg;
                    status |= pconWriteChanReg(ctrlr, pDev, chan_num, 0x10, &inv_volt_reg);
                    status |= pconWriteChanReg(ctrlr, pDev, chan_num, 0x12, &cur_volt_reg);
                }
            }
            else
            {
                status = pconWriteChanReg(ctrlr, pDev, chan_num, 0x10, &inv_volt_reg);
                status |= pconWriteChanReg(ctrlr, pDev, chan_num, 0x12, &volt_reg);
            }
        }
    }
    return status;
}
SrlStatus hwPconSetUnderVoltageInt(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t milli_volt)
{
    SrlStatus status = 0;
    uint16_t volt_reg, inv_volt_reg;
    uint32_t chan_num, conf_mvolt, cur_mvolt;
    if ((status = getChannelInfo(pconConfig, idx, rail_num, &chan_num, &conf_mvolt)) == 0)
    {
        tPconRailConfig * rail_config = pconConfig->rails;
        if ((status = hwPconGetMeasuredVoltage(ctrlr, pDev, idx, pconConfig, rail_num, &cur_mvolt)) == 0)
        {
            do { uint32_t set_mvolt = 0; hwPconGetConfiguredVoltage(ctrlr, pDev, idx, pconConfig, rail_num, &set_mvolt); if ((set_mvolt > 0) && (set_mvolt <= (750))) { milli_volt = set_mvolt; (1) ? (milli_volt -= (60)) : (milli_volt += (60)); } } while(0);
            if (cur_mvolt <= (milli_volt + 5))
            {
                printf("Ignoring request under voltage limit %u mv on rail %s" "\n", milli_volt, rail_config[rail_num].name);
                return (-1);
            }
            if (conf_mvolt > (3000))
                milli_volt = hwPconRailApplyScaleFactor(0, conf_mvolt, milli_volt);
            volt_reg = ((((milli_volt) * (1 << 10)) / (3000)) + (((((milli_volt) * (1 << 10)) % (3000)) + ((3000) >> 1)) / (3000)));
            inv_volt_reg = ~volt_reg;
            status = pconWriteChanReg(ctrlr, pDev, chan_num, 0x14, &inv_volt_reg);
            status |= pconWriteChanReg(ctrlr, pDev, chan_num, 0x16, &volt_reg);
        }
    }
    return status;
}
SrlStatus hwPconSetOverVoltageInt(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t milli_volt)
{
    SrlStatus status = 0;
    uint16_t volt_reg, inv_volt_reg;
    uint32_t chan_num, conf_mvolt, cur_mvolt;
    if ((status = getChannelInfo(pconConfig, idx, rail_num, &chan_num, &conf_mvolt)) == 0)
    {
        tPconRailConfig * rail_config = pconConfig->rails;
        if ((status = hwPconGetMeasuredVoltage(ctrlr, pDev, idx, pconConfig, rail_num, &cur_mvolt)) == 0)
        {
            do { uint32_t set_mvolt = 0; hwPconGetConfiguredVoltage(ctrlr, pDev, idx, pconConfig, rail_num, &set_mvolt); if ((set_mvolt > 0) && (set_mvolt <= (750))) { milli_volt = set_mvolt; (0) ? (milli_volt -= (60)) : (milli_volt += (60)); } } while(0);
            if ((cur_mvolt + 5) >= milli_volt)
            {
                printf("Ignoring request over voltage limit %u mv exceeds current %u mv on rail %s" "\n", milli_volt, cur_mvolt, rail_config[rail_num].name);
                return (-1);
            }
            if (conf_mvolt > (3000))
                milli_volt = hwPconRailApplyScaleFactor(0, conf_mvolt, milli_volt);
            volt_reg = ((((milli_volt) * (1 << 10)) / (3000)) + (((((milli_volt) * (1 << 10)) % (3000)) + ((3000) >> 1)) / (3000)));
            inv_volt_reg = ~volt_reg;
            status = pconWriteChanReg(ctrlr, pDev, chan_num, 0x18, &inv_volt_reg);
            status |= pconWriteChanReg(ctrlr, pDev, chan_num, 0x1A, &volt_reg);
        }
    }
    return status;
}
SrlStatus hwPconGetMeasuredVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig,
                                   uint32_t rail_num, uint32_t *milli_volt)
{
    SrlStatus status = 0;
    uint16_t volt_reg;
    uint32_t chan_num, conf_mvolt;
    if (milli_volt == NULL)
    {
        return (-1);
    }
    if ((status = getChannelInfo(pconConfig, idx, rail_num, &chan_num, &conf_mvolt)) == 0)
    {
        if ((status = pconReadChanReg(ctrlr, pDev, chan_num, 0x1C, &volt_reg)) == 0)
        {
            *milli_volt = ((((volt_reg & ((1 << 10) - 1)) * (3000)) / (1 << 10)) + (((((volt_reg & ((1 << 10) - 1)) * (3000)) % (1 << 10)) + ((1 << 10) >> 1)) / (1 << 10)));
            if (conf_mvolt > (3000))
                *milli_volt = hwPconRailApplyScaleFactor(1, conf_mvolt, *milli_volt);
        }
    }
    return status;
}
SrlStatus hwPconGetConfiguredVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t *milli_volt)
{
    SrlStatus status = 0;
    uint16_t volt_reg;
    uint32_t chan_num, conf_mvolt;
    if (milli_volt == NULL)
    {
        return (-1);
    }
    if ((status = getChannelInfo(pconConfig, idx, rail_num, &chan_num, &conf_mvolt)) == 0)
    {
        if ((status = pconReadChanReg(ctrlr, pDev, chan_num, 0x12, &volt_reg)) == 0)
        {
            tPconRailConfig * rail_config = pconConfig->rails;
            *milli_volt = ((((volt_reg & ((1 << 10) - 1)) * (3000)) / (1 << 10)) + (((((volt_reg & ((1 << 10) - 1)) * (3000)) % (1 << 10)) + ((1 << 10) >> 1)) / (1 << 10))) - rail_config[rail_num].voltOffset;
            if (conf_mvolt > (3000))
                *milli_volt = hwPconRailApplyScaleFactor(1, conf_mvolt, *milli_volt);
        }
    }
    return status;
}
SrlStatus hwPconGetUnderVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, uint32_t rail_num, uint32_t *milli_volt)
{
    SrlStatus status = 0;
    uint16_t volt_reg;
    uint32_t chan_num, conf_mvolt;
    if (milli_volt == NULL)
    {
        return (-1);
    }
    if ((status = getChannelInfo(pconConfig, idx, rail_num, &chan_num, &conf_mvolt)) == 0)
    {
        if ((status = pconReadChanReg(ctrlr, pDev, chan_num, 0x16, &volt_reg)) == 0)
        {
            *milli_volt = ((((volt_reg & ((1 << 10) - 1)) * (3000)) / (1 << 10)) + (((((volt_reg & ((1 << 10) - 1)) * (3000)) % (1 << 10)) + ((1 << 10) >> 1)) / (1 << 10)));
            if (conf_mvolt > (3000))
                *milli_volt = hwPconRailApplyScaleFactor(1, conf_mvolt, *milli_volt);
        }
    }
    return status;
}
SrlStatus hwPconGetOverVoltage(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t idx, tPconConfig *pconConfig, int32_t rail_num, uint32_t *milli_volt)
{
    SrlStatus status = 0;
    uint16_t volt_reg;
    uint32_t chan_num, conf_mvolt;
    if (milli_volt == NULL)
    {
        return (-1);
    }
    if ((status = getChannelInfo(pconConfig, idx, rail_num, &chan_num, &conf_mvolt)) == 0)
    {
        if ((status = pconReadChanReg(ctrlr, pDev, chan_num, 0x1A, &volt_reg)) == 0)
        {
            *milli_volt = ((((volt_reg & ((1 << 10) - 1)) * (3000)) / (1 << 10)) + (((((volt_reg & ((1 << 10) - 1)) * (3000)) % (1 << 10)) + ((1 << 10) >> 1)) / (1 << 10)));
            if (conf_mvolt > (3000))
                *milli_volt = hwPconRailApplyScaleFactor(1, conf_mvolt, *milli_volt);
        }
    }
    return status;
}
std::string hwPconGetRailsConfigInt(HwInstance instance, I2CFpgaCtrlrDeviceParams *pDev, uint16_t device_idx, tPconConfig *pconConfig)
{
    std::string output;
    uint32_t rail_cnt = pconConfig->railCount;
    output.append(fmt::format("\nDevice Index {}, Number of Rails {}: \n", device_idx, rail_cnt));
    output.append(fmt::format("{:<10s}{:<35s}{:<16s}{:<16s}{:<16s}{:<16s}{:<16s}{:<16s}\n", "RAIL NUM", "RAIL NAME", "CONFIGURED mV", "TARGET mV", "UV THRES mV", "OV THRES mV", "MEASURED mV", "MEASURED mAmp"));
    output.append(fmt::format("{:<10s}{:<35s}{:<16s}{:<16s}{:<16s}{:<16s}{:<16s}{:<16s}\n", "========", "=========", "=============", "=========", "===========", "===========", "===========", "============="));
    for (uint32_t rail_num = 0; rail_num < rail_cnt; rail_num++)
    {
        uint32_t channel_num = 0;
        uint32_t conf_mvolt = 0;
        uint32_t target_mvolt = 0;
        uint32_t measured_mvolt = 0;
        uint32_t uv_mvolt = 0;
        uint32_t ov_mvolt = 0;
        uint32_t rail_current = 0;
        const char *rail_name = pconConfig->rails[rail_num].name;
        if (rail_name == NULL)
            continue;
        const tPconDevice *pconDevConfig = hwPconGetPconInfo(instance, device_idx);
        if (pconDevConfig == nullptr)
        {
            output.append(fmt::format("Could not get pcon device config for {} index {}", HwInstanceToString(instance), device_idx));
            return output;
        }
        I2CCtrlr ctrlr = hwPconGetI2CCtrlr(instance, *pconDevConfig);
        getChannelInfo(pconConfig, device_idx, rail_num, &channel_num, &conf_mvolt);
        hwPconGetConfiguredVoltage(&ctrlr, pDev, device_idx, pconConfig, rail_num, &target_mvolt);
        hwPconGetMeasuredVoltage(&ctrlr, pDev, device_idx, pconConfig, rail_num, &measured_mvolt);
        hwPconGetUnderVoltage(&ctrlr, pDev, device_idx, pconConfig, rail_num, &uv_mvolt);
        hwPconGetOverVoltage(&ctrlr, pDev, device_idx, pconConfig, rail_num, &ov_mvolt);
        hwPconGetMeasuredCurrent(ctrlr, *pconDevConfig, rail_num, rail_current);
        output.append(fmt::format("{:<10}{:<35s}{:<16}{:<16}{:<16}{:<16}{:<16}{:<16}\n", rail_num, rail_name, conf_mvolt, target_mvolt, uv_mvolt, ov_mvolt, measured_mvolt, rail_current));
    }
    return output;
}
void hwPconShowRailsConfigInt(HwInstance instance, I2CFpgaCtrlrDeviceParams *pDev, uint16_t device_idx, tPconConfig *pconConfig)
{
    printf("%s", hwPconGetRailsConfigInt(instance, pDev, device_idx, pconConfig).c_str());
}
void hwPconShowRailConfigAll(HwInstance instance)
{
    tPconDevice *pcon_info;
    if (int size = hwPconGetCardPconInfo(instance, &pcon_info))
    {
        for (int device_idx = 0; device_idx < size; device_idx++)
        {
            {
                hwPconShowRailsConfigInt(instance, &pcon_info[device_idx].dev.dev_params, device_idx, &pcon_info[device_idx].config);
            }
        }
    }
}
std::string hwPconGetRailConfigAll(HwInstance instance)
{
    std::string output;
    tPconDevice *pcon_info;
    if (int size = hwPconGetCardPconInfo(instance, &pcon_info))
    {
        for (int i = 0; i < size; i++)
        {
            output.append(hwPconGetRailsConfigInt(instance, &pcon_info[i].dev.dev_params, pcon_info[i].dev.index, &pcon_info[i].config));
        }
    }
    return output;
}
SrlStatus hwPconReadRailVoltage(I2CCtrlr *ctrlr, tPconDevice *pcon_info, uint32_t rail, uint32_t *voltage, bool verbose)
{
    return (hwPconReadChannelVoltage(ctrlr, &pcon_info->dev.dev_params, pcon_info->config.rails[rail].masterChan, voltage,
                                     pcon_info->config.channels[pcon_info->config.rails[rail].masterChan], verbose));
}
SrlStatus hwPconReadRailCurrent(I2CCtrlr *ctrlr, tPconDevice *pcon_info, uint32_t rail, uint32_t *current, bool verbose)
{
    uint32_t current32 = 0;
    uint32_t samples[16] = { 0 };
    for (uint32_t chan = 0; chan < pcon_info->config.channelCount; chan++)
    {
        if (pcon_info->config.channels[chan].name)
        {
            for (uint32_t sample = 0; sample < (unsigned int)(sizeof (samples) / sizeof ((samples) [0])); sample++)
            {
                if ((chan == pcon_info->config.rails[rail].masterChan) ||
                    (pcon_info->config.channels[chan].masterChan == pcon_info->config.rails[rail].masterChan))
                {
                    uint32_t channelCurrent;
                    if (hwPconReadChannelCurrent(ctrlr, &pcon_info->dev.dev_params, chan, &channelCurrent,
                                                 (verbose && (sample == 0))) != 0)
                        return (-1);
                    else
                        samples[sample] += channelCurrent;
                }
            }
        }
    }
    for (uint32_t sample = 0; sample < (unsigned int)(sizeof (samples) / sizeof ((samples) [0])); sample++)
        current32 += samples[sample];
    current32 /= (unsigned int)(sizeof (samples) / sizeof ((samples) [0]));
    if (current)
        *current = current32;
    return 0;
}
void hwPconShowRailVoltages(HwInstance instance, uint8_t idx, bool verbose)
{
    printf("%s", hwPconGetRailVoltages(instance, idx, verbose).c_str());
}
std::string hwPconGetRailVoltages(HwInstance instance, uint8_t idx, bool verbose)
{
    constexpr size_t buf_size = 8192;
    char output[buf_size];
    size_t index = 0;
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, idx);
    if (!pcon_info)
        return "";
    uint8_t spiChannel = pcon_info->dev.spiChannel;
    I2CCtrlr ctrlr = hwPconGetI2CCtrlr(instance, *pcon_info);
    int ret;
    if ((ret = snprintf(output + index, buf_size - index, "==============================\n")) == -1)
    {
        printf("hwPconGetRailVoltages: buf size not large enough\n");
        return "";
    }
    index += ret;
    if ((ret = snprintf(output + index, buf_size - index, "PCON Device %02d  SPI Channel %02d\n", idx, spiChannel)) == -1)
    {
        printf("hwPconGetRailVoltages: buf size not large enough\n");
        return "";
    }
    index += ret;
    if ((ret = snprintf(output + index, buf_size - index, "==============================\n")) == -1)
    {
        printf("hwPconGetRailVoltages: buf size not large enough\n");
        return "";
    }
    index += ret;
    if ((ret = snprintf(output + index, buf_size - index, "%-4s %-8s %-36s %-36s %-12s %-12s\n", "SPI", "RAIL", "NAME", "MASTER", "VOLTAGE", "CURRENT")) == -1)
    {
        printf("hwPconGetRailVoltages: buf size not large enough\n");
        return "";
    }
    index += ret;
    if ((ret = snprintf(output + index, buf_size - index, "%-4s %-8s %-36s %-36s %-12s %-12s\n", "===", "====", "====", "======", "=======", "=======")) == -1)
    {
        printf("hwPconGetRailVoltages: buf size not large enough\n");
        return "";
    }
    index += ret;
    for (uint32_t i = 0; i < pcon_info->config.railCount; i++)
    {
        std::string voltage;
        std::string current;
        uint32_t value;
        if (pcon_info->config.rails[i].name == NULL)
            continue;
        if (hwPconReadRailVoltage(&ctrlr, pcon_info, i, &value, verbose) == 0)
            voltage += Format("%5umV", value);
        else
            voltage += "*ERR*";
        if (hwPconReadRailCurrent(&ctrlr, pcon_info, i, &value, verbose) == 0)
            current += Format("%5umA", value);
        else
            current += "*ERR*";
        if ((ret = snprintf(output + index, buf_size - index, "%02u%2s %02u%6s %-36s %-36s %-12s %-12s\n", spiChannel, " ", i, " ", pcon_info->config.rails[i].name,
                pcon_info->config.channels[pcon_info->config.rails[i].masterChan].name, voltage.c_str(), current.c_str())) == -1)
        {
            printf("hwPconGetRailVoltages: buf size not large enough\n");
            return "";
        }
        index += ret;
    }
    return std::string { output };
}
SrlStatus hwPconShowChannelsAll(HwInstance instance)
{
    SrlStatus rc = 0;
    tPconDevice *pcon_info;
    if (int size = hwPconGetCardPconInfo(instance, &pcon_info))
    {
        for (int i = 0; i < size; i++)
        {
            I2CCtrlr ctrlr = hwPconGetI2CCtrlr(instance, pcon_info[i]);
            {
                hwPconShowChannels(&ctrlr, &pcon_info[i].dev.dev_params, pcon_info[i].dev.index, pcon_info[i].config);
            }
        }
    }
    else
        rc = (-1);
    return rc;
}
SrlStatus hwPconGetInputVoltage(HwInstance instance, uint32_t index, uint32_t *milli_volt)
{
    SrlStatus status = (-1);
    if (milli_volt == nullptr)
    {
        return status;
    }
    uint16_t imbv_reg_value = hwPconReadGlobalReg(instance, index, 0x02);
    if (imbv_reg_value == 0xffff)
    {
        *milli_volt = 0;
        status = (-1);
    }
    else
    {
        *milli_volt = ((12.49 / 2.49) * ((((imbv_reg_value & ((1 << 10) - 1)) * (3000)) / (1 << 10)) + (((((imbv_reg_value & ((1 << 10) - 1)) * (3000)) % (1 << 10)) + ((1 << 10) >> 1)) / (1 << 10))));
        status = 0;
    }
    return status;
}
SrlStatus getChannelInfo(tPconConfig *pconConfig, uint32_t idx, uint32_t rail_num, uint32_t * channel_num, uint32_t * conf_mvolt)
{
    if ((channel_num == NULL) || (conf_mvolt == NULL))
        return (-1);
    const tPconConfig * config = pconConfig;
    if (config == NULL)
        return (-1);
    if (rail_num >= config->railCount)
    {
        printf("PCON %d voltage rail number %u invalid" "\n", idx, rail_num);
        return (-1);
    }
    tPconRailConfig * rail_config = config->rails;
    uint8_t chan_num = rail_config[rail_num].masterChan;
    if (((chan_num) >= (config)->channelCount) || ((config)->channels[(chan_num)].name == NULL))
    {
        printf("PCON %d channel number %u invalid, count %d %s" "\n", idx, chan_num, config->channelCount, config->channels[chan_num].name);
        return (-1);
    }
    const tPconChanConfig * chan_config = config->channels;
    *channel_num = chan_num;
    *conf_mvolt = chan_config[chan_num].voltage;
    return 0;
}
void hwPconShowVoltageOrCurrent(HwInstance instance, uint32_t idx, uint32_t chan, bool verbose, bool voltage)
{
    tPconChan i;
    bool all = (chan >= 42);
    tPconDevice *pcon_info;
    uint32_t data;
    SrlStatus status;
    const char *str;
    char unit;
    pcon_info = hwPconGetPconInfo(instance, idx);
    if (!pcon_info)
    {
        printf("No device for index %u\n", idx);
        return;
    }
    I2CCtrlr ctrlr;
    I2CFpgaCtrlrDeviceParams dev_params = getPconI2CParams(instance, idx, &ctrlr);
    if (voltage) {str = "Voltage"; unit = 'V';}
    else {str = "Current"; unit = 'A';}
    for (i = (all ? 0 : chan); i < (all ? pcon_info->config.channelCount : chan+1); i++)
    {
        if (pcon_info->config.channels[i].name == NULL) continue;
        if (voltage)
        {
            if (pcon_info->config.channels[i].master != 1)
                continue;
            status = hwPconReadChannelVoltage(&ctrlr, &dev_params, i, &data, pcon_info->config.channels[i], verbose);
        }
        else
        {
            status = hwPconReadChannelCurrent(&ctrlr, &dev_params, i, &data, verbose);
        }
        if (status == 0)
            printf ("%s for channel %2u (%30s) is %5um%c\n", str, i, pcon_info->config.channels[i].name, data, unit);
        else
            printf ("ERROR reading %s for channel %u (%s)\n", str, i, pcon_info->config.channels[i].name);
    }
}
void hwPconShowChannelVoltage(HwInstance instance, uint32_t idx, uint32_t chan, bool verbose)
{
    hwPconShowVoltageOrCurrent(instance, idx, chan, verbose, 1);
}
void hwPconShowChannelCurrent(HwInstance instance, uint32_t idx, uint32_t chan, bool verbose)
{
    hwPconShowVoltageOrCurrent(instance, idx, chan, verbose, 0);
}
SrlStatus hwPconSetTargetVoltageSel(HwInstance instance, uint8_t index, uint32_t rail_num, uint32_t milli_volt)
{
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    if (pcon_info)
    {
        I2CCtrlr ctrlr = hwPconGetI2CCtrlr(instance, *pcon_info);
        return hwPconSetTargetVoltageInt(&ctrlr, &pcon_info->dev.dev_params, index, &pcon_info->config, rail_num, milli_volt);
    }
    else
    {
        printf("No device at index %u for %s" "\n", index, HwInstanceToString(instance).c_str());
    }
    return (-1);
}
SrlStatus hwPconSetUnderVoltageSel(HwInstance instance, uint8_t index, uint32_t rail_num, uint32_t milli_volt)
{
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    if (pcon_info)
    {
        I2CCtrlr ctrlr = hwPconGetI2CCtrlr(instance, *pcon_info);
        return hwPconSetUnderVoltageInt(&ctrlr, &pcon_info->dev.dev_params, index, &pcon_info->config, rail_num, milli_volt);
    }
    else
    {
        printf("No device at index %u for %s" "\n", index, HwInstanceToString(instance).c_str());
    }
    return (-1);
}
SrlStatus hwPconSetOverVoltageSel(HwInstance instance, uint8_t index, uint32_t rail_num, uint32_t milli_volt)
{
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    if (pcon_info)
    {
        I2CCtrlr ctrlr = hwPconGetI2CCtrlr(instance, *pcon_info);
        return hwPconSetOverVoltageInt(&ctrlr, &pcon_info->dev.dev_params, index, &pcon_info->config, rail_num, milli_volt);
    }
    else
    {
        printf("No device at index %u for %s" "\n", index, HwInstanceToString(instance).c_str());
    }
    return (-1);
}
tPconConfig getPconData(HwInstance instance, uint32_t index)
{
    if (instance.id == HW_INSTANCE_CARD && instance.card.card_type == 0x00)
    {
        instance.card.card_type = GetMyCardType();
    }
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    if (pcon_info)
    {
        return (pcon_info->config);
    }
    printf("Invalid index for %s" "\n", HwInstanceToString(instance).c_str());
    return fireflyPcon0;
}
I2CFpgaCtrlrDeviceParams getPconI2CParams(HwInstance instance, uint32_t index, I2CCtrlr *ctrlr)
{
    if (instance.id == HW_INSTANCE_CARD && instance.card.card_type == 0x00)
    {
        printf("HW_CARD_UNKNOWN passed in, getting card type" "\n");
        instance.card.card_type = GetMyCardType();
    }
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    if (pcon_info)
    {
        if (ctrlr)
        {
            switch (instance.id)
            {
                case HW_INSTANCE_CARD:
                {
                    ctrlr->fpga_id = pcon_info->dev.fpga_id;
                    ctrlr->is_remote = 0;
                    ctrlr->hw_slot = 0;
                    break;
                }
            }
        }
        return (pcon_info->dev.dev_params);
    }
    printf("invalid index for %s" "\n", HwInstanceToString(instance).c_str());
    return {0x19, 0xe8, 0, 2, 1, I2C_CLASS_UNKNOWN};
}
tSpiParameters getPconSPIParams(HwInstance instance, uint32_t index)
{
    if (instance.id == HW_INSTANCE_CARD && instance.card.card_type == 0x00)
    {
        printf("HW_CARD_UNKNOWN passed in, getting card type" "\n");
        instance.card.card_type = GetMyCardType();
    }
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    if (pcon_info)
    {
        tSpiParameters params = {pcon_info->dev.fpga_id, 0 , (6),(2),0,1,0,0,0};
        params.channel = pcon_info->dev.spiChannel;
        params.timer = pcon_info->dev.spiTimer;
        return (params);
    }
    printf("invalid index for %s" "\n", HwInstanceToString(instance).c_str());
    return {CTL_FPGA_IOCTL, 0 , (6),(2),0,1,0,0,0};
}
void hwPconCntrlExtSpiMasterToProm(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, bool enable)
{
    SrlStatus status;
    uint16_t value;
    status = pconReadGlobalReg(ctrlr, pDev, 0x08, &value);
    if (status == 0)
    {
        value = (value & ~(0x2 | 0x1));
        if (enable)
        {
            value |= 0x1;
        }
        pconWriteGlobalReg(ctrlr, pDev, 0x08, &value);
    }
}
void hwPconCntrlExtSpiMasterToLogNvr(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, bool enable)
{
    SrlStatus status;
    uint16_t value;
    status = pconReadGlobalReg(ctrlr, pDev, 0x08, &value);
    if (status == 0)
    {
        value = (value & ~(0x2 | 0x1));
        if (enable)
        {
            value = value | 0x2 | 0x1;
        }
        pconWriteGlobalReg(ctrlr, pDev, 0x08, &value);
    }
}
SrlStatus hwPconReadEventLogMemory(HwInstance instance, uint8_t idx, uint32_t offset, uint8_t *buf, int length)
{
    SrlStatus status = 0;
    uint32_t wrdata;
    tSpiParameters parms;
    I2CCtrlr ctrlr;
    I2CFpgaCtrlrDeviceParams dev_params;
    const tPconAccessApi* pcon_access = hwPconGetAccessApis(instance);
    parms = getPconSPIParams(instance, idx);
    dev_params = getPconI2CParams(instance, idx, &ctrlr);
    if ((length <= 0) || (buf == NULL))
        return (-1);
    hwPconCntrlExtSpiMasterToLogNvr(&ctrlr, &dev_params, 1);
    wrdata = (0x03 << 24) | (offset & 0xffffff);
    status = pcon_access->hwSpiReadBlock(&parms, wrdata, buf, length);
    hwPconCntrlExtSpiMasterToLogNvr(&ctrlr, &dev_params, 0);
    return status;
}
SrlStatus hwPconWriteEventLogMemory(HwInstance instance, uint32_t idx, uint32_t offset, uint8_t *buf, int length)
{
    SrlStatus status = 0;
    uint8_t wrdata[132];
    tSpiParameters parms;
    I2CCtrlr ctrlr;
    I2CFpgaCtrlrDeviceParams dev_params;
    const tPconAccessApi* pcon_access = hwPconGetAccessApis(instance);
    parms = getPconSPIParams(instance, idx);
    dev_params = getPconI2CParams(instance, idx, &ctrlr);
    if ((length <= 0) || (length > 128) || (buf == NULL))
        return (-1);
    hwPconCntrlExtSpiMasterToLogNvr(&ctrlr, &dev_params, 1);
    int i;
    wrdata[0] = 0x02;
    for (i = 0; i < 3; i++)
    {
        wrdata[1 + i] = (offset >> (16 - 8 * i)) & 0xff;
    }
    for (i = 0; i < length; i++)
    {
        wrdata[4 + i] = buf[i];
    }
    pcon_access->hwSpiWrite8(&parms, 0x06);
    uint8_t nvr_sr = 0;
    for (i = 0; i < 100; i++)
    {
        status = pcon_access->hwSpiRead8(&parms, 0x05, &nvr_sr);
        if ((status == 0) && ((nvr_sr & 0x0F) == 0x02))
            break;
    }
    if (((nvr_sr & 0x04) == 0x04) || ((nvr_sr & 0x08) == 0x08))
    {
        uint8_t wrsr_data[4];
        wrsr_data[0] = 0x01;
        wrsr_data[1] = wrsr_data[2] = wrsr_data[3] = 0x02;
        pcon_access->hwSpiWriteBlock(&parms, (uint8_t *)&wrsr_data, 2);
        pcon_access->hwSpiWrite8(&parms, 0x06);
        for (i = 0; i < 100; i++)
        {
            status = pcon_access->hwSpiRead8(&parms, 0x05, &nvr_sr);
            if ((status == 0) && ((nvr_sr & 0x0F) == 0x02))
                break;
        }
        printf("hwPconWriteEventLogMemory: Re-enable Block writes in WSRS - nvr_sr 0x%x status %d\n" "\n", nvr_sr, status);
    }
    if (i < 100)
        status = pcon_access->hwSpiWriteBlock(&parms, (uint8_t *)&wrdata, length + 4);
    else
    {
        printf("hwPconWriteEventLogMemory: error - nvr_sr 0x%x status %d\n" "\n", nvr_sr, status);
        status = (-1);
    }
    pcon_access->hwSpiWrite8(&parms, 0x04);
    hwPconCntrlExtSpiMasterToLogNvr(&ctrlr, &dev_params, 0);
    return status;
}
uint8_t crc8CalulateGp07(uint8_t *message, int nBytes)
{
    uint8_t data;
    static const uint8_t crc8_table[256] = {0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65, 0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd, 0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2, 0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a, 0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42, 0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4, 0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c, 0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34, 0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b, 0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83, 0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3};
    uint8_t remainder = 0xff;
    int byte;
    for (byte = 0; byte < nBytes; ++byte)
    {
        data = message[byte] ^ remainder;
        remainder = crc8_table[data];
    }
    return (remainder);
}
uint16_t analyzeChannelStatus(tChannelStatus chanStatus)
{
    uint16_t status = 0;
    uint8_t event = (chanStatus.prm_file << 7) | (chanStatus.ev1_to << 6) | (chanStatus.ev0_to << 5) | (chanStatus.c_a2d << 4) |
                   (chanStatus.v_a2d << 3) | (chanStatus.oc << 2) | (chanStatus.ov << 1) | (chanStatus.uv << 0);
    if (crc8CalulateGp07(&event, 1) == chanStatus.crc8)
    {
        status = event;
    }
    else
    {
        status = (chanStatus.crc8 << 8) | event;
    }
    return status;
}
void fromSecondsCalulateOnTime(uint32_t seconds, tPoweredOnTime *pOntimeData)
{
    pOntimeData->days = seconds / (24 * 60 * 60);
    seconds = seconds % (24 * 60 * 60);
    pOntimeData->hours = seconds / (60 * 60);
    seconds = seconds % (60 * 60);
    pOntimeData->minutes = seconds / 60;
    pOntimeData->seconds = seconds % 60;
}
SrlStatus hwPconGetEventLogMemory(HwInstance instance, uint8_t idx, int event_num, tPconEventLogMemory * pEventRam)
{
    uint32_t offset;
    SrlStatus status;
    tPconEventHeader header = {0};
    int current_event, num_event_wrap;
    int req_event;
    if ((pEventRam == NULL) || (((event_num) < 0 ? -(event_num) : (event_num)) >= ((1 << 10) - 1)))
        return (-1);
    offset = 0;
    status = hwPconReadEventLogMemory(instance, idx, offset, (uint8_t *) &header, sizeof(tPconEventHeader));
    if (status != 0)
    {
        printf("PCON %u: could not read event log memory\n", idx);
        return (-1);
    }
    offset = header.eventPtr;
    uint32_t temp_eventPtr = (header.eventPtr);
    temp_eventPtr = be32toh(temp_eventPtr << 8);
    header.eventPtr = temp_eventPtr;
    if (crc8CalulateGp07((uint8_t *) &offset, 3) != header.hdrCrc)
    {
        printf("PCON %u: bad header(0x%08X) CRC-8 value in event log header\n", idx, (header.hdrCrc << 24 | header.eventPtr));
        return (-1);
    }
    current_event = header.eventPtr % (((1 << 10) - 1) + 1);
    num_event_wrap = header.eventPtr >> 10;
    if (num_event_wrap == 0)
        req_event = (((event_num) < 0 ? -(event_num) : (event_num)) >= current_event) ? current_event : current_event + event_num;
    else
        req_event = (((event_num) < 0 ? -(event_num) : (event_num)) >= current_event) ? (((1 << 10) - 1) + current_event + event_num) : (current_event + event_num);
    ;
    offset = req_event * (sizeof(tPconEventLogMemory));
    status = hwPconReadEventLogMemory(instance, idx, offset, (uint8_t *) pEventRam, sizeof(tPconEventLogMemory));
    return status;
}
SrlStatus hwPconGetEventLogNumPowerCycle(HwInstance instance, uint8_t idx, uint32_t *num_power_cycle)
{
    uint32_t offset;
    SrlStatus status;
    tPconEventHeader header = {0};
    offset = 0;
    status = hwPconReadEventLogMemory(instance, idx, offset, (uint8_t *) &header, sizeof(tPconEventHeader));
    offset = header.eventPtr;
    if ((status == 0) && (crc8CalulateGp07((uint8_t *) &offset, 3) == header.hdrCrc))
    {
       *num_power_cycle = be32toh(header.eventPtr) >> 8;
    }
    else
    {
        printf("Number of Pcon power cycles not obtained correctly.\n");
    }
    return status;
}
SrlStatus hwPconDumpEventLogMemory(HwInstance instance, uint8_t idx, int event_num, bool display_header, bool verbose)
{
    tPconEventLogMemory event_log __attribute__ ((aligned (8)));
    tPconEventLogSoftware* softwareInfo;
    SrlStatus status;
    uint32_t up_time;
    tPoweredOnTime power_up_time;
    const char *ev_name[] = {"uv", "ov", "oc", "v_a2d", "c_a2d", "ev0_to", "ev1_to", "prm_file"};
    tPconConfig pconConfig = getPconData(instance, idx);
    const tPconConfig * config = &pconConfig;
    const tPconChanConfig * chan_config = config->channels;
    const tPconDevice * pcon_info = hwPconGetPconInfo(instance, idx);
    status = hwPconGetEventLogMemory(instance, idx, event_num, &event_log);
    if (status == 0)
    {
        softwareInfo = &(hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->softwareReserved : ((tPconEventLogMemory * )&event_log)->softwareReserved);
        int i;
        printf("\n");
        if (display_header)
            printf("Pcon Device Index %u (name %s, des %s)\n", idx, pcon_info->dev.name, pcon_info->dev.desc);
        up_time = ( (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->upTimeInSeconds : ((tPconEventLogMemory * )&event_log)->upTimeInSeconds) ==
                   ~(hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->invUpTimeInSeconds : ((tPconEventLogMemory * )&event_log)->invUpTimeInSeconds)) ?
                    (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->upTimeInSeconds : ((tPconEventLogMemory * )&event_log)->upTimeInSeconds) : 0;
        up_time = be32toh(up_time);
        fromSecondsCalulateOnTime(up_time, &power_up_time);
        printf("Pcon Event number %d: Powered on: %u days %02u:%02u:%02u, IMBV voltage was %u millivolt when card powered down.\n",
                event_num, power_up_time.days, power_up_time.hours, power_up_time.minutes, power_up_time.seconds,
                (uint32_t) ((12.49 / 2.49) * ((((be16toh((hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->rawImbvVoltValue : ((tPconEventLogMemory * )&event_log)->rawImbvVoltValue)) & ((1 << 10) - 1)) * (3000)) / (1 << 10)) + (((((be16toh((hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->rawImbvVoltValue : ((tPconEventLogMemory * )&event_log)->rawImbvVoltValue)) & ((1 << 10) - 1)) * (3000)) % (1 << 10)) + ((1 << 10) >> 1)) / (1 << 10)))));
        printf("Power Cycle Num: %d, Reset Cycle Num: %d, Reset Reason: %x, epoch time: %lu, crc: %x\n",
                softwareInfo->powerCycleNum, softwareInfo->resetCycleNum, softwareInfo->resetReason,
                softwareInfo->epochTime, softwareInfo->crc8);
        for (i = 0; i < config->channelCount; i++)
        {
            if (config->channels[i].name)
            {
                uint16_t chan_status = analyzeChannelStatus((hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->channelStatus[i] : ((tPconEventLogMemory * )&event_log)->channelStatus[i]));
                if (!(chan_status >> 8))
                {
                    int j;
                    for (j = 0; j < 8; j ++)
                        if (chan_status & (1 << j))
                            printf("channel %d: event %s occurred\n", i, ev_name[j]);
                }
                else
                {
                    printf("channel %d: bad CRC-8 value 0x%02X \n", i, (chan_status >> 8));
                }
            }
        }
        printf("\n");
        printf("Channel  Name                          prm_file    ev1_to   ev0_to   c_a2d    v_a2d    oc       ov       uv    \n");
        printf("================================================================================================================\n");
        for (i = 0; i < config->channelCount; i++)
        {
            if (config->channels[i].name)
            {
                const char *chan_name = chan_config[i].name;
                tChannelStatus chan_status = (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->channelStatus[i] : ((tPconEventLogMemory * )&event_log)->channelStatus[i]);
                if (verbose || analyzeChannelStatus(chan_status))
                {
                    printf("%02u%6s %-30s   %-9u%-9u%-9u%-9u%-9u%-9u%-9u%-9u \n",
                            i, "", chan_name, chan_status.prm_file, chan_status.ev1_to, chan_status.ev0_to, chan_status.c_a2d,
                            chan_status.v_a2d, chan_status.oc, chan_status.ov, chan_status.uv);
                }
            }
        }
        printf("\n");
    }
    return status;
}
bool hwPconIdxIsValid(HwInstance instance, uint8_t idx)
{
    if (instance.id == HW_INSTANCE_CARD && instance.card.card_type == 0x00)
    {
        instance.card.card_type = GetMyCardType();
    }
    return (hwPconGetPconInfo(instance, idx, 0) != NULL);
}
void hwPconDumpEvents(HwInstance instance, uint8_t idx, int num_past_events, bool verbose)
{
    if (hwPconIdxIsValid(instance, idx))
    {
        int event_num = -1;
        bool display_header = 1;
        if ((num_past_events < 1) || (num_past_events > 1023)) num_past_events = 1;
        do
        {
            if (hwPconDumpEventLogMemory(instance, idx, event_num, display_header, verbose) != 0)
                break;
            display_header = 0;
            event_num--;
            num_past_events--;
        } while (num_past_events > 0);
    }
    else
    {
        printf("PCON Index Invalid\n");
    }
}
SrlStatus hwPconGetEventLogCurrentOffset(HwInstance instance, uint8_t idx, uint32_t *current_offset)
{
    SrlStatus status;
    uint32_t offset;
    tPconEventHeader header = {};
    uint32_t current_event;
    offset = 0;
    status = hwPconReadEventLogMemory(instance, idx, offset, (uint8_t *) &header, sizeof(tPconEventHeader));
    offset = header.eventPtr;
    uint32_t temp_eventPtr = header.eventPtr;
    temp_eventPtr = be32toh(temp_eventPtr <<8);
    header.eventPtr = temp_eventPtr;
    if ((status == 0) && (crc8CalulateGp07((uint8_t *) &offset, 3) == header.hdrCrc))
    {
        current_event = header.eventPtr % (((1 << 10) - 1) + 1);
        *current_offset = current_event * (sizeof(tPconEventLogMemory));
        ;
    }
    else
    {
        printf("Pcon %u on %s: error %d encountered while determining current event offset.\n" "\n", idx, HwInstanceToString(instance).c_str(), status);
        status = (-1);
    }
    return status;
}
tPconBoardResetType hwPconDeterminePowerCycleType(HwInstance instance, uint8_t idx, uint8_t res_event)
{
    tPconBoardResetType reset_type = res_event;
    tPconEventLogMemory event_log __attribute__ ((aligned (8)));
    SrlStatus status;
    uint32_t input_voltage = 0;
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, idx);
    if (pcon_info == nullptr)
    {
        printf("Could not find pcon %d info for %s\n" "\n", idx, HwInstanceToString(instance).c_str());
        return 0;
    }
    tPconConfig *config = &pcon_info->config;
    tPconEvent *event = hwPconGetPconEventInfo(instance, idx);
    status = hwPconGetEventLogMemory(instance, idx, -1, &event_log);
    input_voltage = (uint32_t) ((12.49 / 2.49) * ((((be16toh((hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->rawImbvVoltValue : ((tPconEventLogMemory * )&event_log)->rawImbvVoltValue)) & ((1 << 10) - 1)) * (3000)) / (1 << 10)) + (((((be16toh((hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->rawImbvVoltValue : ((tPconEventLogMemory * )&event_log)->rawImbvVoltValue)) & ((1 << 10) - 1)) * (3000)) % (1 << 10)) + ((1 << 10) >> 1)) / (1 << 10))));
    if (status == 0)
    {
        int i;
        if ((input_voltage < (7000)) || (input_voltage > (14500)))
            reset_type = (0x8000);
        else
            for (i = 0; i < config->channelCount; i++)
                if (config->channels[i].name)
                {
                    uint16_t chan_status = analyzeChannelStatus((hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->channelStatus[i] : ((tPconEventLogMemory * )&event_log)->channelStatus[i]));
                    uint8_t event_type = chan_status & 0xff;
                    if (chan_status >> 8)
                    {
                        reset_type = (0x8000) | 0x00ff;
                        reset_type |= (i << 8);
                        ;
                        break;
                    }
                    if (event_type > 0)
                    {
                        reset_type = (0x8000) | event_type;
                        reset_type |= (i << 8);
                        ;
                        break;
                    }
                }
        event->lastPowerOnDuration = be32toh(( (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->upTimeInSeconds : ((tPconEventLogMemory * )&event_log)->upTimeInSeconds) ==
                                              ~(hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->invUpTimeInSeconds : ((tPconEventLogMemory * )&event_log)->invUpTimeInSeconds)) ?
                                               (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->upTimeInSeconds : ((tPconEventLogMemory * )&event_log)->upTimeInSeconds) : 0);
        uint8_t calculated_crc = crc8CalulateGp07((uint8_t *) &(hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->softwareReserved : ((tPconEventLogMemory * )&event_log)->softwareReserved), (sizeof(tPconEventLogSoftware) - 1));
        if (calculated_crc == (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->softwareReserved : ((tPconEventLogMemory * )&event_log)->softwareReserved).crc8)
            event->lastPowerDownTime = (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->softwareReserved : ((tPconEventLogMemory * )&event_log)->softwareReserved).epochTime + event->lastPowerOnDuration;
    }
    return reset_type;
}
uint32_t hwPconElapsedSecsSincePowerOn(HwInstance instance, uint8_t idx)
{
    SrlStatus status;
    uint32_t power_up_time_sec = 0;
    if (hwPconIdxIsValid(instance, idx))
    {
        uint16_t sec_count;
        tPconDevice *pcon_info = hwPconGetPconInfo(instance, idx);
        if (pcon_info == nullptr)
        {
            printf("Pcon %u: could not find pcon info for %s.\n" "\n", idx, HwInstanceToString(instance).c_str());
            return power_up_time_sec;
        }
        I2CFpgaCtrlrDeviceParams *pcon_dev = &pcon_info->dev.dev_params;
        I2CCtrlr ctrlr = hwPconGetI2CCtrlr(instance, *pcon_info);
        status = pconReadGlobalReg(&ctrlr, pcon_dev, 0x0a, &sec_count);
        ;
        power_up_time_sec = sec_count;
        status |= pconReadGlobalReg(&ctrlr, pcon_dev, 0x0c, &sec_count);
        ;
        if (status == 0)
            power_up_time_sec |= sec_count << 16;
        else
            power_up_time_sec = 0;
    }
    return power_up_time_sec;
}
SrlStatus hwPconGetEventLogResetReason(HwInstance instance, uint8_t idx)
{
    SrlStatus status = 0;
    uint32_t offset;
    uint8_t res_event;
    uint32_t num_reset;
    uint32_t pow_cyc_num;
    tPconEventLogSoftware softwareInfo = {};
    time_t epoch_time_now = (GetUnixTime());
    if (!hwPconIdxIsValid(instance, idx))
    {
        printf("Invalid Pcon: %s\n" "\n", HwInstanceToString(instance).c_str());
        return (-1);
    }
    tPconEvent *pcon = hwPconGetPconEventInfo(instance, idx);
    if (pcon->numPowerCycles)
    {
        return 0;
    }
    status = hwPconGetEventLogCurrentOffset(instance, idx, &offset);
    status |= hwPconGetEventLogNumPowerCycle(instance, idx, &pow_cyc_num);
    if (status == 0)
    {
        pcon->numPowerCycles = pow_cyc_num;
        pcon->lastPowerUpTime = epoch_time_now - hwPconElapsedSecsSincePowerOn(instance, idx);
        offset = offset + (hwPconIsMini(instance, idx) ? offsetof(tPconMiniEventLogMemory, softwareReserved) : offsetof(tPconEventLogMemory, softwareReserved));
        status = hwPconReadEventLogMemory(instance, idx, offset,
                                          (uint8_t *) &softwareInfo, sizeof(softwareInfo));
        if (status == 0)
        {
            if ((crc8CalulateGp07((uint8_t *)&softwareInfo, (sizeof(softwareInfo) - 1)) != softwareInfo.crc8))
            {
                memset(&softwareInfo, '\0', sizeof(softwareInfo));
                softwareInfo.crc8 = crc8CalulateGp07((uint8_t *)&softwareInfo, (sizeof(softwareInfo) - 1));
            }
            res_event = softwareInfo.resetReason;
            num_reset = softwareInfo.resetCycleNum;
            if (pcon->numPowerCycles == softwareInfo.powerCycleNum)
            {
                pcon->lastResetReason = res_event;
                pcon->numResetSincePowerUp = num_reset;
                pcon->lastBootUpTime = epoch_time_now - (GetUnixUptime());
            }
            else
            {
                pcon->lastResetReason = hwPconDeterminePowerCycleType(instance, idx, res_event);
                pcon->numResetSincePowerUp = 0;
                pcon->lastBootUpTime = pcon->lastPowerUpTime;
            }
        }
        else
        {
            printf("Pcon %u: could not read event log software info for %s\n" "\n",
                       idx, HwInstanceToString(instance).c_str());
            status = (-1);
        }
    }
    return status;
}
void hwPconDumpResetReason(HwInstance instance, uint8_t idx)
{
    uint32_t power_up_time_sec;
    tPoweredOnTime power_up_time;
    if (hwPconIdxIsValid(instance, idx))
    {
        tPconEvent *pcon = hwPconGetPconEventInfo(instance, idx);
        power_up_time_sec = hwPconElapsedSecsSincePowerOn(instance, idx);
        fromSecondsCalulateOnTime(power_up_time_sec, &power_up_time);
        printf("Pcon %d on %s: Power cycles: %u  Powered on: %u days %02u:%02u:%02u  Reset cycles: %u  Reset reason: %s(0x%04X)" "\n",
                   idx, HwInstanceToString(instance).c_str(), pcon->numPowerCycles, power_up_time.days, power_up_time.hours, power_up_time.minutes, power_up_time.seconds,
                   pcon->numResetSincePowerUp, ((pcon->lastResetReason & (0x8000)) == (0x8000)) ? "power cycle" : "software/watchdog",
                   pcon->lastResetReason);
        printf("Pcon %d on %s: Last boot time: %s" "\n", idx, HwInstanceToString(instance).c_str(), ctime((time_t *) &pcon->lastBootUpTime));
        printf("Pcon %d on %s: Last power on time %s" "\n", idx, HwInstanceToString(instance).c_str(), ctime((time_t *) &pcon->lastPowerUpTime));
        printf("Pcon %d on %s: Last power off time %s" "\n", idx, HwInstanceToString(instance).c_str(), ctime((time_t *) &pcon->lastPowerDownTime));
    }
}
std::string hwPconGetResetReason(HwInstance instance, uint8_t idx)
{
    uint32_t power_up_time_sec;
    tPoweredOnTime power_up_time;
    std::string msg_output{};
    if (!hwPconIdxIsValid(instance, idx))
        return msg_output;
    if (hwPconGetEventLogResetReason(instance, idx) != 0)
        return msg_output;
    tPconEvent *pcon = hwPconGetPconEventInfo(instance, idx);
    power_up_time_sec = hwPconElapsedSecsSincePowerOn(instance, idx);
    fromSecondsCalulateOnTime(power_up_time_sec, &power_up_time);
    msg_output += Format("Pcon %d on %s: Power cycles: %u  Powered on: %u days %02u:%02u:%02u  Reset cycles: %u  Reset reason: %s(0x%04X)\n",
                         idx, HwInstanceToString(instance).c_str(), pcon->numPowerCycles, power_up_time.days, power_up_time.hours, power_up_time.minutes,
                         power_up_time.seconds, pcon->numResetSincePowerUp, ((pcon->lastResetReason & (0x8000)) == (0x8000)) ?
                         "power cycle" : "software/watchdog", pcon->lastResetReason);
    msg_output += Format("Pcon %d on %s: Last boot time: %s", idx, HwInstanceToString(instance).c_str(), ctime((time_t *) &pcon->lastBootUpTime));
    msg_output += Format("Pcon %d on %s: Last power on time: %s", idx, HwInstanceToString(instance).c_str(), ctime((time_t *) &pcon->lastPowerUpTime));
    msg_output += Format("Pcon %d on %s: Last power off time: %s\n", idx, HwInstanceToString(instance).c_str(), ctime((time_t *) &pcon->lastPowerDownTime));
    return msg_output;
}
std::string hwPconGetResetReasonAll(HwInstance instance)
{
    std::string msg_output{""};
    tPconDevice *pcon_info;
    if (instance.id == HW_INSTANCE_CARD && instance.card.card_type == 0x00)
    {
        instance.card.card_type = GetMyCardType();
    }
    int num_pcon = hwPconGetCardPconInfo(instance, &pcon_info);
    if (num_pcon > 0)
    {
        for (int i = 0; i < num_pcon; i++)
        {
            if (hwPconIdxIsValid(instance, i))
            {
                msg_output += hwPconGetResetReason(instance, i);
            }
        }
    }
    return msg_output;
}
SrlStatus hwPconGetClearEventLogResetReason(HwInstance instance, uint8_t idx)
{
    SrlStatus status = 0;
    uint32_t offset;
    uint8_t res_event;
    uint32_t pow_cyc_num;
    tPconEventLogMemory event_log __attribute__ ((aligned (8))) = {};
    tPconEventLogSoftware softwareInfo = {};
    if (!hwPconIdxIsValid(instance, idx))
    {
        printf("Pcon %d on %s: invalid pcon index.\n" "\n", idx, HwInstanceToString(instance).c_str());
        return (-1);
    }
    time_t epoch_time_now = (GetUnixTime());
    tPconEvent * pcon = hwPconGetPconEventInfo(instance, idx);
    memset(pcon, '\0', sizeof(tPconEvent));
    status = hwPconGetEventLogCurrentOffset(instance, idx, &offset);
    status |= hwPconGetEventLogNumPowerCycle(instance, idx, &pow_cyc_num);
    if (status == 0)
    {
        pcon->numPowerCycles = pow_cyc_num;
        offset = offset + (hwPconIsMini(instance, idx) ? offsetof(tPconMiniEventLogMemory, softwareReserved) : offsetof(tPconEventLogMemory, softwareReserved));
        status = hwPconReadEventLogMemory(instance, idx, offset, (uint8_t *) &softwareInfo, sizeof(softwareInfo));
        status |= hwPconGetEventLogMemory(instance, idx, -1, &event_log);
        pcon->lastPowerOnDuration = be32toh(( (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->upTimeInSeconds : ((tPconEventLogMemory * )&event_log)->upTimeInSeconds) ==
                                             ~(hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->invUpTimeInSeconds : ((tPconEventLogMemory * )&event_log)->invUpTimeInSeconds)) ?
                                              (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->upTimeInSeconds : ((tPconEventLogMemory * )&event_log)->upTimeInSeconds) : 0);
        uint8_t calculated_crc = crc8CalulateGp07((uint8_t *) &((hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->softwareReserved : ((tPconEventLogMemory * )&event_log)->softwareReserved)), (sizeof(tPconEventLogSoftware) - 1));
        if (calculated_crc == (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->softwareReserved : ((tPconEventLogMemory * )&event_log)->softwareReserved).crc8)
            pcon->lastPowerDownTime = (hwPconIsMini(instance, idx) ? ((tPconMiniEventLogMemory * )&event_log)->softwareReserved : ((tPconEventLogMemory * )&event_log)->softwareReserved).epochTime + pcon->lastPowerOnDuration;
    }
    if (status == 0)
    {
        uint8_t cal_crc8 = crc8CalulateGp07((uint8_t *) &softwareInfo, (sizeof(softwareInfo) - 1));
        if (cal_crc8 != softwareInfo.crc8)
        {
            ;
            memset(&softwareInfo, '\0', sizeof(softwareInfo));
            softwareInfo.crc8 = crc8CalulateGp07((uint8_t *) &softwareInfo, (sizeof(softwareInfo) - 1));
        }
        res_event = softwareInfo.resetReason;
        if (pow_cyc_num == softwareInfo.powerCycleNum)
            pcon->lastResetReason = res_event;
        else
            pcon->lastResetReason = hwPconDeterminePowerCycleType(instance, idx, res_event);
        ;
        ;
        ;
        ;
        ;
        if (!((pcon->lastResetReason & (0x8000)) == (0x8000)))
        {
            softwareInfo.resetCycleNum = softwareInfo.resetCycleNum + 1;
            softwareInfo.resetReason = res_event = 0;
            softwareInfo.crc8 = crc8CalulateGp07((uint8_t *) &softwareInfo, (sizeof(softwareInfo) - 1));
            pcon->lastBootUpTime = epoch_time_now - (GetUnixUptime());
            pcon->lastPowerUpTime = (softwareInfo.epochTime > 0) ? softwareInfo.epochTime :
                                     (epoch_time_now - hwPconElapsedSecsSincePowerOn(instance, idx));
            pcon->numResetSincePowerUp = softwareInfo.resetCycleNum;
        }
        else
        {
            memset(&softwareInfo, '\0', sizeof(softwareInfo));
            softwareInfo.powerCycleNum = pow_cyc_num;
            softwareInfo.epochTime = epoch_time_now - hwPconElapsedSecsSincePowerOn(instance, idx);
            softwareInfo.crc8 = crc8CalulateGp07((uint8_t *) &softwareInfo, (sizeof(softwareInfo) - 1));
            pcon->lastBootUpTime = epoch_time_now;
            pcon->lastPowerUpTime = softwareInfo.epochTime;
        }
        ;
        status = hwPconWriteEventLogMemory(instance, idx, offset, (uint8_t *) &softwareInfo, sizeof(softwareInfo));
        hwPconDumpResetReason(instance, idx);
    }
    else
    {
        printf("Pcon %d on %s: could not read event log memory for %s.\n" "\n", idx, HwInstanceToString(instance).c_str(), HwInstanceToString(instance).c_str());
        status = (-1);
    }
    return status;
}
SrlStatus hwPconGetClearEventLogResetReasonAll(HwInstance instance)
{
    tPconDevice *pcon_info;
    int num_pcon = hwPconGetCardPconInfo(instance, &pcon_info);
    ;
    if (num_pcon > 0)
    {
        for (int i = 0; i < num_pcon; i++)
        {
            if (hwPconIdxIsValid(instance, i))
            {
                ;
                hwPconGetClearEventLogResetReason(instance, i);
            }
        }
    }
    return 0;
}
