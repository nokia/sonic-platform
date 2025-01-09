#!/bin/bash
# Script call sonic_platform code force reboot a IMM
if [ $# -ne 2 ]; then
    echo "Invalid parameter! Operation aborts."
    echo "Syntax: nokia_cpm_force_reboot_imm_slot.sh  <slot> <key>"
else
    slot=$1
    key=$2
    if [ "$key" == "sonic" ]; then
       echo "Supervisor card force reboot Linecard slot $slot"
       python3 -c "import sonic_platform.platform; platform_chassis = sonic_platform.platform.Platform().get_chassis(); platform_chassis.force_reboot_imm($slot)"
    else
        echo "Invalid key! Operation aborts."
    fi
fi

       
