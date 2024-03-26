# Name: sfp.py, version: 1.0
#
# Description: Module contains the definitions of SFP related APIs
# for Nokia IXR 7220 H3 platform.
#
# Copyright (c) 2024, Nokia
# All rights reserved.
#

try:
    import os
    import time
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common.logger import Logger
    from sonic_py_common import device_info
    from sonic_py_common.general import getstatusoutput_noshell

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

import subprocess as cmd

QSFP_PORT_NUM = 32
QSFP_IN_SWPLD = 16

CPUPLD_DIR = "/sys/bus/i2c/devices/0-0031/"
SWPLD1_DIR = "/sys/bus/i2c/devices/17-0031/"
SWPLD2_DIR = "/sys/bus/i2c/devices/17-0034/"
SWPLD3_DIR = "/sys/bus/i2c/devices/17-0035/"

# SFP PORT numbers

logger = Logger()

class Sfp(SfpOptoeBase):
    """
    Nokia IXR-7220 H3 Platform-specific Sfp refactor class
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

        logger.log_debug("Sfp __init__ index {} setting name to {} and eeprom_path to {}".format(index, self.name, self.eeprom_path))

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
        if self.index <= QSFP_IN_SWPLD:
            swpld_path = SWPLD2_DIR
        else:
            swpld_path = SWPLD3_DIR 

        if self.index <= QSFP_PORT_NUM:
            sfpstatus = self._read_sysfs_file(swpld_path+"qsfp{}_prs".format(self.index))
        else:
            sfpstatus = self._read_sysfs_file(swpld_path+"sfp{}_prs".format(self.index - QSFP_PORT_NUM - 1))
            
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
        result1 = 'ERR'
        result2 = 'ERR'
        if self.index <= QSFP_IN_SWPLD:
            swpld_path = SWPLD2_DIR
        else:
            swpld_path = SWPLD3_DIR

        if self.index <= QSFP_PORT_NUM:
            result1 = self._write_sysfs_file(swpld_path+"qsfp{}_rstn".format(self.index), '0')
            time.sleep(1)
            result2 = self._write_sysfs_file(swpld_path+"qsfp{}_rstn".format(self.index), '1')
        
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
        if self.index <= QSFP_IN_SWPLD:
            swpld_path = SWPLD2_DIR
        else:
            swpld_path = SWPLD3_DIR

        if self.index <= QSFP_PORT_NUM:
            if lpmode:
                result = self._write_sysfs_file(swpld_path+"qsfp{}_lpmod".format(self.index), '1')
            else:
                result = self._write_sysfs_file(swpld_path+"qsfp{}_lpmod".format(self.index), '0')
        
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
        if self.index <= QSFP_IN_SWPLD:
            swpld_path = SWPLD2_DIR
        else:
            swpld_path = SWPLD3_DIR

        if self.index <= QSFP_PORT_NUM:
            result = self._read_sysfs_file(swpld_path+"qsfp{}_lpmod".format(self.index))
        
        if result == '1':
            return True

        return False
