#!/bin/bash

#platform init script for Nokia-IXR7220-H5-64D

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
    rmmod ee1004
    modprobe eeprom
    modprobe i2c-mux-gpio
    modprobe i2c-mux-pca954x
    modprobe sys_fpga
    modprobe emc2305
    modprobe dni_psu
    modprobe h5_64d_cpupld    
    modprobe h5_64d_portpld1
    modprobe h5_64d_portpld2    
    modprobe at24     
    modprobe optoe
}

nokia_7220H5_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h4_32d-r0/Nokia-IXR7220-H4-32D/profile.ini
    #echo "Nokia-7220-H4: Updating switch mac address ${MAC_ADDR}"
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

#insmod /lib/modules/6.1.0-11-2-amd64/delta_fpga.ko

#Enumerate I2C Multiplexers
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-3/new_device
for devnum in {4..11}; do
    echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-${devnum}/new_device 
    sleep 0.1
done

#Enumerate CPLDs
echo h5_portpld1 0x41 > /sys/bus/i2c/devices/i2c-2/new_device
echo h5_portpld2 0x45 > /sys/bus/i2c/devices/i2c-2/new_device

#Enumerate QSFPs and SFPs
echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-12/new_device
echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-13/new_device
for qsfpnum in {20..83}; do
	echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${qsfpnum}/new_device
	sleep 0.1
done

exit 0
