#############################################################################
# Nokia
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fan Drawer status which is available in the platform
#
#############################################################################

try:
    import os
    import time
    import struct    
    from mmap import *
    from sonic_platform.eeprom import Eeprom
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FANS_PER_DRAWER = 2
I2C_DIR = "/sys/bus/i2c/devices/i2c-25/25-0033/" 

sonic_logger = logger.Logger('fan_drawer')

class NokiaFanDrawer(FanDrawerBase):
    def __init__(self, index):
        super(NokiaFanDrawer, self).__init__()
        self._index = index + 1
        self.eeprom = Eeprom(False, 0, True, index)
        self.system_led_supported_color = ['off', 'red', 'green']
        self.fan_led_reg = I2C_DIR + "fan{}_led".format(self._index)
        self.fan_presence_reg = I2C_DIR + "fan{}_present".format(self._index)

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
        # On successful write, the value read will be written on
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

    def get_index(self):
        return self._index

    def get_presence(self):
        result = self._read_sysfs_file(self.fan_presence_reg)
        if result == '1':
            return True
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number of the Fan Drawer
        Returns:
            string: Part number of Fan Drawer
        """
        return self.eeprom.modelstr()

    def get_serial(self):
        """
        Retrieves the serial number of the Fan Drawer
        Returns:
            string: Serial number of Fan
        """
        return self.eeprom.serial_number_str()
    
    def get_part_number(self):
        """
        Retrieves the part number of the Fan Drawer

        Returns:
            string: Part number of Fan
        """
        return self.eeprom.part_number_str()

    def get_status(self):
        """
        Retrieves the operational status of the Fan Drawer
        Returns:
            bool: True if Fan is operating properly, False if not
        """
        good_fan = 0
        for fan in self._fan_list:
            if fan.get_status() == True: 
                good_fan = good_fan + 1

        if (good_fan == FANS_PER_DRAWER):                
            return True
        else:
            return False

    def get_direction(self):

        return 'intake'

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED

        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        if not self.get_presence(): 
            return False
        if color not in self.system_led_supported_color:
            return False

        rv = self._write_sysfs_file(self.fan_led_reg, color)
        if (rv != 'ERR'):
            return True
        else:
            return False

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        if not self.get_presence(): 
            return self.STATUS_LED_COLOR_OFF        

        result = self._read_sysfs_file(self.fan_led_reg)

        if result == "base" or result == "off":
            return self.STATUS_LED_COLOR_OFF
        elif result == "green":
            return self.STATUS_LED_COLOR_GREEN
        elif result == "red":
            return self.STATUS_LED_COLOR_RED
        else:
            return 'N/A'

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self._index


# For Nokia platforms with fan drawer(s)
class RealDrawer(NokiaFanDrawer):
    def __init__(self, index):
        super(RealDrawer, self).__init__(index)
        self._name = 'drawer{}'.format(self._index)

    def get_name(self):
        return self._name