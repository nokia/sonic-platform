#!/bin/bash

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

    modprobe i2c-i801
    modprobe i2c-dev
    modprobe i2c-mux
    modprobe i2c-smbus
    modprobe i2c-ismt
    modprobe i2c-ocores
    modprobe h6_i2c_oc
    modprobe sys_mux
    modprobe drivetemp
}

h6-64_profile()
{
    MAC_ADDR=$(ip link show eth0 | grep ether | awk '{print $2}')
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h6_64-r0/Nokia-IXR7220-H6-64/profile.ini
    echo "Nokia-IXR7220-H6-64: Updated switch mac address ${MAC_ADDR}"
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

echo sys_fpga 0x60 > /sys/bus/i2c/devices/i2c-1/new_device

#Instantiated fpga mux
echo sys_mux 0x70 > /sys/bus/i2c/devices/i2c-1/new_device
echo sys_mux 0x71 > /sys/bus/i2c/devices/i2c-1/new_device
echo sys_mux 0x72 > /sys/bus/i2c/devices/i2c-73/new_device
echo sys_mux 0x73 > /sys/bus/i2c/devices/i2c-73/new_device
echo sys_mux 0x74 > /sys/bus/i2c/devices/i2c-73/new_device
echo sys_mux 0x76 > /sys/bus/i2c/devices/i2c-79/new_device
echo sys_mux 0x76 > /sys/bus/i2c/devices/i2c-80/new_device

#Instantiated system eeprom
echo 24c04 0x56 > /sys/bus/i2c/devices/i2c-99/new_device

echo sys_cpld 0x61 > /sys/bus/i2c/devices/i2c-73/new_device
echo port_cpld0 0x64 > /sys/bus/i2c/devices/i2c-89/new_device
echo port_cpld1 0x65 > /sys/bus/i2c/devices/i2c-90/new_device

# Fan
echo h6_fan 0x33 > /sys/bus/i2c/devices/i2c-79/new_device
echo h6_fan 0x33 > /sys/bus/i2c/devices/i2c-80/new_device

echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-112/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-113/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-114/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-115/new_device

echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-120/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-121/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-122/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-123/new_device

for index in {5..68}; do
	echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${index}/new_device
done
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-69/new_device
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-70/new_device

#PSU
for i in {0..3}; do
    pres="/sys/bus/i2c/devices/73-0061/psu$((i+1))_pres"
    bus_path="/sys/bus/i2c/devices/i2c-$((94+i))/new_device" # 94~97
    pmbus_addr=$((0x58 + i))    # 0x58~0x5B
    eeprom_addr=$((0x50 + i))   # 0x50~0x53

    if [ "$(cat "$pres")" = "0" ]; then
        echo "pmbus_psu $(printf '0x%X' $pmbus_addr)"   > "$bus_path"
        echo "eeprom_fru $(printf '0x%X' $eeprom_addr)" > "$bus_path"
    fi
done

# thermal
echo embd_ctrl 0x21 > /sys/bus/i2c/devices/i2c-0/new_device
echo jc42 0x18 > /sys/bus/i2c/devices/i2c-0/new_device
echo lm75 0x48 > /sys/bus/i2c/devices/i2c-77/new_device
echo lm75 0x49 > /sys/bus/i2c/devices/i2c-77/new_device
echo tmp464 0x48 > /sys/bus/i2c/devices/i2c-101/new_device
echo lm75 0x48 > /sys/bus/i2c/devices/i2c-98/new_device
echo lm75 0x49 > /sys/bus/i2c/devices/i2c-98/new_device
echo lm75 0x4a > /sys/bus/i2c/devices/i2c-98/new_device
echo lm75 0x4b > /sys/bus/i2c/devices/i2c-98/new_device
echo lm75 0x4c > /sys/bus/i2c/devices/i2c-98/new_device
echo lm75 0x4d > /sys/bus/i2c/devices/i2c-98/new_device
echo lm75 0x4d > /sys/bus/i2c/devices/i2c-111/new_device
echo lm75 0x4d > /sys/bus/i2c/devices/i2c-119/new_device

for ch in {1..8}; do
    echo 60 > /sys/bus/i2c/devices/79-0033/hwmon/hwmon*/fan${ch}_pwm
    echo 60 > /sys/bus/i2c/devices/80-0033/hwmon/hwmon*/fan${ch}_pwm
done

file_exists /sys/bus/i2c/devices/99-0056/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/99-0056/eeprom
else
    echo "SYSEEPROM file not found"
fi

h6-64_profile

exit

