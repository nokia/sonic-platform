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

    modprobe i2c-i801
    modprobe i2c-dev
    modprobe i2c-mux
    modprobe i2c-smbus
    modprobe i2c-ismt
    modprobe i2c-ocores
    modprobe h6_i2c_oc
}

h6-128_profile()
{
    MAC_ADDR=$(ip link show eth0 | grep ether | awk '{print $2}')
    sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7220_h6_128-r0/Nokia-IXR7220-H6-128/profile.ini
    echo "Nokia-IXR7220-H6-128: Updated switch mac address ${MAC_ADDR}"
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

load_kernel_drivers

sleep 1
i2cset -y 1 0x60 0xf 0x1
sleep 1
i2cset -y 1 0x77 0x0 0x03
sleep 2
i2cset -y 1 0x71 0xc 0x3f
sleep 1

echo sys_fpga 0x60 > /sys/bus/i2c/devices/i2c-1/new_device

echo mux_fpga 0x77 > /sys/bus/i2c/devices/i2c-1/new_device
sleep 1
echo mux_sys_cpld 0x72 > /sys/bus/i2c/devices/i2c-134/new_device

echo mux_fcm 0x75 > /sys/bus/i2c/devices/i2c-144/new_device
echo mux_fcm 0x76 > /sys/bus/i2c/devices/i2c-145/new_device
sleep 1
echo mux_mezz 0x75 > /sys/bus/i2c/devices/i2c-150/new_device
echo mux_mezz 0x75 > /sys/bus/i2c/devices/i2c-151/new_device
echo mux_mezz 0x75 > /sys/bus/i2c/devices/i2c-152/new_device
echo mux_mezz 0x75 > /sys/bus/i2c/devices/i2c-153/new_device

echo port_cpld0 0x74 > /sys/bus/i2c/devices/i2c-148/new_device
echo port_cpld1 0x75 > /sys/bus/i2c/devices/i2c-149/new_device
echo port_cpld2 0x73 > /sys/bus/i2c/devices/i2c-150/new_device
echo port_cpld2 0x73 > /sys/bus/i2c/devices/i2c-151/new_device
echo port_cpld2 0x76 > /sys/bus/i2c/devices/i2c-152/new_device
echo port_cpld2 0x76 > /sys/bus/i2c/devices/i2c-153/new_device

echo 24c04 0x56 > /sys/bus/i2c/devices/i2c-173/new_device
echo 24c04 0x56 > /sys/bus/i2c/devices/i2c-176/new_device
echo 24c04 0x56 > /sys/bus/i2c/devices/i2c-179/new_device
echo 24c04 0x56 > /sys/bus/i2c/devices/i2c-182/new_device

echo 24c04 0x56 > /sys/bus/i2c/devices/i2c-158/new_device

echo h6_fan 0x32 > /sys/bus/i2c/devices/i2c-144/new_device
echo h6_fan 0x33 > /sys/bus/i2c/devices/i2c-145/new_device

echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-163/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-164/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-165/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-166/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-169/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-170/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-171/new_device
echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-172/new_device

for i in {0..3}; do
    pres="/sys/bus/i2c/devices/1-0060/psu$((i+1))_pres"
    bus_path="/sys/bus/i2c/devices/i2c-$((136+i))/new_device"
    pmbus_addr=$((0x58 + i))
    eeprom_addr=$((0x50 + i))

    if [ "$(cat "$pres")" = "0" ]; then
        echo "pmbus_psu $(printf '0x%X' $pmbus_addr)"   > "$bus_path"
        echo "eeprom_fru $(printf '0x%X' $eeprom_addr)" > "$bus_path"
    fi
done

echo embd_ctrl 0x21 > /sys/bus/i2c/devices/i2c-0/new_device
echo jc42 0x18 > /sys/bus/i2c/devices/i2c-0/new_device
echo lm75 0x48 > /sys/bus/i2c/devices/i2c-143/new_device
echo lm75 0x48 > /sys/bus/i2c/devices/i2c-154/new_device
echo lm75 0x49 > /sys/bus/i2c/devices/i2c-154/new_device
echo lm75 0x4A > /sys/bus/i2c/devices/i2c-154/new_device
echo lm75 0x4B > /sys/bus/i2c/devices/i2c-154/new_device
echo lm75 0x4C > /sys/bus/i2c/devices/i2c-154/new_device
echo lm75 0x4D > /sys/bus/i2c/devices/i2c-154/new_device

echo tmp75 0x4D > /sys/bus/i2c/devices/i2c-161/new_device
echo tmp75 0x4E > /sys/bus/i2c/devices/i2c-162/new_device
echo tmp75 0x4D > /sys/bus/i2c/devices/i2c-167/new_device
echo tmp75 0x4E > /sys/bus/i2c/devices/i2c-168/new_device

echo lm75 0x48 > /sys/bus/i2c/devices/i2c-174/new_device
echo lm75 0x48 > /sys/bus/i2c/devices/i2c-177/new_device
echo lm75 0x48 > /sys/bus/i2c/devices/i2c-180/new_device
echo lm75 0x48 > /sys/bus/i2c/devices/i2c-183/new_device

file_exists /sys/bus/i2c/devices/158-0056/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/158-0056/eeprom
else
    echo "SYSEEPROM file not found"
fi

for index in {2..129}; do
	echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${index}/new_device
done
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-130/new_device
echo optoe2 0x50 > /sys/bus/i2c/devices/i2c-131/new_device

exit

for ch in {1..8}; do
    echo 60 > /sys/bus/i2c/devices/144-0032/hwmon/hwmon*/fan${ch}_pwm
    echo 60 > /sys/bus/i2c/devices/145-0033/hwmon/hwmon*/fan${ch}_pwm
done

h6-128_profile

exit


