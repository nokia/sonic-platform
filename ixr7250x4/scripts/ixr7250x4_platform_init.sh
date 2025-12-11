#!/bin/bash

# platform init script for Nokia IXR7250 X4

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
    modprobe rtc_ds1307
    modprobe optoe
    modprobe at24
    modprobe max31790_wd
    modprobe jc42
    modprobe psu_x3b
    modprobe fan_eeprom
    modprobe psu_eeprom
    modprobe fan_led
    modprobe pcon
}

x4_profile()
{
    MAC_ADDR=$(sudo decode-syseeprom -m)
    if [ $? -eq 0 ]; then
      sed -i "s/switchMacAddress=.*/switchMacAddress=$MAC_ADDR/g" /usr/share/sonic/device/x86_64-nokia_ixr7250_x4-r0/Nokia-IXR7250-X4/profile.ini
      echo "Nokia-IXR7250-X4: Updated switch mac address ${MAC_ADDR}"
    fi
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

dev_conf_init() {
    CONF_FILE=/var/run/sonic-platform-nokia/devices.conf
    mkdir -p /var/run/sonic-platform-nokia/
    cat /dev/null > CONF_FILE

    echo board=x4 >> $CONF_FILE

    echo "cpctl=/sys/bus/pci/drivers/cpuctl/0000:01:00.0/" >> $CONF_FILE
    echo "ioctl=/sys/bus/pci/drivers/cpuctl/0000:05:00.0/" >> $CONF_FILE

    # pcons on x4
    echo pcon 0x74 > /sys/bus/i2c/devices/i2c-23/new_device
    echo pcon 0x74 > /sys/bus/i2c/devices/i2c-24/new_device
    echo pconm 0x74 > /sys/bus/i2c/devices/i2c-17/new_device

    file_exists /sys/bus/i2c/drivers/pcon/23-0074/name
    file_exists /sys/bus/i2c/drivers/pcon/24-0074/name
    file_exists /sys/bus/i2c/drivers/pcon/17-0074/name

    PCON0_HWMON=$(ls /sys/bus/i2c/drivers/pcon/23-0074/hwmon/)
    echo "pcon0=/sys/class/hwmon/${PCON0_HWMON}" >> $CONF_FILE
    PCON1_HWMON=$(ls /sys/bus/i2c/drivers/pcon/24-0074/hwmon/)
    echo "pcon1=/sys/class/hwmon/${PCON1_HWMON}" >> $CONF_FILE
    PCON2_HWMON=$(ls /sys/bus/i2c/drivers/pcon/17-0074/hwmon/)
    echo "pcon2=/sys/class/hwmon/${PCON2_HWMON}" >> $CONF_FILE
}

# Install kernel drivers required for i2c bus access
load_kernel_drivers

echo m41t11 0x68 > /sys/bus/i2c/devices/i2c-7/new_device

# Enumerate system eeprom
file_exists /sys/bus/i2c/devices/i2c-1/new_device
echo 24c64 0x54 > /sys/bus/i2c/devices/i2c-1/new_device

hwclock -s -f /dev/rtc1

dev_conf_init

if type sets_setup &> /dev/null ; then
    sets_setup -d
fi

if type asic_rov_config &> /dev/null ; then 
    asic_rov_config -v
fi

# take asics out of reset
/etc/init.d/opennsl-modules stop
echo 1 > /sys/bus/pci/drivers/cpuctl/0000:05:00.0/jer_reset_seq
sleep 1
/etc/init.d/opennsl-modules start

echo jc42 0x18 > /sys/bus/i2c/devices/i2c-0/new_device
echo jc42 0x19 > /sys/bus/i2c/devices/i2c-0/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-1/new_device
echo tmp421 0x1e > /sys/bus/i2c/devices/i2c-7/new_device
echo tmp75 0x49 > /sys/bus/i2c/devices/i2c-19/new_device
echo tmp75 0x4a > /sys/bus/i2c/devices/i2c-19/new_device
echo tmp75 0x4b > /sys/bus/i2c/devices/i2c-19/new_device

#fan
for idx in {1..3}
do
    prs=$(cat /sys/bus/pci/devices/0000:01:00.0/fandraw_${idx}_prs)
    if [ "$prs" == "0" ]; then
        echo max31790_wd 0x20 > /sys/bus/i2c/devices/i2c-$((${idx}+10))/new_device
        echo fan_led 0x60 > /sys/bus/i2c/devices/i2c-$((${idx}+10))/new_device
        echo fan_eeprom 0x54 > /sys/bus/i2c/devices/i2c-$((${idx}+10))/new_device
    fi
done

# PSU
echo psu_x3b 0x5b > /sys/bus/i2c/devices/i2c-14/new_device
echo psu_x3b 0x5b > /sys/bus/i2c/devices/i2c-15/new_device
echo psu_eeprom 0x53 > /sys/bus/i2c/devices/i2c-14/new_device
echo psu_eeprom 0x53 > /sys/bus/i2c/devices/i2c-15/new_device

for num in {27..62}; do
    echo optoe1 0x50 > /sys/bus/i2c/devices/i2c-${num}/new_device
done

for num in {27..58}; do
    echo 300 > /sys/bus/i2c/devices/${num}-0050/write_timeout
done

file_exists /sys/bus/i2c/devices/1-0054/eeprom
status=$?
if [ "$status" == "1" ]; then
    chmod 644 /sys/bus/i2c/devices/1-0054/eeprom
    x4_profile
else
    echo "SYSEEPROM file not foud"
fi

if type pcon_cmds &> /dev/null ; then 
    pcon_cmds -v -r /var/run/sonic-platform-nokia/pcon_reboot_reason
fi

if type sets_setup &> /dev/null ; then 
    sets_setup --wait-lock
fi

exit 0
