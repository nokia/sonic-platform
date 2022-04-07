#!/usr/bin/env python3

# Name: asic_thermal.py, version: 1.0
#
# Description: Module contains the definitions to the asic thermal information from STATE_DB and
# populate Nokia platform NDK
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#


from sonic_py_common import multi_asic
from swsscommon import swsscommon
from swsscommon.swsscommon import SonicV2Connector
from natsort import natsorted
from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
from platform_ndk import nokia_cmd
import time
import subprocess
import sys



# The empty namespace refers to linux host namespace.
EMPTY_NAMESPACE = ''
REDIS_TIMEOUT_MSECS = 0
ASIC_TEMP_INFO = "*ASIC_TEMPERATURE_INFO*"
ASIC_SENSORS = "*ASIC_SENSORS*"
ASIC_SENSOR_POLL_INTERVAL = 'ASIC_SENSORS_POLLER_INTERVAL'
ASIC_SENSOR_POLL_STATUS = 'ASIC_SENSORS_POLLER_STATUS'
ASIC_SENSOR_ADMIN_STATE = 'admin_status'
ASIC_SENSOR_INTERVAL = 'interval'
ASIC_TEMP_DEVICE_THRESHOLD = 102
temp_mon_list = ['FAB0', 'FAB1', 'FAB2', 'FAB3', 'NIF0', 'NIF1', 'PRM', 'EMI0', 'EMI1' ]
SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
HWSKU_KEY = 'DEVICE_METADATA.localhost.hwsku'
PLATFORM_KEY = 'DEVICE_METADATA.localhost.platform'



def db_connect(db_name, namespace=EMPTY_NAMESPACE):
   return swsscommon.DBConnector(db_name, REDIS_TIMEOUT_MSECS, True, namespace)

class asic_thermal(object):

  def __init__(self):
     self.db = {}
     self.state_db = {}
     self.config_db = {}
     self.config_db_keys = {}
     self.state_db_keys = {}
     self.poll_interval = {}
     proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-H', '-v', PLATFORM_KEY],
                                    stdout=subprocess.PIPE,
                                    shell=False,
                                    stderr=subprocess.STDOUT,
                                    universal_newlines=True)
     stdout = proc.communicate()[0]
     proc.wait()
     self.platform = stdout.rstrip('\n')

     proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-d', '-v', HWSKU_KEY],
                                    stdout=subprocess.PIPE,
                                    shell=False,
                                    stderr=subprocess.STDOUT,
                                    universal_newlines=True)
     stdout = proc.communicate()[0]
     proc.wait()
     self.hwsku = stdout.rstrip('\n')

     if multi_asic.is_multi_asic():
       # Load the namespace details first from the database_global.json file.
       swsscommon.SonicDBConfig.initializeGlobalConfig()

     sel = swsscommon.Select()
     self.namespaces = multi_asic.get_front_end_namespaces()
     for namespace in self.namespaces:
        asic_id = multi_asic.get_asic_index_from_namespace(namespace)
        self.db[asic_id] = SonicV2Connector(use_unix_socket_path=True, namespace=namespace)
        self.db[asic_id].connect(self.db[asic_id].STATE_DB)
        self.db[asic_id].connect(self.db[asic_id].CONFIG_DB)
        self.poll_interval[asic_id] = 0
     return

  def get_platform_and_hwsku(self):
     return (self.platform, self.hwsku)

  def get_asic_namespaces(self):
    return self.namespaces

  def get_asic_sensor_config(self, asic_id):
    self.config_db_keys[asic_id] = self.db[asic_id].keys(self.db[asic_id].CONFIG_DB, ASIC_SENSORS)
    if not self.config_db_keys[asic_id]:
      return False
    poller_status = 'disable'

    for cfg_key in natsorted(self.config_db_keys[asic_id]):
       key_list = cfg_key.split('|')
       if len(key_list) != 2: # error data in DB, log it and ignore
          print('Warn: Invalid key in table {}: {}'.format(ASIC_SENSORS, cfg_key))
          continue

       name = key_list[1]
       data_dict = self.db[asic_id].get_all(self.db[asic_id].CONFIG_DB, cfg_key)
       if name == ASIC_SENSOR_POLL_STATUS:
         #Check if the ASIC SENSOR polling is enabled in config_db
         poller_status = data_dict[ASIC_SENSOR_ADMIN_STATE]

       if name == ASIC_SENSOR_POLL_INTERVAL:
         #Get ASIC SENSOR polling interval config_db
         self.poll_interval[asic_id] = data_dict[ASIC_SENSOR_INTERVAL]
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

  def update_temperature(self, namespace, asic_temp_data):
    for key, value in asic_temp_data.items():
       temp,index = key.split('_')
       asic_temp = ''
       if temp == 'temperature':
         temp_name = namespace+'_'+index+'--'+temp_mon_list[int(index)]
       elif temp == 'average':
         temp_name = namespace+'_'+ 'average'
       elif temp == 'maximum':
         temp_name = namespace+'_'+ 'maximum'

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
         self.update_temperature(namespace, asic_temp_data)

    return

  def update_asic_temperature(seld):
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
   thermal = asic_thermal()
   platform,hwsku = thermal.get_platform_and_hwsku()
   if platform == 'x86_64-nokia_ixr7250e_sup-r0':
     print('Asic thermal is not supported in CPM')
     sys.exit()
   else:
     thermal.update_asic_temperature()
