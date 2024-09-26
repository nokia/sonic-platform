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

watchdog_fail=1
watchdog_sleep_time=20
function wait_for_sleep_time()
{
    iter=$watchdog_sleep_time
    while [ $iter -gt 0 ]; do
        if [ ! -e /tmp/fsde_dev_hung_sig ]; then
           sleep 1
           if [[ $watchdog_fail -eq 1 ]]; then
              sync
              sync
           fi
        else
           echo "FSDE detected hung device and rebooting at " `date -u` >> $wd_init_log_file
           /usr/sbin/reboot
        fi
        iter=$(( iter - 1 ))
    done
}

start_date=`date -u`
MAX_RETAINED_LOGS=10

suf=`date -u "+%Y%m%d%H%M%S"`

ls -t /var/log/nokia-watchdog-init*.log| cat -n | while read n f; do if [ $n -gt $MAX_RETAINED_LOGS ]; then echo "pruning $n $f"; rm $f; else cp -p $f `printf "/tmp/nokia-watchdog-init%d.log" $n`; rm $f; fi; done
ls -t /var/log/nokia-watchdog-last*.log| cat -n | while read n f; do if [ $n -gt $MAX_RETAINED_LOGS ]; then echo "pruning $n $f"; rm $f; else cp -p $f `printf "/tmp/nokia-watchdog-last%d.log" $n`; rm $f; fi; done
ls -t /var/log/nokia-watchdog*.log| cat -n | while read n f; do if [ $n -gt $MAX_RETAINED_LOGS ]; then echo "pruning $n $f"; rm $f; else cp -p $f `printf "/tmp/nokia-watchdog%d.log" $n`; rm $f; fi; done

cp -p /tmp/nokia-watchdog* /var/log/
rm -f /tmp/nokia-watchdog*

wd_init_log_file=/var/log/nokia-watchdog-init.log
echo "Started at $start_date" > $wd_init_log_file
ls -las /dev/watch* >> $wd_init_log_file
stat /dev/watch* >> $wd_init_log_file

# check for and remove sp5100 driver just in case blacklist somehow not present
lsmod | grep 5100 >> $wd_init_log_file
modprobe -v -r sp5100_tco >> $wd_init_log_file
sleep 1
if [ ! -e /dev/watchdog ]; then
   echo "no /dev/watchdog present" >> $wd_init_log_file	
else
   echo "/dev/watchdog present" >> $wd_init_log_file
fi

echo "done check for /dev/watchdog at " `date -u` >> $wd_init_log_file

ls -las /dev/watch* >> $wd_init_log_file
stat /dev/watch* >> $wd_init_log_file

depmod -A

echo "before modprobe nokia_gpio_wdt at " `date -u` >> $wd_init_log_file
modprobe -v nokia_gpio_wdt >> $wd_init_log_file
while [ "$?" != "0" ]
do
    sleep 1
    modprobe -v nokia_gpio_wdt >> $wd_init_log_file
done
echo "after modprobe nokia_gpio_wdt at " `date -u` >> $wd_init_log_file

sleep 2

ls -las /dev/watch* >> $wd_init_log_file
stat /dev/watch* >> $wd_init_log_file

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

wd_log_file=/var/log/nokia-watchdog.log
wd_temp_file=/tmp/nokia-watchdog.tmp

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
n_kicks=2
while [ 1 ] ; do
    #Process health monitor
    if [[ $n_kicks -le 0 ]]; then
        if [ "$disable_watchdog_hm" != "1" ]; then
            platform_ndk_health_monitor
            if [[ $app_count -gt 0 ]]; then
                echo "platform process health monitor failed with app_count `expr $app_count` at " `date -u` >> $wd_init_log_file
            fi
            if [[ $app_count -ge $min_app_count_failures ]]; then
                echo "missed too many health monitor iters. System will reboot soon." `date -u` >> $wd_init_log_file
                watchdog_fail=1
                wait_for_sleep_time
                continue
            fi
        fi
    else
        echo "Skipping platform_ndk health monitor check for $n_kicks at " `date -u` >> $wd_init_log_file
        n_kicks=$(( n_kicks - 1 ))
    fi

    if [[ $watchdog_fail -eq 1 ]]; then
        echo "enable watchdog kick" `date -u` >> $wd_init_log_file
        watchdog_fail=0
    fi

    #Kick watchdog
    echo "w" >&${desc}
    echo $start_date > $wd_temp_file
    echo `date -u`  >> $wd_temp_file
    cp $wd_temp_file $wd_log_file
    wait_for_sleep_time
    # sleep $watchdog_sleep_time
done
