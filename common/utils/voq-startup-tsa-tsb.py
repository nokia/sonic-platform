#!/usr/bin/env python3

# Name: voq-startup-tsa-tsb.py, version: 1.0
#
# Description: Module contains the definitions to the VOQ Startup TSA-TSB service

#
# Copyright (c) 2023, Nokia
# All rights reserved.
#

from sonic_py_common import multi_asic
import subprocess
import sys, getopt
from threading import Timer


class voq_startup_tsa_tsb(object):

  def __init__(self):
    self.tsa_enabled = {}
    return

  def getPlatform(self):
     platform_key = "DEVICE_METADATA['localhost']['platform']"
     platform = (subprocess.check_output(['sonic-cfggen', '-d', '-v', platform_key.replace('"',"'")]).strip()).decode()
     return platform

  def getNumAsics(self):
     platform = self.getPlatform()
     asic_config_file = '/usr/share/sonic/device/{}/asic.conf'.format(platform)
     file = open(asic_config_file, 'r')
     Lines = file.readlines()
     for line in Lines:
        field = line.split('=')[0].strip()
        if field == "NUM_ASIC":
            return line.split('=')[1].strip()
     return 0


  def getSonicConfig(self, ns, config_name):
     return subprocess.check_output(['sonic-cfggen', '-d', '-v', config_name.replace('"', "'"), '-n', ns.replace('"', "'")]).strip()

  def getSubRole(self, asic_ns):
     sub_role_config = "DEVICE_METADATA['localhost']['sub_role']"
     sub_role = (self.getSonicConfig(asic_ns, sub_role_config)).decode()
     return sub_role

  def getTsaConfig(self, asic_ns):
     tsa_config = 'BGP_DEVICE_GLOBAL.STATE.tsa_enabled'
     tsa_enabled = (self.getSonicConfig(asic_ns, tsa_config)).decode()
     print('{}: {} - CONFIG_DB.{} : {}'.format(__file__, asic_ns, tsa_config, tsa_enabled))
     return tsa_enabled

  def get_tsa_status(self):
     asic_num = self.getNumAsics()
     counter = 0
     for asic_id in range(int(asic_num)):
        asic_ns = 'asic{}'.format(asic_id)
        sub_role = self.getSubRole(asic_ns)
        if sub_role == 'FrontEnd':
          self.tsa_enabled[asic_id] = self.getTsaConfig(asic_ns) 
          if self.tsa_enabled[asic_id] == 'false':
             counter += 1
        else:
          self.tsa_enabled[asic_id] = 'false' 
     if counter == int(asic_num):
       return True;
     else:
       return False;

  def config_tsa(self):
     tsa_enable = self.get_tsa_status()
     if tsa_enable == True:
        print("{}: Configuring TSA".format(__file__))
        subprocess.check_output(['sudo', 'TSA']).strip()
     else:
        print("{}: Either TSA is already configured or switch sub_role is not Frontend - not configuring TSA".format(__file__))
     return tsa_enable

  def config_tsb(self):
     print("voq-startup-tsa-tsb: Configuring TSB")
     return subprocess.check_output(['sudo', 'TSB']).strip()

  def start_tsb_timer(self, interval):
     print("{}: Starting timer with interval {} seconds to configure TSB".format(__file__, interval))
     t = Timer(int(interval), self.config_tsb)
     t.start()
     return


if __name__ == "__main__":
    # parse command line options:
    if len(sys.argv) <= 1:
       print (" voq-startup-tsa-tsb.py -tsb_timer  <timer-val-secs>")
       sys.exit()

    try:
       opts, args = getopt.getopt(sys.argv[1:], 'ht:', ['help', 'tsb_timer'], )
    except getopt.GetoptError:
       print (" voq-startup-tsa-tsb.py -tsb_timer  <timer-val-secs>")
       sys.exit()

    for opt, arg in opts:
       if opt in ("-t", "--tsb_timer"):
          tsb_timer = arg
          print('{}: TSB timer:{} seconds'.format(__file__, tsb_timer))

    voq_monitor = voq_startup_tsa_tsb()
    platform = voq_monitor.getPlatform()
    if platform == 'x86_64-nokia_ixr7250e_sup-r0' or platform == 'x86_64-nokia_ixr7250_cpm-r0':
      print("{}: Not configuring TSA in Supervisor card".format(__file__))
      sys.exit()

    #Configure TSA if it was not configured already in CONFIG_DB
    tsa_enabled = voq_monitor.config_tsa()
    if tsa_enabled == True:
      #Start the timer to configure TSB
      voq_monitor.start_tsb_timer(tsb_timer)

