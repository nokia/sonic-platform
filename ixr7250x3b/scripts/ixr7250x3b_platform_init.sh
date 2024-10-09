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

# Enumerate system eeprom
file_exists /sys/bus/i2c/devices/i2c-1/new_device
echo 24c256 0x54 > /sys/bus/i2c/devices/i2c-1/new_device

# take asics out of reset
echo 1 > /sys/bus/pci/drivers/cpuctl/0000:05:00.0/jer_reset_seq

exit 0
