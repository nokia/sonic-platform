# Name: sfp.py, version: 1.0
#
# Description: Module contains the definitions of SFP related APIs
# for Nokia IXR 7220 H4-64D platform.
#
# Copyright (c) 2024, Nokia
# All rights reserved.
#

try:
    import os
    import time
    import sys
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common import logger
    from sonic_py_common import device_info
    from sonic_py_common.general import getstatusoutput_noshell

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

import subprocess as cmd

QSFP_PORT_NUM = 64

REG_DIR = "/sys/devices/platform/sys_fpga/"

SYSLOG_IDENTIFIER = "sfp"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class Sfp(SfpOptoeBase):
    """
    Nokia IXR-7220 H4-64D Platform-specific Sfp refactor class
    """
    instances = []

    port_to_i2c_mapping = 0
    
    def __init__(self, index, sfp_type, eeprom_path, port_i2c_map):
        SfpOptoeBase.__init__(self)

        self.index = index
        self.port_num = index
        self.sfp_type = sfp_type

        self.eeprom_path = eeprom_path
        self.port_to_i2c_mapping = port_i2c_map
        if index <= QSFP_PORT_NUM:
            self.name = sfp_type + '_' + str(index)
            self.port_name = sfp_type + '_' + str(index-1)
        else:
            self.name = sfp_type + '_' + str(index-QSFP_PORT_NUM)
            self.port_name = sfp_type + '_' + str(index-QSFP_PORT_NUM-1)
        self.port_to_eeprom_mapping = {}
        self.port_to_eeprom_mapping[index] = eeprom_path        
        
        self._version_info = device_info.get_sonic_version_info()
        self.lastPresence = False

        #sonic_logger.log_info("Sfp __init__ index {} setting name to {} and eeprom_path to {}".format(index, self.name, self.eeprom_path))

        Sfp.instances.append(self)

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
    
    def get_eeprom_path(self):
        return self.eeprom_path

    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        sfpstatus = self._read_sysfs_file(REG_DIR+"module_present_{}".format(self.index))
                    
        if sfpstatus == '0':
            return True

        return False

    def get_name(self):
        """
        Retrieves the name of the device
            Returns:
            string: The name of the device
        """
        return self.name

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device or
                     -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """

        return True

    def _get_error_code(self):
        """
        Get error code of the SFP module

        Returns:
            The error code
        """
        return NotImplementedError

    def get_error_description(self):
        """
        Get error description

        Args:
            error_code: The error code returned by _get_error_code

        Returns:
            The error description
        """
        if not self.get_presence():
            error_description = self.SFP_STATUS_UNPLUGGED
        else:
            error_description = self.SFP_STATUS_OK

        return error_description

    def get_reset_status(self):
        """
        Retrieves the reset status of SFP
        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self.index <= QSFP_PORT_NUM:
            result = self._read_sysfs_file(REG_DIR+"module_reset_{}".format(self.index))
            if result == '0':
                return True
            else:
                return False
        else:
            return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()

        if reset is True:
            status = False
        else:
            status = True

        return status

    def reset(self):
        """
        Reset SFP.
        Returns:
            A boolean, True if successful, False if not
        """
        if not self.get_presence():
            sys.stderr.write("Error: Module not inserted, could not reset it.\n\n")
            return False
        
        result1 = 'ERR'
        result2 = 'ERR'
        t = 0

        if self.index <= QSFP_PORT_NUM:
            result2 = self._write_sysfs_file(REG_DIR+"qsfp{}_reset".format(self.index), '1')
            result2 = self._write_sysfs_file(REG_DIR+"module_lp_mode_{}".format(self.index), '1')
            result1 = self._write_sysfs_file(REG_DIR+"module_reset_{}".format(self.index), '0')            
            time.sleep(0.5)            
            while t < 10:
                if self._read_sysfs_file(REG_DIR+"qsfp{}_reset".format(self.index)) == '2':
                    result1 = self._write_sysfs_file(REG_DIR+"module_reset_{}".format(self.index), '1')
                    time.sleep(2)
                    break
                else:
                    time.sleep(0.5)
                if t == 8:                    
                    return False
                else:
                    t = t + 1
            
            result2 = self._write_sysfs_file(REG_DIR+"qsfp{}_reset".format(self.index), '3')
        else:
            return False
        
        if result1 != 'ERR' and result2 != 'ERR':
            return True

        return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : 
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        result = 'ERR'

        if self.index <= QSFP_PORT_NUM:
            if lpmode:
                result = self._write_sysfs_file(REG_DIR+"module_lp_mode_{}".format(self.index), '1')
            else:
                result = self._write_sysfs_file(REG_DIR+"module_lp_mode_{}".format(self.index), '0')
        
        if result != 'ERR':
            return True

        return False

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP
        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        result = 'ERR'

        if self.index <= QSFP_PORT_NUM:
            result = self._read_sysfs_file(REG_DIR+"module_lp_mode_{}".format(self.index))
        
        if result == '1':
            return True

        return False
