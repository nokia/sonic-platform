"""
Module contains an implementation of SONiC Platform Base API and
provides access to hardware watchdog
"""

import os
import sys
from sonic_platform_base.watchdog_base import WatchdogBase

""" watchdog sysfs """
WD_SYSFS_PATH = "/sys/bus/i2c/devices/0-0031/"

WD_COMMON_ERROR = -1

class WatchdogImplBase(WatchdogBase):
    """
    Base class that implements common logic for interacting
    with watchdog using ioctl commands
    """

    def __init__(self):
        """
        Open a watchdog handle
        """
        super(WatchdogImplBase, self).__init__()
        
        #self.watchdog=""
        self.wd_timeout_reg = WD_SYSFS_PATH+"wd_timer"    
        self.timeout = self._gettimeout()

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

    def _disablewatchdog(self):
        """
        Turn off the watchdog timer
        """ 
               
        self._write_sysfs_file(WD_SYSFS_PATH+"wd_enable", '0')

    def _enablewatchdog(self):
        """
        Turn on the watchdog timer
        """

        self._write_sysfs_file(WD_SYSFS_PATH+"wd_enable", '1')

    def _keepalive(self):
        """
        Keep alive watchdog timer
        """

        self._write_sysfs_file(WD_SYSFS_PATH+"wd_punch", '0')

    def _settimeout(self, seconds):
        """
        Set watchdog timer timeout
        @param seconds - timeout in seconds
        @return is the actual set timeout
        """        
        
        if seconds in range(0, 16):
            timeout = 15
            reg_value = '0'
        elif seconds in range(16, 21):
            timeout = 20
            reg_value = '1'
        elif seconds in range(21, 31):
            timeout = 30
            reg_value = '2'
        elif seconds in range(31, 41):
            timeout = 40
            reg_value = '3'
        elif seconds in range(41, 51):
            timeout = 50
            reg_value = '4'
        elif seconds in range(51, 61):
            timeout = 60
            reg_value = '5'
        elif seconds in range(61, 66):
            timeout = 65
            reg_value = '6'
        elif seconds in range(67, 71):
            timeout = 70
            reg_value = '7'
        else:
            timeout = 30
            reg_value = '2'
        
        self._write_sysfs_file(self.wd_timeout_reg, reg_value)

        return timeout

    def _gettimeout(self):
        """
        Get watchdog timeout
        @return watchdog timeout
        """
        timeout=0
        reg_value = self._read_sysfs_file(self.wd_timeout_reg)
        timeout = int(reg_value[5:7])

        return timeout

    def _gettimeleft(self):
        """
        Get time left before watchdog timer expires
        @return time left in seconds
        """

        # H3 platform do not support this feature
        return WD_COMMON_ERROR
        
    def arm(self, seconds):
        """
        Arm the hardware watchdog
        """

        ret = WD_COMMON_ERROR
        if seconds < 0 or seconds > 70:
            sys.stderr.write("Error: Wrong watchdog timer choice: {} \n".format(seconds))
            sys.stderr.write("H3 platform only support watchdog timer in [15, 20, 30, 40, 50, 60, 65, 70] seconds\n\n")
            return ret        
        
        try:
            if self.timeout != seconds:
                self.timeout = self._settimeout(seconds)
            if self.is_armed():
                self._keepalive()
            else:
                self._enablewatchdog()
            ret = self.timeout
        except IOError:
            pass

        return ret

    def disarm(self):
        """
        Disarm the hardware watchdog

        Returns:
            A boolean, True if watchdog is disarmed successfully, False
            if not
        """
        
        if self.is_armed():            
            try:
                self._disablewatchdog()
                self.timeout = 0
            except IOError:
                return False

        return True

    def is_armed(self):
        """
        Implements is_armed WatchdogBase API
        """
        status = False

        state = self._read_sysfs_file(WD_SYSFS_PATH+"wd_enable")
        if state == '1':
            status = True

        return status

    def get_remaining_time(self):
        """
        Implements get_remaining_time WatchdogBase API
        """
        # H3 platform do not support this feature
        return WD_COMMON_ERROR