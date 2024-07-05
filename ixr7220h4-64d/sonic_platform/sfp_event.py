'''
listen for the SFP change event and return to chassis.
'''
import os
import time
from sonic_py_common import logger
from sonic_py_common.general import getstatusoutput_noshell

# system level event/error
EVENT_ON_ALL_SFP = '-1'
SYSTEM_NOT_READY = 'system_not_ready'
SYSTEM_READY = 'system_become_ready'
SYSTEM_FAIL = 'system_fail'

# SFP PORT numbers
QSFP_PORT_NUM = 64
PORT_START = 1
PORT_END = 66

REG_DIR = "/sys/devices/platform/sys_fpga/"

SYSLOG_IDENTIFIER = "sfp_event"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()


class sfp_event:
    ''' Listen to plugin/plugout cable events '''

    def __init__(self):
        self.handle = None

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
                fd.close()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv
    
    def _write_sysfs_file(self, sysfs_file, value):
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(value)
                fd.close()
        except Exception as e:
            rv = 'ERR'        

        return rv
    
    def initialize(self):
        self.modprs_list = []
        # Get Transceiver status
        time.sleep(5)
        self.modprs_list = self._get_transceiver_status()
        sonic_logger.log_info("Initial SFP presence=%s" % str(self.modprs_list))
        if self.modprs_list[PORT_END-2] == True:
            self._write_sysfs_file(REG_DIR+"module_tx_disable_65", '0')
        if self.modprs_list[PORT_END-1] == True:
            self._write_sysfs_file(REG_DIR+"module_tx_disable_66", '0')

    def deinitialize(self):
        if self.handle is None:
            return

    def _get_transceiver_status(self):        
        port_status = []
        reg_value = self._read_sysfs_file(REG_DIR + "module_present_all")
        bin_str = '{:068b}'.format(int(reg_value, 16))
        bin_str = bin_str[68:1:-1]
        bool_list = [not bool(int(bit)) for bit in bin_str]
        port_status.extend(bool_list)
            
        for port in range (PORT_START, PORT_START + PORT_END):                          
            if port <= QSFP_PORT_NUM:                
                if port_status[port-1]:
                    reset_status = self._read_sysfs_file(REG_DIR+"qsfp{}_reset".format(port))                    
                    if reset_status == '1':                        
                        port_status[port-1] = False
                        self._write_sysfs_file(REG_DIR+"qsfp{}_reset".format(port), '2')
                    elif reset_status == '2':                        
                        port_status[port-1] = False
                    elif reset_status == '3':                        
                        port_status[port-1] = True
                        self._write_sysfs_file(REG_DIR+"qsfp{}_reset".format(port), '0')       

        return port_status

    def check_sfp_status(self, port_change, timeout):
        """
        check_sfp_status called from get_change_event, this will return correct
            status of all SFP ports if there is a change in any of them
        """
        start_time = time.time()
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            return False, {}
        end_time = start_time + timeout

        if (start_time > end_time):
            return False, {}  # Time wrap or possibly incorrect timeout

        while (timeout >= 0):
            # Check for OIR events and return updated port_change
            port_status = self._get_transceiver_status()
            if (port_status != self.modprs_list):                
                for i in range(PORT_END):
                    if (port_status[i] != self.modprs_list[i]):
                        # sfp_presence is active low
                        if port_status[i] == True:
                            port_change[i+1] = '1'
                        else:
                            port_change[i+1] = '0'

                # Update reg value
                self.modprs_list = port_status
                return True, port_change

            if forever:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1)  # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, {}
        return False, {}
