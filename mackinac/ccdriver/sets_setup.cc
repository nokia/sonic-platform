/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include "replacements.h"
#include "platform_hw_info.h"
#include "idt8a3xxxx.h"
#include "idtfw_4_9_7.h"
#include "hw_pll_tables_sets.h"
#include <vector>
#include <string_view>
namespace srlinux::platform::bsp {
 tIdt8a3xxxxDeviceConfigInfo vermilionDeviceConfigInfo =
{
    .addrInfo = {},
    .deviceVariant = idt8a35003DeviceVariant,
    .deviceName = "Vermilion SETS",
    .configFile = vermilion_sets_idt_PLL_cfg,
    .firmware = &firmware_4_9_7,
    .emptyPromOnly = 0,
    .eraseProm = 1,
    .eepromBlockI2cAddr = { 0x51, 0x55 },
    .broadscynOtdc =
    {
        .valid = 1,
        .otdcIdx = idt8a3xxxxOutputTdc1
    },
    .inputConfig =
    {
        .perInputInfo =
        {
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "localT0",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "localLoopBack",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "LoopInImmPM",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "faceplatePps",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "ref1",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "ref2",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "ptp",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
              { .inputType = idt8a3xxxxUnusedInput, .t0HwRef = 0, .inputName = "n/c",},
        },
    },
    .dpllConfig =
    { .t0Dpll = idt8a3xxxxDpll2,
        .t4Dpll = idt8a3xxxxInvalidDpll,
        .esDpll = idt8a3xxxxDpll5,
        .gnssDpll = idt8a3xxxxInvalidDpll,
        .localZDpll = idt8a3xxxxDpll0,
        .ts1ZDpll = idt8a3xxxxDpll1,
        .perDpllInfo =
        {
              { .dpllName = "localZdpll",},
              { .dpllName = "ts1Zdpll",},
              { .dpllName = "t0",},
              { .dpllName = "fpTdc",},
              { .dpllName = "fpPps",},
              { .dpllName = "esPll",},
              { .dpllName = "broadsync",},
              { .dpllName = "n/c",},
        },
    },
    .outputConfig =
    {
        .bitsOutput = idt8a3xxxxInvalidOutput,
        .bitsDivForT1 = 1,
        .bitsDivForE1 = 1,
        .bitsDivForSq = 1,
        .perOutputInfo =
        {
              { .outputName = "localLpbk",},
              { .outputName = "phyPpsBot",},
              { .outputName = "zdpllFanout",},
              { .outputName = "ppsFpga",},
              { .outputName = "localT0",},
              { .outputName = "n/c",},
              { .outputName = "tsClk",},
              { .outputName = "tsSync",},
              { .outputName = "fpPps",},
              { .outputName = "espllOut",},
              { .outputName = "fpgaTsClk",},
              { .outputName = "fpgatsSync",},
        },
    },
};
static tIdt8a3xxxxDevIndex idt8a35003DevIdx = 10;
static tIdt8a3xxxxDeviceConfigInfo *idt8a35003CurrentDeviceConfigInfo = NULL;
eIoctrlNum idt8a35003IoctrlNumGet(void)
{
    eIoctrlNum ioctrlNum = IOCTRL_NUM_BASE;
    if ((0))
        ioctrlNum = IOCTRL_NUM_FPI_1;
    return (ioctrlNum);
}
tIdt8a3xxxxDevIndex idt8a35003InitApisHw(void)
{
    tSpiParameters parms = {CTL_FPGA_DEFAULT, 0 , 0,0,0,1,0,0,0};
    if ((idt8a35003DevIdx < 10))
        return idt8a35003DevIdx;
    switch (GetMyCardType())
    {
        case 0x1b: case 0x20: case 0x23: case 0x3c:
            idt8a35003CurrentDeviceConfigInfo = &vermilionDeviceConfigInfo;
            parms.fpga_id = CTL_FPGA_CPUCTL;
            parms.channel = (1);
            parms.timer = 16;
            parms.ioctrlNum = idt8a35003IoctrlNumGet();
            idt8a35003CurrentDeviceConfigInfo->addrInfo.addrType = idt8a3xxxxSpiAddressType;
            idt8a35003CurrentDeviceConfigInfo->addrInfo.spiParms = parms;
            break;
    }
    idt8a35003DevIdx = idt8a3xxxxInitDevInfo(((tIdt8a3xxxxDevIndex) -1), idt8a35003CurrentDeviceConfigInfo);
    return idt8a35003DevIdx;
}
}
namespace sets_setup_options {
bool download;
bool info;
std::string filename;
std::string_view get_option(
    const std::vector<std::string_view>& args,
    const std::string_view& option_name) {
    for (auto it = args.begin(), end = args.end(); it != end; ++it) {
        if (*it == option_name)
            if (it + 1 != end)
                return *(it + 1);
    }
    return "";
}
bool has_switch(
    const std::vector<std::string_view>& args,
    const std::string_view& option_name) {
    for (auto it = args.begin(), end = args.end(); it != end; ++it) {
        if (*it == option_name)
            return 1;
    }
    return 0;
}
void parse(int argc, char* argv[]) {
    if (argc > 32) {
        throw std::runtime_error("too many input parameters!");
    }
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    download = has_switch(args, "-d") || has_switch(args, "--download");
    info = has_switch(args, "-i");
    filename = get_option(args, "-f");
    if (download + (filename != "") + info > 1 ) {
        throw std::runtime_error("both more than one action requested");
    }
    if (!info && !download && filename == "") {
        throw std::runtime_error("not doing anything; neither filename nor download specified");
    }
    return;
}
void usage(char *command) {
    printf("%s: ( -i ) ([ -d | --download ] | [-f <firmware_file>])\n", command);
    return;
}
}
int main(int argc, char *argv[])
{
    CardType my_id = GetMyCardType();
    if (my_id == 0) {
        printf("Environment initialization appears to be failing; quitting\n");
        return EXIT_FAILURE;
    }
    try {
        sets_setup_options::parse(argc, argv);
    } catch (const std::exception &x) {
        printf("%s: %s\n", argv[0], x.what());
        sets_setup_options::usage(argv[0]);
        return EXIT_FAILURE;
    }
    printf("My CardType %i\n", my_id);
    {
        srlinux::platform::bsp::tIdt8a3xxxxDevIndex dev0 =
            srlinux::platform::bsp::idt8a35003InitApisHw();
        if (sets_setup_options::info) {
            srlinux::platform::bsp::idt8a3xxxxDumpDevs();
        }
        else if (sets_setup_options::download) {
            srlinux::platform::bsp::idt8a3xxxxBringupByDownload(dev0);
        }
        else {
            srlinux::platform::bsp::idt8a3xxxxProgramEepromFromFile(dev0,
                const_cast<char *>(sets_setup_options::filename.c_str()), 0);
        }
    }
    return EXIT_SUCCESS;
}
