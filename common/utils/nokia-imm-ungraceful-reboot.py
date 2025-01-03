#!/usr/bin/env python3

# Name: nokia-imm-ungraceful-reboot.py, version: 1.0
#
# Description: Module contains the definitions to listen to CHASSIS_STATE for IMM reachable/unreachable events 
# and reset the IBGP when the IMM becomes unreachable.
#
# Copyright (c) 2024, Nokia
# All rights reserved.
#

try:
   from sonic_py_common import multi_asic
   from sonic_py_common import daemon_base
   from swsscommon import swsscommon
   from sonic_py_common.logger import Logger
   from platform_ndk import nokia_common
   import ipaddress
   import time,os
except ImportError as e:
    raise ImportError(str(e) + " - required module not found")


SYSLOG_IDENTIFIER = 'nokia-imm-ungraceful-reboot.py'
CHASSIS_MODULE_STATUS_TABLE = 'CHASSIS_MODULE_STATUS_TABLE'
CHASSIS_MODULE_INFO_TABLE = 'CHASSIS_MODULE_TABLE'
SYSTEM_NBR_TABLE = 'SYSTEM_NEIGH'
CHASSIS_MODULE_INFO_HOSTNAME_FIELD = 'hostname'
CHASSIS_MODULE_INFO_SLOT_FIELD = 'slot'
SELECT_TIMEOUT_MSECS = 5000

class BgpPeerReset(object):
    prev_oper_status = {}
    
    logger = Logger(SYSLOG_IDENTIFIER)
    logger.set_min_log_priority_info()

    if multi_asic.is_multi_asic():
       swsscommon.SonicDBConfig.initializeGlobalConfig()

    namespaces = multi_asic.get_front_end_namespaces()

    my_slot = nokia_common._get_my_slot()
    ch_state_db = daemon_base.db_connect("CHASSIS_STATE_DB")
    ch_app_db = daemon_base.db_connect("CHASSIS_APP_DB")

    ch_module_table = swsscommon.Table(ch_state_db, CHASSIS_MODULE_INFO_TABLE)
    system_nbr_table = swsscommon.Table(ch_app_db, SYSTEM_NBR_TABLE)

    sel = swsscommon.Select()
    sst = swsscommon.SubscriberStateTable(ch_state_db, CHASSIS_MODULE_STATUS_TABLE)
    sel.addSelectable(sst)

    while True:
        (state, c) = sel.select(SELECT_TIMEOUT_MSECS)
        if state == swsscommon.Select.TIMEOUT:
            continue
        if state != swsscommon.Select.OBJECT:
            continue

        (key, op, val) = sst.pop()

        fvs = dict(val)
        oper_status = fvs.get('oper_status')
        if oper_status is None:
            logger.log_info('Unable to find oper_status for Module {}'.format(key))
            continue
        if not key in prev_oper_status:
            prev_oper_status[key] = oper_status
        prev_st = prev_oper_status[key]
        prev_oper_status[key] = oper_status
        if op == 'SET':
            if oper_status == 'down' and prev_st == 'up':
                fvs = ch_module_table.get(key)
                if isinstance(fvs, list) and fvs[0] is True:
                    fvs = dict(fvs[-1])
                    if fvs[CHASSIS_MODULE_INFO_SLOT_FIELD] != my_slot:
                        hostname = fvs[CHASSIS_MODULE_INFO_HOSTNAME_FIELD]
                        logger.log_info('CPM notified BDB not present for {}:{}. Resetting i-BGP neighbors'.format(key,hostname))
                        nbr_keys = list(system_nbr_table.getKeys())
                        for nbr_key in nbr_keys:
                            if hostname in nbr_key and  'Ethernet-IB' in nbr_key:
                                key_tokens = nbr_key.split('|')
                                for ns in namespaces:
                                    asic_id = multi_asic.get_asic_index_from_namespace(ns)
                                    device = 'Ethernet-IB' + str(asic_id)
                                    ip_ver = ipaddress.ip_address(key_tokens[3])
                                    if ip_ver.version == 4:
                                        cmd = "sudo ip netns exec {} ip neigh del {} dev {}".format(ns, key_tokens[3],device)
                                    else:
                                        cmd = "sudo ip netns exec {} ip -6 neigh del {} dev {}".format(ns, key_tokens[3],device)
                                    os.system(cmd)
                                    cmd = "vtysh -n {} -c \"clear bgp {}\"".format(asic_id,key_tokens[3])
                                    os.system(cmd)
                                    logger.log_info('Reset i-BGP neighbor {} in {}'.format(key_tokens[3],ns))



if __name__ == "__main__":
   while True:
      #Wait till GRPC is setup
      slot = nokia_common._get_my_slot()
      if slot >= 0:
         break
      time.sleep(1)

   BgpPeerReset()
