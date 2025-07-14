/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include "replacements.h"
#include "hwPcon.h"
#include <stdint.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
namespace pcon_options {
bool is_get_all_cmd;
bool is_dump_events_cmd;
bool is_reboot_analysis;
bool is_verbose;
int dump_pcon_index;
int dump_event_count;
std::string reboot_output_file;
std::string_view get_option_value(
    const std::vector<std::string_view>& args,
    const std::string_view& option_name,
    const int nth=1) {
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
    is_dump_events_cmd = has_switch(args, "-d");
    is_get_all_cmd = has_switch(args, "-g");
    is_reboot_analysis = has_switch(args, "-r") || has_switch(args, "--reboot-analysis");
    is_verbose = has_switch(args, "-v");
    if (!is_dump_events_cmd && !is_get_all_cmd && !is_reboot_analysis) {
        throw std::runtime_error("not doing anything;");
    }
    if (has_switch(args, "-r") && has_switch(args, "--reboot-analysis")) {
        throw std::runtime_error("multiple reboot analysis switch will be confusing");
    }
    if (is_dump_events_cmd) {
        std::string str;
        char *parsed_token;
        str = get_option_value(args, "-d", 1);
        dump_pcon_index = strtol(str.c_str(), &parsed_token, 10);
        if (parsed_token == str.c_str() || *parsed_token != '\0' || errno == ERANGE) {
            throw std::runtime_error("could not parse pcon index number; exiting...");
        }
        str = get_option_value(args, "-d", 2);
        dump_event_count = strtol(str.c_str(), &parsed_token, 10);
        if (parsed_token == str.c_str() || *parsed_token != '\0' || errno == ERANGE) {
            throw std::runtime_error("could not parse event count number; exiting...");
        }
    }
    if (is_reboot_analysis) {
        if (has_switch(args, "-r")) {
            reboot_output_file = get_option_value(args, "-r", 1);
        }
        else {
            reboot_output_file = get_option_value(args, "--reboot-analysis", 1);
        }
        if (reboot_output_file.empty() || reboot_output_file.starts_with("-")) {
            throw std::runtime_error("missing or invalid filename argument for reboot analysis output file; exiting...");
        }
    }
    return;
}
void usage(char *command) {
    printf("%s: ([ -d <pcon index> <event count> ] | [ -g ] [ (-r | --reboot-analysis) <output file>] )\n", command);
}
}
int main(int argc, char *argv[])
{
    CardType my_id = GetMyCardType();
    if (my_id == 0) {
        printf("My CardType %i\n", my_id);
        printf("Environment initialization appears to be failing; quitting\n");
        return EXIT_FAILURE;
    }
    try {
        pcon_options::parse(argc, argv);
    } catch (const std::exception &x) {
        printf("%s: %s\n", argv[0], x.what());
        pcon_options::usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (argc > 0) {
        if (pcon_options::is_dump_events_cmd) {
            HwInstance hw_instance_ = GetMyHwInstance();
            hwPconDumpEvents(hw_instance_,
                pcon_options::dump_pcon_index,
                pcon_options::dump_event_count,
                pcon_options::is_verbose);
        }
        if (pcon_options::is_get_all_cmd) {
            HwInstance hw_instance_ = GetMyHwInstance();
            hwPconShowDevices(hw_instance_);
            hwPconShowChannelsAll(hw_instance_);
            hwPconShowRailConfigAll(hw_instance_);
        }
        if (pcon_options::is_reboot_analysis) {
            HwInstance hw_instance_ = GetMyHwInstance();
            bool is_hw_power_failure = 0;
            std::string last_boot_time_str;
            std::string last_power_time_str;
            int pcon_index = GetPconIndexForCPU();
            std::string reset_log = hwPconGetResetReason(hw_instance_, pcon_index);
            if (pcon_options::is_verbose) {
                printf("Reset Log\n%s", reset_log.c_str());
            }
            std::stringstream reset_stream(reset_log);
            std::string reset_line;
            while (std::getline(reset_stream, reset_line, '\n')) {
                std::size_t offset;
                offset = reset_line.find("Reset reason: power cycle");
                if (offset != std::string::npos) {
                    is_hw_power_failure = 1;
                }
                std::string last_boot_key = "Last boot time: ";
                offset = reset_line.find(last_boot_key);
                if (offset != std::string::npos) {
                    last_boot_time_str = reset_line.substr(offset + last_boot_key.size());
                }
                std::string last_power_key = "Last power on time: ";
                offset = reset_line.find(last_power_key);
                if (offset != std::string::npos) {
                    last_power_time_str = reset_line.substr(offset + last_power_key.size());
                }
            }
            if (pcon_options::is_verbose) {
                printf("Power failure: %i\n", is_hw_power_failure);
                printf("Last boot time: %s\n", last_boot_time_str.c_str());
                printf("Last power time: %s\n", last_power_time_str.c_str());
            }
            if (pcon_options::is_verbose) {
                printf("Writing to status to: %s\n", pcon_options::reboot_output_file.c_str());
            }
            if (is_hw_power_failure) {
                std::ofstream(pcon_options::reboot_output_file, std::ios::out) << "Power Loss\n"
                    << last_power_time_str << "\n";
            }
            else {
                std::ofstream(pcon_options::reboot_output_file, std::ios::out) << "Unknown\n"
                    << last_boot_time_str << "\n";;
            }
            hwPconGetClearEventLogResetReason(hw_instance_, pcon_index);
        }
    }
    return EXIT_SUCCESS;
}
