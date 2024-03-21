#!/bin/bash

#platform init script for Nokia IXR7220 H3

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
    modprobe i2c-mux-gpio
    modprobe i2c-mux-pca954x
    modprobe emc2305
    modprobe dni_psu    
    modprobe nokia_7220_h3_cpupld
    modprobe nokia_7220_h3_swpld1
    modprobe nokia_7220_h3_swpld2
    modprobe nokia_7220_h3_swpld3    
    modprobe at24     
    modprobe optoe
}

nokia_7220H3_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h3-r0/Nokia-IXR7220-H3/profile.ini
    #echo "Nokia-7220-H3: Updating switch mac address ${MAC_ADDR}"
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

file_exists /sys/bus/i2c/devices/i2c-0/new_device
# Enumerate I2C Multiplexer
echo pca9548 0x77 > /sys/bus/i2c/devices/i2c-0/new_device

# Enumerate system eeprom
echo 24c02 0x53 > /sys/bus/i2c/devices/i2c-0/new_device

#file_exists /sys/bus/i2c/devices/i2c-2/new_device
# Enumerate I2C Multiplexer
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-2/new_device

#file_exists /sys/bus/i2c/devices/i2c-3/new_device
# Enumerate I2C Multiplexer
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-3/new_device

# Enumerate I2C Multiplexer
echo pca9548 0x71 > /sys/bus/i2c/devices/i2c-3/new_device

# Enumerate I2C Multiplexer
echo pca9548 0x72 > /sys/bus/i2c/devices/i2c-3/new_device

# Enumerate I2C Multiplexer
echo pca9548 0x73 > /sys/bus/i2c/devices/i2c-3/new_device

# Enumerate I2C Multiplexer
echo pca9548 0x74 > /sys/bus/i2c/devices/i2c-3/new_device

# Enumerate PSU1 
echo dps_1600ab_29_a  0x58 > /sys/bus/i2c/devices/i2c-10/new_device
# Enumerate PSU2
echo dps_1600ab_29_a  0x58 > /sys/bus/i2c/devices/i2c-11/new_device

# Enumerate I2C Multiplexer
echo pca9548 0x75 > /sys/bus/i2c/devices/i2c-12/new_device

# Enumerate Fan SPD controller
echo emc2305 0x2e > /sys/bus/i2c/devices/i2c-13/new_device

# Enumerate Fan SPD controller
echo emc2305 0x4c > /sys/bus/i2c/devices/i2c-13/new_device

# Enumerate Fan SPD controller
echo emc2305 0x2d > /sys/bus/i2c/devices/i2c-13/new_device

# Enumerate PCA9555(GPIO Mux)
echo pca9555 0x27 > /sys/bus/i2c/devices/i2c-64/new_device

# Enumerate Thermal Sensor (FANBD)
echo tmp75 0x4f > /sys/bus/i2c/devices/i2c-13/new_device

# Enumerate Thermal Sensor (RF)
echo tmp75 0x4b > /sys/bus/i2c/devices/i2c-14/new_device

# Enumerate Thermal Sensor (LF)
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-14/new_device

# Enumerate Thermal Sensor (UPPER MAC)
echo tmp75 0x4a > /sys/bus/i2c/devices/i2c-14/new_device

# Enumerate Thermal Sensor (LOWER MAC)
echo tmp75 0x4e > /sys/bus/i2c/devices/i2c-14/new_device

# Enumerate Thermal Sensor (CPU)
echo tmp75 0x4d > /sys/bus/i2c/devices/i2c-14/new_device

# Enumerate Voltage Ragulator (0V8)
echo tps53647 0x65 > /sys/bus/i2c/devices/i2c-15/new_device

# Enumerate Voltage Ragulator (0V9)
echo ir35221 0x10 > /sys/bus/i2c/devices/i2c-15/new_device

# Enumerate Voltage Ragulator (3V3)
echo tps53667 0x64 > /sys/bus/i2c/devices/i2c-15/new_device

#Enumerate CPLDs
echo nokia_7220h3_cpupld 0x31 > /sys/bus/i2c/devices/i2c-0/new_device
echo nokia_7220h3_swpld1 0x32 > /sys/bus/i2c/devices/i2c-17/new_device
echo nokia_7220h3_swpld2 0x34 > /sys/bus/i2c/devices/i2c-17/new_device
echo nokia_7220h3_swpld3 0x35 > /sys/bus/i2c/devices/i2c-17/new_device

# Enumerate the QSFP-DD devices on each mux channel
for chan in {18..49}
do
    echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${chan}/new_device
done

# Enumerate the SFP+ devices on each mux channel
echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-50/new_device
echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-51/new_device

# Enumerate Fans(0-5) eeprom
for chan in {0..5}
do
    echo 24c02 0x50 > /sys/bus/i2c/devices/12-0075/channel-${chan}/new_device
done

file_exists /sys/bus/i2c/devices/0-0053/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/0-0053/eeprom
else
    echo "SYSEEPROM file not foud"
fi

# Ensure switch is programmed with base MAC addr
nokia_7220H3_profile

#Enumerate GPIO port
for port in {4..9}
do
    echo 1022${port} > /sys/class/gpio/export
done

for fan in {58..63}
do
    chmod 644 /sys/bus/i2c/devices/${fan}-0050/eeprom
done

# Set the PCA9548 mux behavior
echo -2 > /sys/bus/i2c/devices/3-0070/idle_state
echo -2 > /sys/bus/i2c/devices/3-0071/idle_state
echo -2 > /sys/bus/i2c/devices/3-0072/idle_state
echo -2 > /sys/bus/i2c/devices/3-0073/idle_state
echo -2 > /sys/bus/i2c/devices/3-0074/idle_state

exit 0
