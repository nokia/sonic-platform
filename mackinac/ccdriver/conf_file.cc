/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include "replacements.h"
#include "platform_hw_info.h"
#include "tmSpiDefs.h"
#include <string>
#include <fmt/format.h>
#include <unordered_map>
#include <map>
#include <utility>
#include <fstream>
#include <iostream>
namespace configuration_file {
const std::string WHITESPACECHARS = " \n\r\t\f\v";
std::string ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACECHARS);
    return (start == std::string::npos) ? "" : s.substr(start);
}
std::string rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACECHARS);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
std::string trim(const std::string &s) {
    return rtrim(ltrim(s));
}
class Configuration
{
public:
    static Configuration& Get() {
        static Configuration instance("/var/run/sonic-platform-nokia/devices.conf");
        return instance;
    }
    ~Configuration() {
        for (auto &it : open_spi_fds )
            spi_close(it.second);
    }
    CardType cardType(void) {
        return myCardType;
    }
    std::string getPconDeviceBase(int index) {
        return pcon_map[index];
    }
    std::string getCPCtlDeviceBase(void) {
        return cpuctl_dev_path;
    }
    std::string getIOCtlDeviceBase(void) {
        return ioctl_dev_path;
    }
    std::string getSpiDevice(CtlFpgaId fpga_id, uint16_t channel) {
        return fmt::format("/dev/spidev{}.{}", (int)fpga_id, channel);
    }
    int getSpiFd(CtlFpgaId fpga_id, uint16_t channel) {
        std::string dev_path = getSpiDevice(fpga_id, channel);
        auto it = open_spi_fds.find(dev_path);
        if (it != open_spi_fds.end()) {
            return it->second;
        }
        else {
            int fd = spi_open(dev_path.c_str());
            if (fd != -1) {
                open_spi_fds[dev_path] = fd;
            }
            return fd;
        }
    }
private:
    CardType myCardType = 0x1b;
    std::unordered_map<std::string, std::string> config_map;
    std::unordered_map <int, std::string> pcon_map;
    std::unordered_map <std::string, int> open_spi_fds;
    std::string cpuctl_dev_path;
    std::string ioctl_dev_path;
    Configuration(std::string config_file) {
        std::ifstream file(config_file);
        std::string line;
        while (getline(file, line)) {
            std::string uncommented_line = line.substr(0, line.find("#"));
            std::string delimiter = "=";
            auto pos = uncommented_line.find(delimiter);
            if (pos == std::string::npos) {
                continue;
            }
            std::string raw_key = uncommented_line.substr(0, pos);
            uncommented_line.erase(0, pos + delimiter.length());
            std::string key = trim(raw_key);
            std::string val = trim(uncommented_line);
            config_map[key] = val;
        }
        std::string card_string = config_map["board"];
        if (card_string.compare("x3b") == 0) {
            myCardType = 0x1b;
        }
        else if (card_string.compare("x4") == 0) {
            myCardType = 0x3c;
        }
        else if (card_string.compare("x1b") == 0) {
            myCardType = 0x20;
        }
        else {
            myCardType = 0x00;
        }
        try {
            cpuctl_dev_path = config_map.at("cpctl");
        }
        catch (std::out_of_range) {
            ioctl_dev_path = "/sys/bus/pci/drivers/cpuctl/0000:01:00.0/";
        }
        try {
            ioctl_dev_path = config_map.at("ioctl");
        }
        catch (std::out_of_range) {
            cpuctl_dev_path = "/sys/bus/pci/drivers/cpuctl/0000:05:00.0/";
        }
        int pcon_index = 0;
        std::string path;
        while((path = config_map[std::string("pcon"+std::to_string(pcon_index))]) != "") {
            pcon_map[pcon_index] = path;
            pcon_index++;
        }
    }
};
}
CardType GetMyCardType(void)
{
    return configuration_file::Configuration::Get().cardType();
}
std::string GetPconDeviceBase(uint32_t index)
{
    return configuration_file::Configuration::Get().getPconDeviceBase(index);
}
std::string GetCPCtlDeviceBase(void)
{
    return configuration_file::Configuration::Get().getCPCtlDeviceBase();
}
std::string GetIOCtlDeviceBase(void)
{
    return configuration_file::Configuration::Get().getIOCtlDeviceBase();
}
std::string GetSpiDevice(const tSpiParameters *spi_paramters)
{
    CtlFpgaId fpga_id = (spi_paramters->fpga_id == CTL_FPGA_DEFAULT) ? ctl_fpga_id_default() : spi_paramters->fpga_id;
    return configuration_file::Configuration::Get().getSpiDevice(fpga_id, spi_paramters->channel);
}
int GetSpiFd(const tSpiParameters *spi_paramters)
{
    CtlFpgaId fpga_id = (spi_paramters->fpga_id == CTL_FPGA_DEFAULT) ? ctl_fpga_id_default() : spi_paramters->fpga_id;
    return configuration_file::Configuration::Get().getSpiFd(fpga_id, spi_paramters->channel);
}
