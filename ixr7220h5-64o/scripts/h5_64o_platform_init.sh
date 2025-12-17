#!/bin/bash

#platform init script for Nokia-IXR7220-H5-64O

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

h5_64o_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h5_64o-r0/Nokia-IXR7220-H5-64O/profile.ini
    echo "Nokia-7220-H5-64O: Updated switch mac address ${MAC_ADDR}"
    ifconfig eth0 hw ether ${MAC_ADDR}
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

#Enumerate I2C Multiplexers
echo pca9548 0x70 > /sys/bus/i2c/devices/i2c-4/new_device
echo pca9546 0x72 > /sys/bus/i2c/devices/i2c-18/new_device

#Enumerate CPLDs
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
echo tmp1075 0x49 > /sys/bus/i2c/devices/i2c-19/new_device
echo tmp1075 0x48 > /sys/bus/i2c/devices/i2c-20/new_device

# Enumerate Fan controller
echo emc2305 0x2d > /sys/bus/i2c/devices/i2c-17/new_device
echo emc2305 0x4c > /sys/bus/i2c/devices/i2c-17/new_device
for ch in {1..5}; do
    echo 128 > /sys/bus/i2c/devices/17-002d/hwmon/hwmon*/pwm${ch}
    echo 128 > /sys/bus/i2c/devices/17-004c/hwmon/hwmon*/pwm${ch}
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

for num in {26..89}; do
	echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${num}/new_device
	sleep 0.1
done
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-90/new_device
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-91/new_device

# Set the PCA9548 mux behavior
echo -2 > /sys/bus/i2c/devices/4-0070/idle_state
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

# Enumerate Fans(0-3) eeprom
for chan in {0..3}
do
    echo eeprom_tlv 0x50 > /sys/bus/i2c/devices/18-0072/channel-${chan}/new_device
done

file_exists /sys/bus/i2c/devices/4-0054/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/4-0054/eeprom
    h5_64o_profile
else
    echo "SYSEEPROM file not found"
fi

#Enumerate GPIO port
for port in {0..3}
do
    echo 80${port} > /sys/class/gpio/export
    echo in > /sys/class/gpio/gpio80${port}/direction
    sleep 0.1
done

check_voltage() {
    status=$(cat /sys/kernel/sys_fpga/psu$(($1))_ok)
    model=$(cat /sys/bus/i2c/devices/15-005$(($1-1))/part_number)
    if [[ ${status[0]} == "0x0" ]] && [[ ${model:0:10} == "3HE20598AA" ]]; then
	    vol_i=$(cat /sys/bus/i2c/devices/15-005$((7+$1))/psu_v_in)
        vol_d=$(echo "$vol_i 1000" | awk '{printf "%0.3f\n", $1/$2}')
        if (("$vol_i" < 170000)); then
            echo -e "\nERROR: PSU $1 not supplying enough voltage. [$vol_d]v is less than the required 200-220V\n"
        fi
    fi
    return 0
}
check_voltage 1
check_voltage 2

retimer() {
    i2cset -y 21 0x27 0xfc 0x1
    i2cset -y 21 0x27 0xff 0x3
    i2cset -y 21 0x27 0x00 0x4
    i2cset -y 21 0x27 0x0a 0xc
    i2cset -y 21 0x27 0x2f 0x00
    i2cset -y 21 0x27 0x31 0x40
    i2cset -y 21 0x27 0x1e 0x02
    i2cset -y 21 0x27 0x0a 0x00
}

retimer
exit 0
