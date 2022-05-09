#!/usr/bin/env python3

# Name: nokia-sfp-reset.py, version: 1.0
#
# Description: Module contains the definitions to reset the sfps after syncd initializes the ASIC
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

SELECT_TIMEOUT = 1000
SFP_RESET_OUTPUT_FILE_NAME = '/var/log/nokia-sfp-reset-output.log'
DOCKER_CLIENT = docker.DockerClient(base_url='unix://var/run/docker.sock')

class asic_sfp_reset(object):

  def __init__(self):
     self.db = {}
     self.state_db = {}
     self.appl_db = {}
     self.state_db_keys = {}
     self.sst = {}
     self.sel = {}
     self.port_admin_state = []
     self.platform_chassis = None
     self.platform_sfputil = None

     self.platform_chassis = sonic_platform.platform.Platform().get_chassis()
     self.sel = swsscommon.Select()
     self.cfg_sel = swsscommon.Select()
     self.namespaces = multi_asic.get_front_end_namespaces()
     for namespace in self.namespaces:
        self.port_admin_state.append({})
        asic_id = multi_asic.get_asic_index_from_namespace(namespace)
        #Subscribing to state_db doesn't send notifications for port down, so using APPL_DB instead
        #self.state_db[asic_id] = daemon_base.db_connect("STATE_DB", namespace=namespace)
        self.appl_db[asic_id] = daemon_base.db_connect("APPL_DB", namespace=namespace)
        #self.sst[asic_id] = swsscommon.SubscriberStateTable(self.state_db[asic_id], swsscommon.STATE_PORT_TABLE_NAME)
        self.sst[asic_id] = swsscommon.SubscriberStateTable(self.appl_db[asic_id], swsscommon.APP_PORT_TABLE_NAME)
        self.sel.addSelectable(self.sst[asic_id])

     return

  def get_asic_namespaces(self):
    return self.namespaces

  def setup_logger(self, name):
    formatter = logging.Formatter(fmt='%(asctime)s.%(msecs)03d %(levelname)-8s %(message)s',
                                  datefmt='%Y-%m-%d %H:%M:%S')
    handler = logging.FileHandler(SFP_RESET_OUTPUT_FILE_NAME, mode='w')
    handler.setFormatter(formatter)
    self.logger = logging.getLogger(name)
    self.logger.setLevel(logging.DEBUG)
    self.logger.addHandler(handler)
    return self.logger

  def validate_port(self,port):
    if port.startswith((backplane_prefix(), inband_prefix(), recirc_prefix())):
      return False
    return True

  def is_syncd_running(self, asic_id):
    container_name = 'syncd'+str(asic_id)
    container = DOCKER_CLIENT.containers.get(container_name)
    container_state = container.attrs['State']
    if container_state['Status'] == 'running':
       return True
    else:
       return False

  def sfp_load_helper(self):
    # Load platform-specific sfputil class
    platform_sfputil_helper.load_platform_sfputil()
    # Load port info
    platform_sfputil_helper.platform_sfputil_read_porttab_mappings()
    self.platform_sfputil = platform_sfputil_helper.platform_sfputil
    return

  def get_port_mapping(self, namespace):
    """Get port mapping from CONFIG_DB
    """
    asic_id = multi_asic.get_asic_index_from_namespace(namespace)
    config_db = daemon_base.db_connect("CONFIG_DB", namespace=namespace)
    port_table = swsscommon.Table(config_db, swsscommon.CFG_PORT_TABLE_NAME)
    for key in port_table.getKeys():
       if not self.validate_port(key):
          continue
       self.port_admin_state[asic_id][key] = 'down'
    return

  def clear_port_mapping(self, asic_id):
      self.port_admin_state[asic_id].clear
      return

  def sfp_reset_on_admin_up(self):
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
    (key, op, fvp) = self.sst[asic_id].pop()
    if fvp:
        fvp_dict = dict(fvp)
        if op == "SET" and "admin_status" in fvp_dict:
           if self.validate_port(key) == True:
             admin_state = fvp_dict["admin_status"]
             prev_admin_state = self.port_admin_state[asic_id][key]
             self.port_admin_state[asic_id][key] = admin_state
             if admin_state == 'up' and prev_admin_state == 'down':
               if self.platform_sfputil is not None:
                 physical_port_list = platform_sfputil_helper.logical_port_name_to_physical_port_list(key)
                 if physical_port_list is not None:
                   for physical_port in physical_port_list:
                      self.logger.info('Resetting sfp for Port:{}'.format(key))
                      result = self.platform_chassis.get_sfp(physical_port).reset()


    return


if __name__ == "__main__":
   while True:
     #Wait till GRPC is setup
     slot = nokia_common._get_my_slot()
     if slot >= 0:
        break
     time.sleep(1)

   if nokia_common.is_cpm() == True:
      print('Asic spf reset is not supported in CPM')
      sys.exit()
   else:
     sfp_reset = asic_sfp_reset()
     logger = sfp_reset.setup_logger('Nokia-Sfp-Reset')
     sfp_reset.sfp_load_helper()
     curr_state = {}
     prev_state = {}
     namespaces = sfp_reset.get_asic_namespaces()
     for namespace in namespaces:
       asic_id = multi_asic.get_asic_index_from_namespace(namespace)
       prev_state[asic_id] = False
     logger.info('Starting while loop')
     while True:
        for namespace in namespaces:
           asic_id = multi_asic.get_asic_index_from_namespace(namespace)
           curr_state[asic_id] = sfp_reset.is_syncd_running(asic_id)
           if curr_state[asic_id] == True and prev_state[asic_id] == False:
              logger.info('syncd is up for namespace:{}'.format(namespace))
              logger.info('Get port map for namespace:{}'.format(namespace))
              sfp_reset.get_port_mapping(namespace)

           if curr_state[asic_id] == False and prev_state[asic_id] == True:
               logger.info('syncd is down for namespace:{}'.format(namespace))
               logger.info('Clear port map for namespace:{}'.format(namespace))
               sfp_reset.clear_port_mapping(asic_id)

           if curr_state[asic_id] == True:
              sfp_reset.sfp_reset_on_admin_up()

           prev_state[asic_id] = curr_state[asic_id]
