/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include "replacements.h"
#include "platform_hw_info.h"
#include "hwPcon.h"
#include <stdarg.h>
#include <unordered_map>
#include <ctime>
#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <cstring>
time_t GetUnixTime(void)
{
    return std::time(nullptr);
}
time_t GetUnixUptime(void)
{
    time_t dummy = 0;
    std::ifstream("/proc/uptime", std::ios::in) >> dummy;
    return dummy;
}
HwInstance GetMyHwInstance()
{
    HwInstance local;
    local.card.card_type = GetMyCardType();
    local.id = HW_INSTANCE_CARD;
    return local;
}
uint32_t GetNumAsicsIf(void)
{
    CardType self = GetMyCardType();
    switch(self) {
    case 0x1b:
        return 2;
    case 0x20:
        return 1;
    case 0x3c:
        return 2;
    }
    return 2;
}
uint32_t GetCtrlFpgaMiscIO2(void)
{
    uint32_t reg_value;
    std::ifstream((GetIOCtlDeviceBase()+"jer_avs").c_str(), std::ios::in) >> std::hex >> reg_value;
    return reg_value;
}
uint32_t GetPconIndexForAsicIf(int asic_num)
{
    CardType self = GetMyCardType();
    switch(self) {
    case 0x1b:
    case 0x20:
        if (asic_num == 0)
            return 0;
        else
            return 2;
        break;
    case 0x3c:
        return asic_num;
    }
    return 0;
}
uint32_t GetPconIndexForCPU()
{
    CardType self = GetMyCardType();
    std::map<CardType, int> board_reset_pcon_index = {
        {0x1b, 3},
        {0x20, 1},
        {0x3c, 1}};
    return board_reset_pcon_index.at(self);
}
uint32_t GetTargetMvolt(uint32_t jer_rov_value)
{
    CardType self = GetMyCardType();
    switch(self) {
    case 0x1b:
    case 0x20:
        return kJ2CPlusRovVolatge.at(jer_rov_value);
        break;
    case 0x3c:
    {
        auto iter = kJ3RamonRovVoltage.find(jer_rov_value);
        if (iter != kJ3RamonRovVoltage.end())
        {
            return iter->second;
        }
        else
        {
            printf("Read Invalid value 0x%02X for J3 AVS" "\n", jer_rov_value);
            return 0;
        }
    }
        break;
    }
    return 0;
}
CtlFpgaId ctl_fpga_id_default(void)
{
    return CTL_FPGA_CPUCTL;
}
const char *ctl_fpga_name(CtlFpgaId fpga_id)
{
    switch (fpga_id)
    {
        case CTL_FPGA_CPUCTL:
            return "CpuCtlFpga";
        case CTL_FPGA_IOCTL:
            return "IoCtlFpga";
        default:
            break;
    }
    return "NULL";
}
bool idt8a35003LocalZDpllLocked(void)
{
    return 1;
}
bool idt8a35003Ts1ZDpllLocked(void)
{
    return 1;
}
std::unordered_map<uint32_t, std::string> global_reg_map = {
    {0x00, "version_id_reg"},
    {0x02, "imb_volt_value_reg"},
    {0x04, "imb_error_reg"},
    {0x08, "spi_select_reg"},
    {0x06, "channel_select_reg"},
    {0x0a, "up_timer_lsw"},
    {0x0c, "up_timer_msw"}
};
std::unordered_map<uint32_t, std::string> channel_reg_map = {
    {0x10, "volt_set_inv_reg"},
    {0x12, "volt_set_reg"},
    {0x14, "under_volt_set_inv_reg"},
    {0x16, "under_volt_set_reg"},
    {0x18, "over_volt_set_inv_reg"},
    {0x1A, "over_volt_set_reg"},
    {0x1C, "measured_volt_reg"},
    {0x1E, "measured_current_reg"},
    {0x20, "current_multiplier_reg"},
    {0x22, "start_time_reg"},
    {0x24, "volt_ramp_reg"},
    {0x28, "max_current_reg"},
    {0x2A, "phase_offset_reg"},
    {0x2C, "volt_trim_allowance_reg"},
    {0x2E, "b0_coeff_reg"},
    {0x30, "b1_coeff_reg"},
    {0x32, "b2_coeff_reg"},
    {0x34, "a1_coeff_reg"},
    {0x36, "a2_coeff_reg"},
    {0x3A, "misc_reg"},
};
static int32_t __revFindPconIndex(HwInstance instance, I2CFpgaCtrlrDeviceParams *pDev)
{
    tPconDevice *pcon_info;
    if (int size = hwPconGetCardPconInfo(instance, &pcon_info))
    {
        for (int i = 0; i < size; i++)
        {
            if (pcon_info[i].dev.dev_params.channel == pDev->channel)
            {
                return pcon_info[i].dev.index;
            }
        }
    }
    return -1;
}
tI2cStatus pconReadGlobalReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t reg, uint16_t *value)
{
    HwInstance instance = GetMyHwInstance();
    int32_t index = __revFindPconIndex(instance, pDev);
    if (index == -1) {
        ;
        return (((1 << 2) + (1 << 1) + 1) + 4);
    }
    *value = hwPconReadGlobalReg(instance, index, reg);
    return 0;
}
tI2cStatus pconWriteGlobalReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint32_t reg, uint16_t *value)
{
    HwInstance instance = GetMyHwInstance();
    int32_t index = __revFindPconIndex(instance, pDev);
    if (index == -1) {
        ;
        return (((1 << 2) + (1 << 1) + 1) + 4);
    }
    hwPconWriteGlobalReg(instance, index, reg, *value);
    return 0;
}
SrlStatus pconReadChanReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint8_t channel, uint32_t reg, uint16_t *value)
{
    HwInstance instance = GetMyHwInstance();
    int32_t index = __revFindPconIndex(instance, pDev);
    if (index == -1) {
        return (((1 << 2) + (1 << 1) + 1) + 4);
    }
    *value = hwPconReadChannelReg(instance, index, channel, reg);
    return 0;
}
SrlStatus pconWriteChanReg(I2CCtrlr *ctrlr, I2CFpgaCtrlrDeviceParams *pDev, uint8_t channel, uint32_t reg, uint16_t *value)
{
    HwInstance instance = GetMyHwInstance();
    int32_t index = __revFindPconIndex(instance, pDev);
    if (index == -1) {
        return (((1 << 2) + (1 << 1) + 1) + 4);
    }
    hwPconWriteChannelReg(instance, index, channel, reg, *value);
    return 0;
}
uint16_t hwPconReadGlobalReg(HwInstance instance, uint32_t index, uint8_t reg)
{
    uint16_t value = 0;
    std::string pcon_device_base = GetPconDeviceBase(index);
    value = 0xffff;
    if (pcon_device_base != "")
    {
        std::string full_path = pcon_device_base + "/" + global_reg_map.at(reg);
        std::ifstream(full_path, std::ios::in) >> std::hex >> value;
        ;
    }
    return value;
}
void hwPconWriteGlobalReg(HwInstance instance, uint32_t index, uint8_t reg, uint16_t value)
{
    std::string pcon_device_base = GetPconDeviceBase(index);
    if (pcon_device_base != "")
    {
        std::string full_path = pcon_device_base + "/" + global_reg_map.at(reg);
        std::ofstream(full_path, std::ios::out) << value;
        ;
    }
    else
        printf("No PCON device at index %u\n", index);
}
uint16_t hwPconReadChannelReg(HwInstance instance, uint32_t index, uint8_t channel, uint8_t reg)
{
    uint16_t value = 0;
    SrlStatus status;
    std::string pcon_device_base = GetPconDeviceBase(index);
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    value = 0xffff;
    if (pcon_device_base != "")
    {
        if (channel >= pcon_info->config.channelCount)
        {
            printf("Invalid channel %u\n", channel);
            status = (-1);
        }
        else
        {
            status = 0;
            std::string full_path = pcon_device_base + "/channel" + std::to_string(channel) + "/" + channel_reg_map.at(reg);
            std::ifstream(full_path, std::ios::in) >> std::hex >> value;
        }
        if (status != 0)
            value = 0xffff;
    }
    else
        printf("No PCON device at index %u\n", index);
    return value;
}
void hwPconWriteChannelReg(HwInstance instance, uint32_t index, uint8_t channel, uint8_t reg, uint16_t value)
{
    SrlStatus status;
    std::string pcon_device_base = GetPconDeviceBase(index);
    tPconDevice *pcon_info = hwPconGetPconInfo(instance, index);
    if (pcon_device_base != "")
    {
        if (channel >= pcon_info->config.channelCount)
        {
            printf("Invalid channel %u\n", channel);
            status = (-1);
        }
        else
        {
            status = 0;
            std::string full_path = pcon_device_base + "/channel" + std::to_string(channel) + "/" + channel_reg_map.at(reg);
            std::ofstream(full_path, std::ios::out) << value;
        }
    }
    else
        printf("No PCON device at index %u\n", index);
    return;
}
namespace srlinux::platform::spi
{
inline void byte_shift_word(uint32_t data, uint8_t *byte_array, int bytes) {
    for (int i=0; i<bytes; i++) {
        byte_array[i] = (data >> (8 * (bytes - i - 1))) & 0xff;
    }
}
SrlStatus spiRead8(const tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata)
{
    SrlStatus status = 0;
    ;
    int fd = GetSpiFd(parms);
    uint8_t tx_buffer[4];
    byte_shift_word(wrdata, tx_buffer, 1);
    int rc;
    rc = spi_xfer(fd, tx_buffer, 1, rddata, 1);
    if (rc < 0 ) {
        printf("%s(): %s failed with %i (%s)\n",
            __FUNCTION__, "spi_xfer", rc, strerror(errno));
        return (-1);
    }
    ;
    return status;
}
SrlStatus spiReadBlock(const tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata, uint32_t rdcount)
{
    SrlStatus status = 0;
    ;
    int fd = GetSpiFd(parms);
    uint8_t tx_buffer[4];
    byte_shift_word(wrdata, tx_buffer, 4);
    int rc;
    rc = spi_xfer(fd, tx_buffer, 4, rddata, rdcount);
    if (rc < 0 ) {
        printf("%s(): %s failed with %i (%s)\n",
            __FUNCTION__, "spi_xfer", rc, strerror(errno));
        return (-1);
    }
    ;
    return status;
}
SrlStatus spiWrite16(const tSpiParameters *parms, uint32_t data)
{
    SrlStatus status = 0;
    ;
    int fd = GetSpiFd(parms);
    uint8_t tx_buffer[4];
    byte_shift_word(data, tx_buffer, 2);
    int rc = spi_write(fd, tx_buffer, 2);
    if (rc < 0 ) {
        printf("%s(): failed with %i (%s)\n", __FUNCTION__, rc, strerror(errno));
        return (-1);
    }
    else {
        ;
        return status;
    }
}
SrlStatus spiWrite8(const tSpiParameters *parms, uint32_t data)
{
    SrlStatus status = 0;
    ;
    int fd = GetSpiFd(parms);
    uint8_t tx_buffer[4];
    byte_shift_word(data, tx_buffer, 1);
    int rc;
    rc = spi_write(fd, tx_buffer, 1);
    if (rc < 0 ) {
        printf("%s(): %s failed with %i (%s)\n",
            __FUNCTION__, "spi_write", rc, strerror(errno));
        return (-1);
    }
    ;
    return status;
}
SrlStatus spiWriteNRead8(const tSpiParameters *parms, uint32_t wrdata, uint8_t wrbytes, uint8_t *rddata)
{
    SrlStatus status = 0;
    ;
    int fd = GetSpiFd(parms);
    uint8_t tx_buffer[4];
    byte_shift_word(wrdata, tx_buffer, wrbytes);
    int rc;
    uint8_t rx_buffer[4];
    rc = spi_xfer(fd, tx_buffer, wrbytes, rx_buffer, 1);
    if (rc < 0 ) {
        printf("%s(): %s failed with %i (%s)\n",
            __FUNCTION__, "spi_xfer", rc, strerror(errno));
        return (-1);
    }
    *rddata = rx_buffer[0];
    return status;
}
SrlStatus spiWrite8BlockRead(const tSpiParameters *parms, uint32_t wrdata, uint8_t *rddata, uint8_t rdbytes)
{
    SrlStatus status = 0;
    ;
    return status;
}
SrlStatus spiWriteBlock(const tSpiParameters *parms, const uint8_t *wrdata, uint32_t wrcount)
{
    SrlStatus status = 0;
    ;
    int fd = GetSpiFd(parms);
    const uint8_t *bldata = wrdata;
    uint32_t recount = wrcount;
    while (recount > 4096) {
        int rc = spi_write(fd, bldata, 4096, 0);
        if (rc < 0 ) {
            printf("%s(): failed with %i (%s), wrcount = %i\n",__FUNCTION__, rc, strerror(errno), 4096);
            return (-1);
        }
        else {
            ;
        }
        bldata += 4096;
        recount -= 4096;
    }
    int rc = spi_write(fd, bldata, recount);
    if (rc < 0 ) {
        printf("%s(): failed with %i (%s), wrcount = %i\n",__FUNCTION__, rc, strerror(errno), 4096);
        return (-1);
    }
    else {
        ;
        return status;
    }
}
}
std::string HwInstanceToString(HwInstance instance)
{
    std::string format;
    switch (instance.id)
    {
        case HW_INSTANCE_CARD:
            format = Format("Card with card_type=%i", instance.card.card_type);
            break;
        default:
            format = Format("Unknown instance id %i", instance.id);
    }
    return format;
}
static std::size_t CalculateLengthOfFormattedString(const char *format, va_list args)
{
    char buf[0];
    va_list local_args;
    va_copy(local_args, args);
    std::size_t length = vsnprintf(buf, 0, format, local_args);
    va_end(local_args);
    return length;
}
static std::string VFormat(const char *format, va_list args)
{
    int final_length = CalculateLengthOfFormattedString(format, args);
    std::string result;
    result.resize(final_length);
    vsnprintf(const_cast<char *>(result.c_str()), final_length + 1, format, args);
    return result;
}
std::string Format(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    std::string result = VFormat(format, args);
    va_end(args);
    return result;
}
int spi_open(const char *device)
{
    int fd;
    fd = open(device, O_RDWR);
    if (fd < 0) {
        return fd;
    }
    return fd;
}
int spi_close(int fd)
{
    return close(fd);
}
int spi_xfer(int fd, const uint8_t *tx_buffer, uint32_t tx_len, uint8_t *rx_buffer, uint32_t rx_len)
{
    int rc;
    struct spi_ioc_transfer ioc_message[2];
    memset(ioc_message, 0, sizeof(ioc_message));
    ioc_message[0].tx_buf = (unsigned long)tx_buffer;
    ioc_message[0].len = tx_len;
    ioc_message[1].rx_buf = (unsigned long)rx_buffer;
    ioc_message[1].len = rx_len;
    ioc_message[1].cs_change = 1;
    rc = ioctl(fd, SPI_IOC_MESSAGE(2), ioc_message);
    return rc;
}
int spi_read(int fd, uint8_t *rx_buffer, uint32_t rx_len)
{
    int rc;
    struct spi_ioc_transfer ioc_message[1];
    memset(ioc_message, 0, sizeof(ioc_message));
    ioc_message[0].rx_buf = (unsigned long)rx_buffer;
    ioc_message[0].len = rx_len;
    ioc_message[0].cs_change = 1;
    rc = ioctl(fd, SPI_IOC_MESSAGE(1), ioc_message);
    return rc;
}
int spi_write(int fd, const uint8_t *tx_buffer, uint32_t tx_len, bool end)
{
    int rc;
    struct spi_ioc_transfer ioc_message[1];
    memset(ioc_message, 0, sizeof(ioc_message));
    ioc_message[0].tx_buf = (unsigned long)tx_buffer;
    ioc_message[0].len = tx_len;
    ioc_message[0].cs_change = (end) ? 1 : 0;
    rc = ioctl(fd, SPI_IOC_MESSAGE(1), ioc_message);
    return rc;
}
int spi_write_two(int fd, const uint8_t *tx_buffer1, uint32_t tx_len1,
                const uint8_t *tx_buffer2, uint32_t tx_len2)
{
    int rc;
    struct spi_ioc_transfer ioc_message[2];
    memset(ioc_message, 0, sizeof(ioc_message));
    ioc_message[0].tx_buf = (unsigned long)tx_buffer1;
    ioc_message[0].len = tx_len1;
    ioc_message[1].tx_buf = (unsigned long)tx_buffer2;
    ioc_message[1].len = tx_len2;
    ioc_message[1].cs_change = 1;
    rc = ioctl(fd, SPI_IOC_MESSAGE(2), ioc_message);
    return rc;
}
