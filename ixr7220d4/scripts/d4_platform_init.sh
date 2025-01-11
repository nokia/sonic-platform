#!/bin/bash

# platform init script for Nokia-IXR7220-D4-36D

# Load required kernel-mode drivers

load_kernel_drivers() {
    echo "Loading Kernel Drivers"
    depmod -a
    rmmod i2c-i801
    modprobe i2c-i801
    modprobe i2c-ismt
    modprobe i2c-dev
    modprobe i2c-mux
    modprobe i2c-smbus
    modprobe i2c-mux-pca954x
    modprobe d4_cpupld
    modprobe d4_cpld1
    modprobe d4_cpld2
    modprobe d4_cpld3
    modprobe fan_cpld
    modprobe acton_psu
    modprobe lm75
    modprobe at24
    modprobe optoe
}

d4_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_d4-r0/Nokia-IXR7220-D4-36D/profile.ini
    echo "Nokia-IXR7220-D4-36D: Updating switch mac address ${MAC_ADDR}"
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

# Enumerate I2C Multiplexers
echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-0/new_device
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-2/new_device
echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-2/new_device
echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-20/new_device
echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-21/new_device
echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-22/new_device
echo pca9546 0x73 > /sys/bus/i2c/devices/i2c-23/new_device
echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-24/new_device

# Enumerate system eeprom
echo 24c02 0x57 > /sys/bus/i2c/devices/i2c-0/new_device

#Enumerate CPLDs
echo d4_cpupld 0x65 > /sys/bus/i2c/devices/i2c-1/new_device
echo d4_cpld1  0x60 > /sys/bus/i2c/devices/i2c-10/new_device
echo d4_cpld2  0x62 > /sys/bus/i2c/devices/i2c-10/new_device
echo d4_cpld3  0x64 > /sys/bus/i2c/devices/i2c-10/new_device

# Bulk operations to (un)set lp mode/reset tranceivers
echo '0' > /sys/bus/i2c/devices/10-0060/bulk_qsfp_reset
sleep 2
echo '0' > /sys/bus/i2c/devices/10-0064/bulk_qsfp28_lpmod
echo '1' > /sys/bus/i2c/devices/10-0064/bulk_qsfpdd_lpmod
sleep 2
echo '1' > /sys/bus/i2c/devices/10-0060/bulk_qsfp_reset

# Set pca9548 idle_state as -2: MUX_IDLE_DISCONNECT
echo -2 > /sys/bus/i2c/devices/2-0071/idle_state
echo -2 > /sys/bus/i2c/devices/2-0072/idle_state
echo -2 > /sys/bus/i2c/devices/20-0073/idle_state
echo -2 > /sys/bus/i2c/devices/21-0073/idle_state
echo -2 > /sys/bus/i2c/devices/22-0073/idle_state
echo -2 > /sys/bus/i2c/devices/23-0073/idle_state
echo -2 > /sys/bus/i2c/devices/24-0073/idle_state

# PSU
echo acbel_fsh082 0x58 > /sys/bus/i2c/devices/i2c-9/new_device
echo acbel_fsh082 0x59 > /sys/bus/i2c/devices/i2c-9/new_device

# FANS
echo fan_cpld 0x66 > /sys/bus/i2c/devices/i2c-14/new_device

for chan in {25..60}; do
    echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${chan}/new_device
    sleep 0.1
done

file_exists /sys/bus/i2c/devices/0-0057/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/0-0057/eeprom
    d4_profile
else
    echo "SYSEEPROM file not found"
fi

exit 0
