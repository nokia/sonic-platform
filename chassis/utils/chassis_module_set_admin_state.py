#!/usr/bin/env python3

# Name: chassis_module_set_admin_state.py, version: 1.0
# Syntax: chassis_module_set_admin_state.py <module_name> <up/down>
#
# Copyright (c) 2024, Nokia
# All rights reserved.
#
import re
import sys
import subprocess
import time

from platform_ndk import nokia_common
from sonic_py_common.logger import Logger

logger=Logger("chassis_module_set_admin_state.py")

sfm_asic_dict = {
    0: [0,1],
    1: [2,3],
    2: [4,5],
    3: [6,7],
    4: [8,9],
    5: [10,11],
    6: [12,13],
    7: [14,15]
}

sfm_hw_slot_mapping = {
    0: 15,
    1: 16,
    2: 17,
    3: 18,
    4: 19,
    5: 20,
    6: 21,
    7: 22
}

def main():
    argc = len(sys.argv)

    if argc != 3:
        logger.log_warning("Missing parameters!")
        return

    module = sys.argv[1]
    state = sys.argv[2]

    if not module.startswith("FABRIC-CARD"):
        logger.log_warning("Failed to set {} state. Admin state can only be set on Fabric module".format(module))
        return

    num = int(re.search(r"(\d+)$", module).group())
    if num not in sfm_hw_slot_mapping:
        logger.log_error("Invalid module name {}".format(module))
        return

    if state == "down":
        logger.log_info("Shutting down chassis module {}".format(module))
        asic_list = sfm_asic_dict[num]
        for asic in asic_list:
            logger.log_info("Stopping swss@{} and syncd@{} ...".format(asic, asic))
            # Process state
            process = subprocess.Popen(['sudo', 'systemctl', 'stop', 'swss@{}.service'.format(asic)],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            outstr = stdout.decode('ascii')
        # wait for service is down
        time.sleep(2)

        logger.log_info("Power off {} module ...".format(module))
        hw_slot = sfm_hw_slot_mapping[num]
        nokia_common._power_onoff_SFM(hw_slot,False)
        logger.log_info("Chassis module {} shutdown completed".format(module))
    else:
        logger.log_info("Starting up chassis module {}".format(module))
        hw_slot = sfm_hw_slot_mapping[num]
        nokia_common._power_onoff_SFM(hw_slot,True)
        # wait SFM HW init done.
        time.sleep(15)

        asic_list = sfm_asic_dict[num]
        for asic in asic_list:
            logger.log_info("Start swss@{} and syncd@{} ...".format(asic, asic))
            # Process state
            process = subprocess.Popen(['sudo', 'systemctl', 'start', 'swss@{}.service'.format(asic)],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            outstr = stdout.decode('ascii')
        logger.log_info("Chassis module {} startup completed".format(module))
    return

if __name__ == "__main__":
    main()