#!/usr/bin/env python3

# Name: asic_thermal.py, version: 1.0
#
# Description: Module contains the definitions to the asic thermal information from STATE_DB and
# populate Nokia platform NDK
#
# Copyright (c) 2022, Nokia
# All rights reserved.
#


from sonic_py_common import multi_asic
from swsscommon import swsscommon
from swsscommon.swsscommon import SonicV2Connector,ConfigDBConnector
from natsort import natsorted
from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
from platform_ndk import nokia_cmd
import time
import sys



# The empty namespace refers to linux host namespace.
ASIC_SENSOR_DEFAULT_POLL_INTERVAL = 60 # sonic uses 60s as default polling interval
ASIC_TEMP_INFO = "*ASIC_TEMPERATURE_INFO*"
ASIC_SENSOR_POLL_INTERVAL = 'ASIC_SENSORS_POLLER_INTERVAL'
ASIC_SENSOR_POLL_STATUS = 'ASIC_SENSORS_POLLER_STATUS'
ASIC_SENSOR_ADMIN_STATE = 'admin_status'
ASIC_SENSOR_INTERVAL = 'interval'
ASIC_TEMP_DEVICE_THRESHOLD = 102
temp_mon_list = ['FAB0', 'FAB1', 'FAB2', 'FAB3', 'NIF0', 'NIF1', 'PRM', 'EMI0', 'EMI1', 'DRAM0', 'DRAM1' ]


class asic_thermal(object):

  def __init__(self):
     self.db = {}
     self.state_db = {}
     self.config_db = {}
     self.config_db_keys = {}
     self.state_db_keys = {}
     self.poll_interval = {}

     if multi_asic.is_multi_asic():
       # Load the namespace details first from the database_global.json file.
       swsscommon.SonicDBConfig.initializeGlobalConfig()

     sel = swsscommon.Select()
     self.namespaces = multi_asic.get_front_end_namespaces()
     for namespace in self.namespaces:
        asic_id = multi_asic.get_asic_index_from_namespace(namespace)
        self.db[asic_id] = SonicV2Connector(use_unix_socket_path=True, namespace=namespace)
        self.db[asic_id].connect(self.db[asic_id].STATE_DB)
        self.config_db[asic_id] = ConfigDBConnector(use_unix_socket_path=True, namespace=namespace)
        self.config_db[asic_id].connect()

        self.poll_interval[asic_id] = ASIC_SENSOR_DEFAULT_POLL_INTERVAL
     return

  def get_platform_and_hwsku(self):
     return (self.platform, self.hwsku)

  def get_asic_namespaces(self):
    return self.namespaces

  def get_asic_sensor_config(self, asic_id):
    if not self.db[asic_id].CONFIG_DB:
      return False

    asic_sensor = self.config_db[asic_id].get_entry('ASIC_SENSORS', 'ASIC_SENSORS_POLLER_STATUS')
    if not asic_sensor:
      return False
    poller_status = asic_sensor[ASIC_SENSOR_ADMIN_STATE]
    asic_interval = self.config_db[asic_id].get_entry('ASIC_SENSORS', 'ASIC_SENSORS_POLLER_INTERVAL')
    if not asic_interval:
      self.poll_interval[asic_id] = ASIC_SENSOR_DEFAULT_POLL_INTERVAL
    else:
      self.poll_interval[asic_id] = asic_interval[ASIC_SENSOR_INTERVAL]
    if poller_status == 'enable':
      return True
    else:
      return False

  def get_asic_poll_interval(self, asic_id):
     return self.poll_interval[asic_id]

  def print_temperature(self, asic_temp_data):
    field = []
    field.append('Name    ')
    field.append('Monitor              ')
    field.append('Temperature')
    item_list = []
    for key, value in asic_temp_data.items():
       item = []
       temp,index = key.split('_')
       if temp == 'temperature':
         item.append(temp_mon_list[int(index)])
       else:
         item.append(temp)
       item.append(key)
       item.append(value)
       item_list.append(item)

    print('   ASIC TEMPERATURE INFO')
    nokia_cmd.print_table(field, item_list)
    return

  def update_temperature(self, asic_id, asic_temp_data):
    for key, value in asic_temp_data.items():
       if (int(value)) == 0:
         return
       temp,index = key.split('_')
       asic_temp = ''
       if temp == 'temperature':
         temp_name = 'ASIC' + str(asic_id) +'_' + index + '--' + temp_mon_list[int(index)]
         nokia_cmd.set_asic_temp(temp_name, int(value), ASIC_TEMP_DEVICE_THRESHOLD)
    return

  def get_db_asic_temp(self, namespace, asic_id):
    is_enabled = self.get_asic_sensor_config(asic_id)
    if is_enabled == True:
      self.state_db_keys[asic_id] = self.db[asic_id].keys(self.db[asic_id].STATE_DB, ASIC_TEMP_INFO)
      if not self.state_db_keys[asic_id]:
        return
      #Get the ASIC Temperature from state_db
      for state_key in natsorted(self.state_db_keys[asic_id]):
         asic_temp_data = self.db[asic_id].get_all(self.db[asic_id].STATE_DB, state_key)
         # Update NDK
         self.update_temperature(asic_id, asic_temp_data)

    return

  def update_asic_temperature(self):
    namespaces = thermal.get_asic_namespaces()
    while True:
      timer_interval  = 0
      for namespace in namespaces:
        asic_id = multi_asic.get_asic_index_from_namespace(namespace)
        thermal.get_db_asic_temp(namespace, asic_id)
        interval = thermal.get_asic_poll_interval(asic_id)
        if interval != 0 and  timer_interval == 0:
           timer_interval = int(interval)
        if interval != 0 and  timer_interval > int(interval):
           timer_interval = int(interval)
      #sleep for lowest polling interval of asics
      time.sleep(timer_interval)
    return

if __name__ == "__main__":
   while True:
     #Wait till GRPC is setup
     slot = nokia_common._get_my_slot()
     if slot >= 0:
        break
     time.sleep(1)

   if nokia_common.is_cpm() == True:
      print('Asic thermal is not supported in CPM')
      sys.exit()
   else:
     thermal = asic_thermal()
     thermal.update_asic_temperature()
