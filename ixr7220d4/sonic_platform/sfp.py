"""
     Name: sfp.py, version: 1.0

     Description: Module contains the definitions of SFP related APIs
     for Nokia IXR 7220 D4 platform.

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


QSFP_PORT_NUM = 36
QSFP_DD_START = 28
QSFP_IN_CPLD3 = 18

CPLD1_DIR = "/sys/bus/i2c/devices/10-0060/"
CPLD3_DIR = "/sys/bus/i2c/devices/10-0064/"

SYSLOG_IDENTIFIER = "sfp"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class Sfp(SfpOptoeBase):
    """
    Nokia IXR-7220 D4 Platform-specific Sfp refactor class
    """
    instances = []

    def __init__(self, index, sfp_type, eeprom_path):
        SfpOptoeBase.__init__(self)

        self.index       = index
        self.port_num    = index
        self.sfp_type    = sfp_type
        self.eeprom_path = eeprom_path
        if index <= QSFP_DD_START:
            self.name = sfp_type + '_' + str(index)
            self.port_name = sfp_type + '_' + str(index-1)
        else:
            self.name = sfp_type + '_' + str(index-QSFP_DD_START)
            self.port_name = sfp_type + '_' + str(index-QSFP_DD_START-1)

        sonic_logger.log_info(f"Sfp __init__ index {index} setting name to {self.name} and eeprom_path to {self.eeprom_path}")

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
        sfpstatus = read_sysfs_file(CPLD1_DIR + f"qsfp{self.index}_mod_prsnt")

        return True if sfpstatus == '0' else False

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

        result = read_sysfs_file(CPLD1_DIR+f"qsfp{self.index}_rstn")

        return True if result == '0' else False

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        return not self.get_reset_status()

    def reset(self):
        """
        Reset SFP.
        Returns:
            A boolean, True if successful, False if not
        """
        if not self.get_presence():
            sys.stderr.write("Error: Port {self.index} not inserted, could not reset it.\n\n")
            return False
        sonic_logger.log_info(f"Reseting port #{self.index}.")

        result1 = 'ERR'
        result2 = 'ERR'
        t = 0

        if self.index <= QSFP_PORT_NUM:
            result2 = write_sysfs_file(CPLD1_DIR + f"qsfp{self.index}_reset", '1')
            time.sleep(0.5)
            if self.index <= QSFP_DD_START:
                result2 = write_sysfs_file(CPLD3_DIR + f"qsfp{self.index}_lpmod", '0')
            else:
                result2 = write_sysfs_file(CPLD3_DIR + f"qsfp{self.index}_lpmod", '1')
            time.sleep(0.5)
            result1 = write_sysfs_file(CPLD1_DIR + f"qsfp{self.index}_rstn", '0')
            while t < 10:
                if read_sysfs_file(CPLD1_DIR + f"qsfp{self.index}_reset") == '2':
                    result1 = write_sysfs_file(CPLD1_DIR+f"qsfp{self.index}_rstn", '1')
                    time.sleep(2)
                    break
                time.sleep(0.5)
                if t == 8:
                    sonic_logger.log_info(f"Reset port #{self.index} timeout, reset failed.")
                    return False
                t = t + 1

            result2 = write_sysfs_file(CPLD1_DIR + f"qsfp{self.index}_reset", '3')
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

        result = write_sysfs_file(CPLD3_DIR + f"qsfp{self.index}_lpmod", '1' if lpmode else '0')

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

        result = read_sysfs_file(CPLD3_DIR+f"qsfp{self.index}_lpmod")

        if result == '1':
            return True

        return False
