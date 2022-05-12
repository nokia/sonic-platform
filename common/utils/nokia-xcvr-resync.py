#!/usr/bin/env python3

# Name: nokia-xcvr-resync.py, version: 1.0
#
# Description: Module contains the definitions to Initialize the SFPs after syncd initializes the ASIC
#
# Copyright (c) 2022, Nokia
# All rights reserved.
#

import sonic_platform
from sonic_py_common import multi_asic,daemon_base
from sonic_py_common.interface import backplane_prefix, inband_prefix, recirc_prefix
from swsscommon import swsscommon
from platform_ndk import nokia_common
import time
import sys
import sfputil.main as sfputil
from utilities_common import platform_sfputil_helper
import docker
import logging
import subprocess

SELECT_TIMEOUT = 1000
XCVR_RESYNC_OUTPUT_FILE_NAME = '/var/log/nokia-xcvr-resync-output.log'
DOCKER_CLIENT = docker.DockerClient(base_url='unix://var/run/docker.sock')
SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
HWSKU_KEY = 'DEVICE_METADATA.localhost.hwsku'

def get_device_hwsku():
   proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-d', '-v', HWSKU_KEY],
                                    stdout=subprocess.PIPE,
                                    shell=False,
                                    stderr=subprocess.STDOUT,
                                    universal_newlines=True)
   stdout = proc.communicate()[0]
   proc.wait()
   hwsku = stdout.rstrip('\n')
   return hwsku

