/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

#include <unistd.h>
#include <stdint.h>
#include <string>
#include "hw_instance.h"
#include "idt8a3xxxx.h"
time_t GetUnixTime(void);
time_t GetUnixUptime(void);
static inline int SleepMilliSeconds(int wait_period) {
    return usleep(1000*wait_period);
}
namespace srlinux {
static inline bool IsUnitTestBinary() {
    return 0;
}
namespace platform {
namespace bsp {
tIdt8a3xxxxDevIndex idt8a35003InitApisHw(void);
}
}
}
void SpiDeviceRead(const tSpiParameters *spi_paramters, uint32_t register_offset, uint32_t registers, uint8_t* data);
void SpiDeviceWrite(const tSpiParameters *spi_paramters, uint32_t register_offset, uint32_t registers, const uint8_t* data);
std::string Format(const char *format, ...);
HwInstance GetMyHwInstance();
uint32_t GetNumAsicsIf(void);
uint32_t GetPconIndexForAsicIf(int asicif_num);
uint32_t GetPconIndexForCPU();
uint32_t GetTargetMvolt(uint32_t jer_rov_value);
uint32_t GetCtrlFpgaMiscIO2(void);
std::string GetPconDeviceBase(uint32_t pcon_index);
std::string GetCPCtlDeviceBase(void);
std::string GetIOCtlDeviceBase(void);
std::string GetSpiDevice(const tSpiParameters *spi_paramters);
int GetSpiFd(const tSpiParameters *spi_paramters);
int spi_open(const char *device);
int spi_close(int fd);
int spi_xfer(int fd, const uint8_t *tx_buffer, uint32_t tx_len, uint8_t *rx_buffer, uint32_t rx_len);
int spi_read(int fd, uint8_t *rx_buffer, uint32_t rx_len);
int spi_write(int fd, const uint8_t *tx_buffer, uint32_t tx_len, bool end=1);
int spi_write_two(int fd, const uint8_t *tx_buffer1, uint32_t tx_len1, const uint8_t *tx_buffer2, uint32_t tx_len2);
