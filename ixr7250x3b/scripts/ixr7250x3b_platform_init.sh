#!/bin/bash

# platform init script for Nokia IXR7250 X3b

# Load required kernel-mode drivers
load_kernel_drivers() {
    echo "Loading Kernel Drivers"
    depmod -a
    rmmod amd-xgbe
    rmmod igb
    rmmod i2c-piix4
    rmmod i2c_designware_platform
    modprobe igb
    modprobe amd-xgbe
    modprobe i2c_designware_platform
    modprobe i2c-piix4
    modprobe i2c-smbus
    modprobe i2c-dev
    modprobe i2c-mux
    modprobe cpuctl
    modprobe rtc_ds1307
    modprobe optoe
    modprobe at24
    modprobe max31790
    modprobe jc42
    modprobe psu_x3b
    modprobe fan_eeprom
    modprobe psu_eeprom
    modprobe fan_led
}

file_exists() {
    # Wait 10 seconds max till file exists
    for((i=0; i<10; i++));
    do
        if [ -f $1 ]; then
            return 1
        fi
        sleep 1
    done
    return 0
 }

# Install kernel drivers required for i2c bus access
load_kernel_drivers

echo m41t11 0x68 > /sys/bus/i2c/devices/i2c-7/new_device

# Enumerate system eeprom
file_exists /sys/bus/i2c/devices/i2c-1/new_device
echo 24c64 0x54 > /sys/bus/i2c/devices/i2c-1/new_device

hwclock -s -f /dev/rtc1

# take asics out of reset
/etc/init.d/opennsl-modules stop
echo 1 > /sys/bus/pci/drivers/cpuctl/0000:05:00.0/jer_reset_seq
sleep 1
/etc/init.d/opennsl-modules start

echo jc42 0x18 > /sys/bus/i2c/devices/i2c-0/new_device
echo jc42 0x19 > /sys/bus/i2c/devices/i2c-0/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-1/new_device
echo tmp421 0x1e > /sys/bus/i2c/devices/i2c-7/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-19/new_device
echo tmp75 0x4a > /sys/bus/i2c/devices/i2c-19/new_device
echo tmp75 0x4b > /sys/bus/i2c/devices/i2c-19/new_device

#fan
i2cset -y 11 0x20 0x0 0x26
i2cset -y 12 0x20 0x0 0x26
i2cset -y 13 0x20 0x0 0x26
echo max31790 0x20 > /sys/bus/i2c/devices/i2c-11/new_device
echo max31790 0x20 > /sys/bus/i2c/devices/i2c-12/new_device
echo max31790 0x20 > /sys/bus/i2c/devices/i2c-13/new_device
echo fan_eeprom 0x54 > /sys/bus/i2c/devices/i2c-11/new_device
echo fan_eeprom 0x54 > /sys/bus/i2c/devices/i2c-12/new_device
echo fan_eeprom 0x54 > /sys/bus/i2c/devices/i2c-13/new_device
echo fan_led 0x60 > /sys/bus/i2c/devices/i2c-11/new_device
echo fan_led 0x60 > /sys/bus/i2c/devices/i2c-12/new_device
echo fan_led 0x60 > /sys/bus/i2c/devices/i2c-13/new_device

# PSU
echo psu_x3b 0x5b > /sys/bus/i2c/devices/i2c-14/new_device
echo psu_x3b 0x5b > /sys/bus/i2c/devices/i2c-15/new_device
echo psu_eeprom 0x53 > /sys/bus/i2c/devices/i2c-14/new_device
echo psu_eeprom 0x53 > /sys/bus/i2c/devices/i2c-15/new_device

for num in {27..62}; do
    echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${num}/new_device
    sleep 0.1
done

for num in {27..62}; do
    echo 300 > /sys/bus/i2c/devices/${num}-0050/write_timeout
    sleep 0.1
done

exit 0
