/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include "replacements.h"
#include "hwPcon.h"
#include <stdint.h>
namespace rov_config_options {
bool fake = 0;
bool verbose = 0;
std::string_view get_option_value(
    const std::vector<std::string_view>& args,
    const std::string_view& option_name,
    const int nth=1)
{
    for (auto it = args.begin(), end = args.end(); it != end; ++it) {
        if (*it == option_name)
            if (size_t(std::distance(it, end) > nth))
                return *(it + nth);
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
    fake = has_switch(args, "-n") || has_switch(args, "--simulate");
    verbose = has_switch(args, "-v") || has_switch(args, "--verbose");
    return;
}
void usage(char *command) {
    printf("%s: ([ -n | --simulate ] [-v | --verbose ])\n", command);
    return;
}
}
void SetRovConfig(bool simulate, bool verbose)
{
    HwInstance hw_instance_ = GetMyHwInstance();
    uint8_t num_asics_if = GetNumAsicsIf();
    uint8_t pcon_index = 0;
    uint32_t rov_mask = 0;
    uint32_t asic0_rov_mask = (0x000000FF);
    uint8_t shift = 0;
    uint8_t shift_factor = 0;
    for (int i = 0; i < num_asics_if; i++)
    {
        pcon_index = GetPconIndexForAsicIf(i);
        shift_factor = (i * (8));
        rov_mask = (asic0_rov_mask << shift_factor);
        shift = shift_factor;
        uint32_t jer_rov = (GetCtrlFpgaMiscIO2() & rov_mask) >> shift;
        uint32_t tgt_mvolt = GetTargetMvolt(jer_rov);
        if (tgt_mvolt == 0) continue;
        uint32_t uv_mvolt = tgt_mvolt * 0.92;
        uint32_t ov_mvolt = tgt_mvolt * 1.08;
        if (verbose)
        {
            printf("Bcm Asic %d Read %.2x ROV voltage %u mV undervoltage %u mV overvoltage %u mV Pcon index %d " "\n",
                i, jer_rov, tgt_mvolt, uv_mvolt, ov_mvolt, pcon_index);
        }
        if (!simulate) {
            hwPconSetOverVoltageSel(hw_instance_, pcon_index, 0, ov_mvolt);
            hwPconSetTargetVoltageSel(hw_instance_, pcon_index, 0, tgt_mvolt);
            hwPconSetUnderVoltageSel(hw_instance_, pcon_index, 0, uv_mvolt);
        }
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
        rov_config_options::parse(argc, argv);
    } catch (const std::exception &x) {
        printf("%s: %s\n", argv[0], x.what());
        rov_config_options::usage(argv[0]);
        return EXIT_FAILURE;
    }
    SetRovConfig(rov_config_options::fake, rov_config_options::verbose);
    return 0;
}
