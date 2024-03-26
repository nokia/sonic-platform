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
PORT_START = 1
PORT_END = 34
QSFP_PORT_NUM = 32
SFP_PORT_NUM = 2
QSFP_IN_SWPLD = 16

CPUPLD_DIR = "/sys/bus/i2c/devices/0-0031/"
SWPLD1_DIR = "/sys/bus/i2c/devices/17-0031/"
SWPLD2_DIR = "/sys/bus/i2c/devices/17-0034/"
SWPLD3_DIR = "/sys/bus/i2c/devices/17-0035/"

SYSLOG_IDENTIFIER = "sfp_event"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)


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
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv
    
    def initialize(self):
        self.modprs_list = []
        # Get Transceiver status
        time.sleep(5)
        self.modprs_list = self._get_transceiver_status()
        sonic_logger.log_info("Initial SFP presence=%s" % str(self.modprs_list))

    def deinitialize(self):
        if self.handle is None:
            return

    def _get_transceiver_status(self):
        
        port_status = []
        for port in range (PORT_START, PORT_START + PORT_END):
            if port <= QSFP_IN_SWPLD:
                swpld_path = SWPLD2_DIR
            else:
                swpld_path = SWPLD3_DIR              
            if port <= QSFP_PORT_NUM:
                status = self._read_sysfs_file(swpld_path+"qsfp{}_prs".format(port))
            else:
                status = self._read_sysfs_file(swpld_path+"sfp{}_prs".format(port - QSFP_PORT_NUM - 1))            
            
            if status == '0':
                port_status.append(True)
            else:
                port_status.append(False)

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
