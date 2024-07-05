#!/bin/bash

# Load required kernel-mode drivers

load_kernel_drivers() {
    echo "Loading Kernel Drivers"  
    depmod -a
    modprobe i2c-i801
    modprobe i2c-dev
    modprobe i2c-mux
    modprobe i2c-smbus
    modprobe i2c-mux-pca954x
    modprobe sys_fpga
    modprobe smb_cpld
    modprobe fan_cpld
    modprobe scm_cpld
    modprobe pdbl_cpld
    modprobe pdbr_cpld
    modprobe dni_psu
    modprobe at24
    modprobe optoe
    sleep 1
}

h4_64d_profile()
{
    MAC_ADDR=$(ip link show eth0 | grep ether | awk '{print $2}')
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h4-r0/Nokia-IXR7220-H4-64D/profile.ini
    echo "Nokia-IXR7220-H4-64D: Updated switch mac address ${MAC_ADDR}"
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

# Enumerate I2C Multiplexer
echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-0/new_device
sleep 0.5
# Enumerate I2C Multiplexers
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-3/new_device
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-5/new_device
echo pca9548 0x76 > /sys/bus/i2c/devices/i2c-9/new_device
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-10/new_device
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-11/new_device
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-12/new_device
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-18/new_device
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-19/new_device

# # Enumerate system eeprom
echo 24c32 0x51 > /sys/bus/i2c/devices/i2c-20/new_device
echo 24c02 0x50 > /sys/bus/i2c/devices/i2c-33/new_device
echo 24c02 0x51 > /sys/bus/i2c/devices/i2c-41/new_device

# Enumerate CPLDs
echo smb_cpld 0x60 > /sys/bus/i2c/devices/i2c-6/new_device
echo fan_cpld 0x33 > /sys/bus/i2c/devices/i2c-25/new_device
echo scm_cpld 0x35 > /sys/bus/i2c/devices/i2c-51/new_device
echo pdbl_cpld 0x60 > /sys/bus/i2c/devices/i2c-36/new_device
echo pdbr_cpld 0x60 > /sys/bus/i2c/devices/i2c-44/new_device

echo dps_2400ab_1 0x58 > /sys/bus/i2c/devices/i2c-33/new_device
echo dps_2400ab_1 0x59 > /sys/bus/i2c/devices/i2c-41/new_device

# Enumerate Thermal Sensor 
echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-2/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-2/new_device
echo tmp422 0x4c > /sys/bus/i2c/devices/i2c-14/new_device
# UDB:
echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-57/new_device
echo tmp422 0x4C > /sys/bus/i2c/devices/i2c-58/new_device
# LDB:
echo tmp75 0x4c > /sys/bus/i2c/devices/i2c-65/new_device
echo tmp422 0x4d > /sys/bus/i2c/devices/i2c-66/new_device

echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-27/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-27/new_device
echo tmp75 0x48 > /sys/bus/i2c/devices/i2c-34/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-42/new_device

# # Set the PCA9548 mux behavior
echo -2 > /sys/bus/i2c/devices/0-0072/idle_state
echo -2 > /sys/bus/i2c/devices/3-0071/idle_state
echo -2 > /sys/bus/i2c/devices/5-0071/idle_state
echo -2 > /sys/bus/i2c/devices/9-0076/idle_state
echo -2 > /sys/bus/i2c/devices/10-0070/idle_state
echo -2 > /sys/bus/i2c/devices/11-0070/idle_state
echo -2 > /sys/bus/i2c/devices/12-0070/idle_state
echo -2 > /sys/bus/i2c/devices/18-0070/idle_state
echo -2 > /sys/bus/i2c/devices/19-0070/idle_state

file_exists /sys/bus/i2c/devices/20-0051/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/20-0051/eeprom
    h4_64d_profile
else
    echo "SYSEEPROM file not foud"
fi

exit 0
