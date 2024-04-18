#!/bin/bash

#platform init script for Nokia-IXR7220-H4-32D

# Load required kernel-mode drivers
load_kernel_drivers() {
    echo "Loading Kernel Drivers"  
    depmod -a 
    rmmod i2c-i801
    rmmod i2c-ismt  
    modprobe i2c-ismt
    modprobe i2c-i801
    modprobe i2c-dev
    modprobe i2c-mux
    modprobe i2c-smbus
    modprobe sys_fpga
    modprobe i2c-mux-gpio
    modprobe i2c-mux-pca954x        
    modprobe h4_32d_cpupld
    modprobe h4_32d_swpld2
    modprobe h4_32d_swpld3
    modprobe dni_psu
    modprobe emc2305
    modprobe at24
    modprobe optoe
    modprobe isl68137 
    sleep 1
}

h4_32d_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h4_32d-r0/IXR7220-H4-32D/profile.ini
    echo "Nokia-IXR7220-H4-32D: Updating switch mac address ${MAC_ADDR}"
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
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-0/new_device

#Enumerate I2C Multiplexers
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-10/new_device 
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-11/new_device 
echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-12/new_device 
echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-13/new_device 

#Enumerate CPLDs
echo h4_32d_cpupld 0x31 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate system eeprom
echo 24c02 0x53 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate I2C Multiplexer
echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-4/new_devicee

# Enumerate PSU 
echo dps_1600ab_29_a  0x58 > /sys/bus/i2c/devices/i2c-2/new_device
echo dps_1600ab_29_a  0x58 > /sys/bus/i2c/devices/i2c-3/new_device

# Enumerate Fan SPD controller
echo emc2305 0x4c > /sys/bus/i2c/devices/i2c-5/new_device
echo emc2305 0x2d > /sys/bus/i2c/devices/i2c-5/new_device
echo emc2305 0x2e > /sys/bus/i2c/devices/i2c-5/new_device

# Enumerate Thermal Sensor 
echo tmp75 0x4f > /sys/bus/i2c/devices/i2c-5/new_device
echo tmp75 0x4b > /sys/bus/i2c/devices/i2c-6/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-6/new_device
echo tmp75 0x4a > /sys/bus/i2c/devices/i2c-6/new_device
echo tmp75 0x4e > /sys/bus/i2c/devices/i2c-6/new_device
echo tmp75 0x4d > /sys/bus/i2c/devices/i2c-6/new_device

# Enumerate Voltage Ragulator
echo isl68239 0x65 > /sys/bus/i2c/devices/i2c-7/new_device
echo isl68239 0x63 > /sys/bus/i2c/devices/i2c-7/new_device
echo isl68239 0x64 > /sys/bus/i2c/devices/i2c-7/new_device

# Enumerate PCA9555(GPIO Mux)
echo pca9555 0x27 > /sys/bus/i2c/devices/i2c-22/new_device

#Enumerate PortPLDs
echo h4_32d_swpld2 0x34 > /sys/bus/i2c/devices/i2c-9/new_device
echo h4_32d_swpld3 0x35 > /sys/bus/i2c/devices/i2c-9/new_device

#Enumerate QSFPs and SFPs
echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-14/new_device
for qsfpnum in {23..54}; do
	echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${qsfpnum}/new_device
	sleep 0.1
done

# Enumerate Fans(0-5) eeprom
for chan in {0..6}
do
    echo 24c02 0x50 > /sys/bus/i2c/devices/4-0075/channel-${chan}/new_device
done

file_exists /sys/bus/i2c/devices/1-0053/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/1-0053/eeprom
    h4_32d_profile
else
    echo "SYSEEPROM file not foud"
fi

#Enumerate GPIO port
for port in {4..9}
do
    echo 1022${port} > /sys/class/gpio/export
done

for fan in {15..21}
do
    chmod 644 /sys/bus/i2c/devices/${fan}-0050/eeprom
done

exit 0
