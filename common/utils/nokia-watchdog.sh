#!/bin/bash

# err_func() {
#   echo "Error raised! " `date -u` >> /var/log/nokia-watchdog-$suf.log
#   exec 5>&-
#}

# trap 'err_func' ERR

#Function to check for health of NDK platform processes
#Allow a min_app_count_failures for process restarts
app_count=0
min_app_count_failures=3
function platform_ndk_health_monitor()
{
    if [ -f /opt/srlinux/bin/platform_ndk_health_check.py ]; then
        python3 /opt/srlinux/bin/platform_ndk_health_check.py
        if [[ $? = 0 ]]; then
            app_count=0
        else
            (( app_count++ ))
        fi
    else
        app_count=0
    fi
}

watchdog_sleep_time=30
start_date=`date -u`

set -x

suf=`date -u "+%Y%m%d%H%M%S"`

wd_init_log_file=/var/log/nokia-watchdog-init-$suf.log
echo "Started at $start_date" > $wd_init_log_file
ls -las /dev/watch* >> $wd_init_log_file
stat /dev/watch* >> $wd_init_log_file

# remove sp5100 driver
lsmod | grep 5100 >> $wd_init_log_file
modprobe -v -r sp5100_tco >> $wd_init_log_file

# ls -las /lib/modules/4.19.0-9-2-amd64/nokia* >> $wd_init_log_file
# find /host -name nokia_gpio_wdt.ko >> $wd_init_log_file
# find /host -name blacklist.conf >> $wd_init_log_file
echo "check for /dev/watchdog at " `date -u` >> $wd_init_log_file

i=0
while [ ! -e /dev/watchdog ] && [ $i -le 10 ] ; do 
   ((i+=1))
   sleep 1
done	

echo "done check $i for /dev/watchdog at " `date -u` >> $wd_init_log_file

ls -las /dev/watch* >> $wd_init_log_file
stat /dev/watch* >> $wd_init_log_file

depmod -A
modprobe -v nokia_gpio_wdt >> $wd_init_log_file
echo "after modprobe" >> $wd_init_log_file
sleep 2

ls -las /dev/watch* >> $wd_init_log_file
stat /dev/watch* >> $wd_init_log_file

# keep it open while we write "w" to it in loop

# exec 5>/dev/watchdog
# echo "w" >&5
exec {desc}>/dev/watchdog
echo "w" >&${desc}

echo "first kick done using desc $desc at " `date -u` >> $wd_init_log_file

# find /host -name nokia_gpio_wdt.ko >> $wd_init_log_file
# find /host -name blacklist.conf >> $wd_init_log_file

lsmod | grep nokia >> $wd_init_log_file
lsmod | grep 5100 >> $wd_init_log_file
dmesg | grep nokia >> $wd_init_log_file
sync
echo "sync done " `date -u` >> $wd_init_log_file


watchdog_fail=1
wd_log_file=/var/log/nokia-watchdog-$suf.log
echo "start watchdog monitoring" > $wd_log_file

#Option to disable watchdog
source /host/machine.conf
PLATFORM_NDK_JSON="/usr/share/sonic/device/${onie_platform}/platform_ndk.json"
disable_watchdog_hm=$(jq -r '.options' ${PLATFORM_NDK_JSON} | jq '.[] | select(.key=="disable_watchdog_hm")' | jq -r '.intval')
if [ "$disable_watchdog_hm" != "1" ]; then
    echo "Watchdog platform-ndk health-monitoring is enabled" >> $wd_init_log_file
else
    echo "Watchdog platform-ndk health-monitoring is disabled" >> $wd_init_log_file
fi

while [ 1 ] ; do
    #Process health monitor
    if [ "$disable_watchdog_hm" != "1" ]; then
        platform_ndk_health_monitor
        if [[ $app_count -ge $min_app_count_failures ]]; then
            echo "platform process health monitor failed at " `date -u` >> $wd_log_file
            sleep $watchdog_sleep_time
            echo "missed `expr $app_count - $min_app_count_failures` watchdog kick. System will reboot soon." `date -u` >> $wd_log_file
            watchdog_fail=1
            continue
        fi
    fi

    if [[ $watchdog_fail -eq 1 ]]; then
        echo "enable watchdog kick" `date -u` >> $wd_log_file
        watchdog_fail=0
    fi

    #Kick watchdog
    echo "w" >&${desc}
    echo $start_date > $wd_log_file
    echo `date -u`  >> $wd_log_file
    sleep $watchdog_sleep_time
done
