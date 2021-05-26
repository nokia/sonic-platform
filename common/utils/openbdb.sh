#!/bin/bash
# This script load/unload openbdb kernel modules
prog=$(basename $0)

function create_devices()
{
    rm -f /dev/nokia-kernel-bdb
    mknod /dev/nokia-kernel-bdb    c 119 0
}

function load_kernel_modules()
{
    modprobe nokia-kernel-bdb
}

function remove_kernel_modules()
{
    rmmod nokia-kernel-bdb
}

is_cpm=1
if [ -e /host/machine.conf ]; then
    source /host/machine.conf
    if [ "${onie_platform}" != "x86_64-nokia_ixr7250_cpm-r0" ] && \
       [ "${onie_platform}" != "x86_64-nokia_ixr7250e_sup-r0" ] ; then
         is_cpm=0
    fi
fi

case "$1" in
start)

    if [ "$is_cpm" = "1" ]; then
        echo -n "Load OpenBDB kernel modules... "

        depmod -a
        
        create_devices
        load_kernel_modules
    fi
    echo "done."
    ;;

stop)
    echo -n "Unload OpenBDB kernel modules... "

    if [ "$is_cpm" = "1" ]; then
        remove_kernel_modules
    fi

    echo "done."
    ;;

force-reload|restart)
    echo "Not supported"
    ;;

*)
    echo "Usage: ${prog} {start|stop}"
    exit 1
    ;;
esac

exit 0
