"""
    Name: sfp.py, version: 1.0

    Description: Module contains the definitions of SFP related APIs
    for NOKIA 7250 IXR-X3B platform.

    Copyright (c) 2024, Nokia
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

PORT_NUM = 36

REG_DIR = "/sys/bus/pci/devices/0000:05:00.0/"

SYSLOG_IDENTIFIER = "sfp"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class Sfp(SfpOptoeBase):
    """
    Nokia IXR-7220 X3B Platform-specific Sfp refactor class
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
        self.name = sfp_type + '_' + str(index)
        self.port_name = sfp_type + '_' + str(index-1)
        
        self.port_to_eeprom_mapping = {}
        self.port_to_eeprom_mapping[index] = eeprom_path
        self.swpld_path = REG_DIR

        self._version_info = device_info.get_sonic_version_info()
        self.lastPresence = False

        #sonic_logger.log_info(f"Sfp __init__ index {index} setting name to {self.name} "
        #                       "and eeprom_path to {self.eeprom_path}")

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
        sfpstatus = read_sysfs_file(self.swpld_path+f"port_{self.index}_prs")
        
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
            result = read_sysfs_file(self.swpld_path+f"port_{self.index}_rst")
            if result == '0':
                return True
            return False
        return False


    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        status = True
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
        result1 = write_sysfs_file(self.swpld_path+f"port_{self.index}_lpmod", '1')
        result2 = write_sysfs_file(self.swpld_path+f"port_{self.index}_rst", '0')
        time.sleep(0.5)
        result2 = write_sysfs_file(self.swpld_path+f"port_{self.index}_rst", '1')

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
                result = write_sysfs_file(self.swpld_path+f"port_{self.index}_lpmod", '1')
            else:
                result = write_sysfs_file(self.swpld_path+f"port_{self.index}_lpmod", '0')

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
            result = read_sysfs_file(self.swpld_path+f"port_{self.index}_lpmod")

        if result == '1':
            return True

        return False
