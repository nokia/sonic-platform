#!/bin/bash

# err_func() {
#   echo "Error raised! " `date -u` >> /var/log/nokia-watchdog-$suf.log
#   exec 5>&-
#}

# trap 'err_func' ERR

#Function to check for health of NDK platform processes
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

echo "Started at $start_date" > /var/log/nokia-watchdog-init-$suf.log

ls -las /dev/watch* >> /var/log/nokia-watchdog-init-$suf.log
stat /dev/watch* >> /var/log/nokia-watchdog-init-$suf.log

# remove sp5100 driver
lsmod | grep 5100 >> /var/log/nokia-watchdog-init-$suf.log
modprobe -v -r sp5100_tco >> /var/log/nokia-watchdog-init-$suf.log

# ls -las /lib/modules/4.19.0-9-2-amd64/nokia* >> /var/log/nokia-watchdog-init-$suf.log
# find /host -name nokia_gpio_wdt.ko >> /var/log/nokia-watchdog-init-$suf.log
# find /host -name blacklist.conf >> /var/log/nokia-watchdog-init-$suf.log
echo "check for /dev/watchdog at " `date -u` >> /var/log/nokia-watchdog-init-$suf.log

i=0
while [ ! -e /dev/watchdog ] && [ $i -le 10 ] ; do 
   ((i+=1))
   sleep 1
done	

echo "done check $i for /dev/watchdog at " `date -u` >> /var/log/nokia-watchdog-init-$suf.log

ls -las /dev/watch* >> /var/log/nokia-watchdog-init-$suf.log
stat /dev/watch* >> /var/log/nokia-watchdog-init-$suf.log

depmod -A
modprobe -v nokia_gpio_wdt >> /var/log/nokia-watchdog-init-$suf.log
echo "after modprobe" >> /var/log/nokia-watchdog-init-$suf.log
sleep 2

ls -las /dev/watch* >> /var/log/nokia-watchdog-init-$suf.log
stat /dev/watch* >> /var/log/nokia-watchdog-init-$suf.log

# keep it open while we write "w" to it in loop

# exec 5>/dev/watchdog
# echo "w" >&5
exec {desc}>/dev/watchdog
echo "w" >&${desc}

echo "first kick done using desc $desc at " `date -u` >> /var/log/nokia-watchdog-init-$suf.log

# find /host -name nokia_gpio_wdt.ko >> /var/log/nokia-watchdog-init-$suf.log
# find /host -name blacklist.conf >> /var/log/nokia-watchdog-init-$suf.log

lsmod | grep nokia >> /var/log/nokia-watchdog-init-$suf.log
lsmod | grep 5100 >> /var/log/nokia-watchdog-init-$suf.log
dmesg | grep nokia >> /var/log/nokia-watchdog-init-$suf.log
sync
echo "sync done " `date -u` >> /var/log/nokia-watchdog-init-$suf.log
while [ 1 ] ; do
    #Process health monitor
    platform_ndk_health_monitor
    echo "NDK Platform process health monitor check at " `date -u` >> /var/log/nokia-watchdog-$suf.log
    if [[ $app_count -ge $min_app_count_failures ]]; then
        echo "NDK Platform process health monitor failed at " `date -u` >> /var/log/nokia-watchdog-$suf.log
        sleep $watchdog_sleep_time
        continue
    fi

    #Kick watchdog
    echo "w" >&${desc}
    echo $start_date > /var/log/nokia-watchdog-$suf.log
    echo `date -u`  >> /var/log/nokia-watchdog-$suf.log
    sleep $watchdog_sleep_time
done
