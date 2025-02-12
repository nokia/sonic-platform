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
    modprobe optoe
    modprobe at24
    modprobe max31790
    modprobe psu_x3b
    modprobe fan_eeprom
    modprobe psu_eeprom
}

x3b_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7250_x3b-r0/Nokia-IXR7250-X3B/profile.ini
    echo "Nokia-IXR7250-X3B: Updated switch mac address ${MAC_ADDR}"
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
echo 24c64 0x54 > /sys/bus/i2c/devices/i2c-1/new_device

# take asics out of reset
/etc/init.d/opennsl-modules stop
echo 1 > /sys/bus/pci/drivers/cpuctl/0000:05:00.0/jer_reset_seq
sleep 1
/etc/init.d/opennsl-modules start

for num in {27..62}; do
    echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${num}/new_device
    sleep 0.1
done

echo tmp421 0x1e > /sys/bus/i2c/devices/i2c-7/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-19/new_device
echo tmp75 0x4a > /sys/bus/i2c/devices/i2c-19/new_device
echo tmp75 0x4b > /sys/bus/i2c/devices/i2c-19/new_device

#fan
echo max31790 0x20 > /sys/bus/i2c/devices/i2c-11/new_device
echo max31790 0x20 > /sys/bus/i2c/devices/i2c-12/new_device
echo max31790 0x20 > /sys/bus/i2c/devices/i2c-13/new_device
echo fan_eeprom 0x54 > /sys/bus/i2c/devices/i2c-11/new_device
echo fan_eeprom 0x54 > /sys/bus/i2c/devices/i2c-12/new_device
echo fan_eeprom 0x54 > /sys/bus/i2c/devices/i2c-13/new_device

# PSU
echo psu_x3b 0x5b > /sys/bus/i2c/devices/i2c-14/new_device
echo psu_x3b 0x5b > /sys/bus/i2c/devices/i2c-15/new_device
echo psu_eeprom 0x53 > /sys/bus/i2c/devices/i2c-14/new_device
echo psu_eeprom 0x53 > /sys/bus/i2c/devices/i2c-15/new_device

file_exists /sys/bus/i2c/devices/1-0054/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/1-0054/eeprom
    x3b_profile
else
    echo "SYSEEPROM file not foud"
fi

exit 0
