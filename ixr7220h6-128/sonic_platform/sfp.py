"""
    Name: sfp.py, version: 1.0

    Description: Module contains the definitions of SFP related APIs
    for Nokia IXR 7220 H6-128 platform.

    Copyright (c) 2026, Nokia
    All rights reserved.
"""
try:
    import time
    import sys
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from sonic_py_common import logger, device_info
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

PORT_NUM = 128

SYSFS_DIR = "/sys/bus/i2c/devices/{}/"
PORTPLD_ADDR = ["152-0076", "153-0076", "148-0074", "149-0075", "150-0073", "151-0073"]
ADDR_IDX = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
            4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,3,3]
PORT_IDX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
            1,2,5,6,9,10,13,14,17,18,21,22,25,26,29,30,1,2,5,6,9,10,13,14,17,18,21,22,25,26,29,30,
            3,4,7,8,11,12,15,16,19,20,23,24,27,28,31,32,3,4,7,8,11,12,15,16,19,20,23,24,27,28,31,32,
            1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,33,34]

SYSLOG_IDENTIFIER = "sfp"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class Sfp(SfpOptoeBase):
    """
    Nokia IXR-7220 H6-128 Platform-specific Sfp refactor class
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
        if index <= PORT_NUM:
            self.name = sfp_type + '_' + str(index)
            self.port_name = sfp_type + '_' + str(index-1)
        else:
            self.name = sfp_type + '_' + str(index-PORT_NUM)
            self.port_name = sfp_type + '_' + str(index-PORT_NUM-1)
        self.port_to_eeprom_mapping = {}
        self.port_to_eeprom_mapping[index] = eeprom_path
        
        self.pld_path = SYSFS_DIR.format(PORTPLD_ADDR[ADDR_IDX[self.index-1]])
        self.pld_port_idx = PORT_IDX[self.index-1]

        self._version_info = device_info.get_sonic_version_info()

        Sfp.instances.append(self)

    def get_eeprom_path(self):
        """
        Retrieves the eeprom path
        Returns:
            string: eeprom path
        """
        return self.eeprom_path

    def get_presence(self):
        """
        Retrieves the presence
        Returns:
            bool: True if is present, False if not
        """
        sfpstatus = read_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_prs")

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
        if self.index <= PORT_NUM:
            result = read_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_rst")
            if result == '0':
                return True
            return False
        return False

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        status = False
        reset = self.get_reset_status()
        if self.get_presence():
            if not reset:
                status = True

        return status

    def reset(self):
        """
        Reset SFP.
        Returns:
            A boolean, True if successful, False if not
        """
        if not self.get_presence():
            sys.stderr.write(f"Error: Port {self.index} not inserted, could not reset it.\n\n")
            return False
        sonic_logger.log_info(f"Reseting port #{self.index}.")

        result1 = 'ERR'
        result2 = 'ERR'
        result1 = write_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_lpmod", '0')
        result2 = write_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_rst", '0')
        time.sleep(0.5)
        result2 = write_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_rst", '1')

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

        if self.index <= PORT_NUM:
            if lpmode:
                result = write_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_lpmod", '0')
            else:
                result = write_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_lpmod", '1')

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

        if self.index <= PORT_NUM:
            result = read_sysfs_file(self.pld_path+f"port_{self.pld_port_idx}_lpmod")

        if result == '0':
            return True

        return False
