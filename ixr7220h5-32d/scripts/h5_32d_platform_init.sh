#!/bin/bash

#platform init script for Nokia-IXR7220-H5-32D

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
    modprobe i2c-mux-gpio
    modprobe i2c-mux-pca954x
    modprobe sys_fpga
    modprobe cpupld
    modprobe swpld2
    modprobe swpld3
    modprobe at24
    modprobe eeprom_tlv
    modprobe eeprom_fru
    modprobe optoe
    modprobe dni_psu
    modprobe emc2305
}

h5_32d_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h5_32d-r0/Nokia-IXR7220-H5-32D/profile.ini
    echo "Nokia-7220-H5-32D: Updated switch mac address ${MAC_ADDR}"
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
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-4/new_device
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-4/new_device
echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-18/new_device

# Enumerate CPLDs
echo cpupld 0x40 > /sys/bus/i2c/devices/i2c-4/new_device

# Enumerate system eeprom
echo 24c02 0x54 > /sys/bus/i2c/devices/i2c-4/new_device

# Enumerate PSU 
echo dni_psu 0x58 > /sys/bus/i2c/devices/i2c-15/new_device
echo dni_psu 0x59 > /sys/bus/i2c/devices/i2c-15/new_device
echo eeprom_fru 0x50 > /sys/bus/i2c/devices/i2c-15/new_device
echo eeprom_fru 0x51 > /sys/bus/i2c/devices/i2c-15/new_device

# Enumerate Thermal Sensor 
echo tmp1075 0x49 > /sys/bus/i2c/devices/i2c-16/new_device
echo tmp1075 0x4a > /sys/bus/i2c/devices/i2c-16/new_device
echo tmp1075 0x4b > /sys/bus/i2c/devices/i2c-16/new_device
echo tmp1075 0x4e > /sys/bus/i2c/devices/i2c-16/new_device
echo tmp1075 0x4f > /sys/bus/i2c/devices/i2c-16/new_device
echo tmp1075 0x4e > /sys/bus/i2c/devices/i2c-17/new_device
echo tmp1075 0x4f > /sys/bus/i2c/devices/i2c-17/new_device
echo tmp1075 0x48 > /sys/bus/i2c/devices/i2c-20/new_device

# Enumerate Fan controller
echo emc2305 0x2d > /sys/bus/i2c/devices/i2c-17/new_device
echo emc2305 0x4c > /sys/bus/i2c/devices/i2c-17/new_device
echo emc2305 0x4d > /sys/bus/i2c/devices/i2c-17/new_device
for ch in {1..5}; do
    echo 128 > /sys/bus/i2c/devices/17-002d/hwmon/hwmon*/pwm${ch}
    echo 128 > /sys/bus/i2c/devices/17-004c/hwmon/hwmon*/pwm${ch}
    echo 128 > /sys/bus/i2c/devices/17-004d/hwmon/hwmon*/pwm${ch}
done

# Enumerate PCA9555(GPIO Mux)
echo pca9555 0x27 > /sys/bus/i2c/devices/i2c-18/new_device

#Enumerate CPLDs
echo swpld2 0x41 > /sys/bus/i2c/devices/i2c-21/new_device
echo swpld3 0x45 > /sys/bus/i2c/devices/i2c-21/new_device

for devnum in {5..13}; do
    echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-${devnum}/new_device
    sleep 0.1
done

#Enumerate QSFPs and SFPs
for qsfpnum in {38..101}; do
	echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${qsfpnum}/new_device
	sleep 0.1
done

echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-102/new_device
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-103/new_device

# Set the PCA9548 mux behavior
echo -2 > /sys/bus/i2c/devices/4-0070/idle_state
echo -2 > /sys/bus/i2c/devices/4-0071/idle_state
echo -2 > /sys/bus/i2c/devices/5-0070/idle_state
echo -2 > /sys/bus/i2c/devices/6-0070/idle_state
echo -2 > /sys/bus/i2c/devices/7-0070/idle_state
echo -2 > /sys/bus/i2c/devices/8-0070/idle_state
echo -2 > /sys/bus/i2c/devices/9-0070/idle_state
echo -2 > /sys/bus/i2c/devices/10-0070/idle_state
echo -2 > /sys/bus/i2c/devices/11-0070/idle_state
echo -2 > /sys/bus/i2c/devices/12-0070/idle_state
echo -2 > /sys/bus/i2c/devices/13-0070/idle_state
echo -2 > /sys/bus/i2c/devices/18-0072/idle_state

# Enumerate fan trays (0-6) eeprom
for chan in {0..6}
do
    echo eeprom_tlv 0x50 > /sys/bus/i2c/devices/18-0072/channel-${chan}/new_device
done

file_exists /sys/bus/i2c/devices/4-0054/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/4-0054/eeprom
    h5_32d_profile
else
    echo "The SYSEEPROM file is not found."
fi

#Enumerate GPIO port
for port in {0..6}
do
    echo 80${port} > /sys/class/gpio/export
    echo in > /sys/class/gpio/gpio80${port}/direction
    sleep 0.1
done

exit 0