class asic_xcvr_resync(object):

  def __init__(self):
     self.appl_db = {}
     self.sst = {}
     self.sel = {}
     self.port_init_done = {}
     self.port_dict = []
     self.platform_chassis = None
     self.platform_sfputil = None
     self.curr_syncd_state = {}
     self.prev_syncd_state = {}



     self.platform_chassis = sonic_platform.platform.Platform().get_chassis()
     self.sel = swsscommon.Select()
     self.cfg_sel = swsscommon.Select()
     self.namespaces = multi_asic.get_front_end_namespaces()
     for namespace in self.namespaces:
        self.port_dict.append({})
        asic_id = multi_asic.get_asic_index_from_namespace(namespace)
        self.appl_db[asic_id] = daemon_base.db_connect("APPL_DB", namespace=namespace)
        port_table = swsscommon.SubscriberStateTable(self.appl_db[asic_id], swsscommon.APP_PORT_TABLE_NAME)
        self.sst[asic_id] = port_table
        self.sel.addSelectable(port_table)
        self.prev_syncd_state[asic_id] = False
     return

  def setup_logger(self, name):
    #configure the log messages to go /var/log/syslog and also XCVR_RESYNC_OUTPUT_FILE_NAME
    logging.basicConfig(filename=XCVR_RESYNC_OUTPUT_FILE_NAME,filemode='a',
                        format='%(asctime)s.%(msecs)03d %(name)s %(levelname)-8s %(message)s',
                        level=logging.INFO, datefmt='%Y-%m-%d %H:%M:%S')
    self.logger = logging.getLogger(name)
    screen_handler = logging.StreamHandler(stream=sys.stdout)
    #format for syslog
    formatter = logging.Formatter(fmt='%(name)s : %(message)s')
    screen_handler.setFormatter(formatter)
    self.logger.addHandler(screen_handler)
    return self.logger

  def validate_port(self,port):
    if port.startswith((backplane_prefix(), inband_prefix(), recirc_prefix())):
      return False
    return True

  def sfp_load_helper(self):
    # Load platform-specific sfputil class
    platform_sfputil_helper.load_platform_sfputil()
    # Load port info
    platform_sfputil_helper.platform_sfputil_read_porttab_mappings()
    self.platform_sfputil = platform_sfputil_helper.platform_sfputil
    return

  def is_syncd_running(self, asic_id):
    container_name = 'syncd'+str(asic_id)
    container = DOCKER_CLIENT.containers.get(container_name)
    container_state = container.attrs['State']
    if container_state['Status'] == 'running':
       return True
    else:
       return False

  def handle_syncd_state(self):
     #checks syncd docker state and clears the port_dict
     for namespace in self.namespaces:
        asic_id = multi_asic.get_asic_index_from_namespace(namespace)
        self.curr_syncd_state[asic_id] = sfp_reset.is_syncd_running(asic_id)
        if self.curr_syncd_state[asic_id] == True and self.prev_syncd_state[asic_id] == False:
           self.logger.info('syncd is up for namespace:{}'.format(namespace))

        if self.curr_syncd_state[asic_id] == False and self.prev_syncd_state[asic_id] == True:
           self.logger.info('syncd is down for namespace:{}'.format(namespace))
           del self.port_dict[asic_id]
           self.port_dict.insert(asic_id,{})
           self.port_init_done[asic_id] = False
        self.prev_syncd_state[asic_id] = self.curr_syncd_state[asic_id]
     return


  def port_update_event(self):
    #This def waits for PORT_TABLE events from redis and queues them to port_dict
    (state, selectableObj) = self.sel.select(SELECT_TIMEOUT)
    if state == swsscommon.Select.TIMEOUT:
       # Do not flood log when select times out
       return
    if state != swsscommon.Select.OBJECT:
       self.log_warning("sel.select() did not return swsscommon.Select.OBJECT")
       return
    # Get the redisselect object from selectable object
    redisSelectObj = swsscommon.CastSelectableToRedisSelectObj(selectableObj)

    # Get the corresponding namespace from redisselect db connector object
    ns= redisSelectObj.getDbConnector().getNamespace()
    asic_id = multi_asic.get_asic_index_from_namespace(ns)
    while True:
       (key, op, fvp) = self.sst[asic_id].pop()
       if not key:
         break
       #self.logger.info('key={} op={} fvp={}'.format(key,op,fvp))
       if fvp:
         fvp_dict = dict(fvp)
         if op == "SET" and key == "PortInitDone":
             self.port_init_done[asic_id] = True
             self.logger.info('PortInitDone for asic={}'.format(ns))
             continue
         if not key.startswith('Ethernet'):
            continue

         if self.validate_port(key) == False:
            continue
         if key not in self.port_dict[asic_id]:
            self.port_dict[asic_id][key] = {}
            self.port_dict[asic_id][key]["prev_oper_status"] = 'invalid'
         if op == "SET":
           if "admin_status" not in fvp_dict:
              continue
           if "oper_status" not in fvp_dict:
              continue
           if "admin_status" in self.port_dict[asic_id][key]:
             if self.port_dict[asic_id][key]["admin_status"] != fvp_dict["admin_status"]:
               self.port_dict[asic_id][key]["prev_oper_status"] = 'invalid'
           self.port_dict[asic_id][key]["admin_status"] = fvp_dict['admin_status']
           self.port_dict[asic_id][key]["oper_status"] = fvp_dict['oper_status']
    return

  def sfp_reset_on_admin_up(self):
     #This def traverses the port_dict and Initializes the SFP if the admin_status is up and oper_status is down
     for namespace in self.namespaces:
        asic_id = multi_asic.get_asic_index_from_namespace(namespace)
        port_list = list(self.port_dict[asic_id].keys())
        for port in port_list:
           if self.port_init_done[asic_id] == True:
              if self.port_dict[asic_id][port]["admin_status"] == 'up' and self.port_dict[asic_id][port]["oper_status"] == 'down':
                 if self.port_dict[asic_id][port]["prev_oper_status"] != 'down':
                   physical_port_list = platform_sfputil_helper.logical_port_name_to_physical_port_list(port)
                   if physical_port_list is not None:
                     for physical_port in physical_port_list:
                       self.logger.info('Initializing SFP for Port:{} '.format(port))
                       result = self.platform_chassis.get_sfp(physical_port).reset()
                       self.port_dict[asic_id][port]["prev_oper_status"] = self.port_dict[asic_id][port]["oper_status"]
     return


if __name__ == "__main__":
   while True:
     #Wait till GRPC is setup
     slot = nokia_common._get_my_slot()
     if slot >= 0:
        break
     time.sleep(1)

   if nokia_common.is_cpm() == True:
      print('Asic Xcvr resync is not supported in CPM')
      sys.exit()
   else:
     hwsku = get_device_hwsku()
     if hwsku == 'Nokia-IXR7250E-36x100G':
       print('Asic Xcvr resync is not supported in {}'.format(hwsku))
       sys.exit()

     sfp_reset = asic_xcvr_resync()
     logger = sfp_reset.setup_logger('Nokia-xcvr-resync')
     logger.info('Starting Nokia-sfp-reset for hwsku {}'.format(hwsku))
     sfp_reset.sfp_load_helper()
     while True:
        sfp_reset.handle_syncd_state()
        sfp_reset.port_update_event()
        sfp_reset.sfp_reset_on_admin_up()
