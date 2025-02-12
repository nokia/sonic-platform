"""
    Nokia IXR7220 D4

    Module contains an implementation of SONiC Platform Base API and
    provides the Fan Drawer status which is available in the platform
"""

try:
    from sonic_platform.sysfs import write_sysfs_file
    from sonic_platform.eeprom import Eeprom
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

FANS_PER_DRAWER = 2
I2C_DIR = "/sys/bus/i2c/devices/i2c-14/14-0066/"
sonic_logger = logger.Logger(FanDrawerBase.DEVICE_TYPE)

class NokiaFanDrawer(FanDrawerBase):
    """
    Nokia platform-specific NokiaFanDrawer class
    """
    def __init__(self, index):
        super(NokiaFanDrawer, self).__init__()
        self._index = index + 1
        self.fan_led_color = [self.STATUS_LED_COLOR_OFF,
                              self.STATUS_LED_COLOR_RED,
                              self.STATUS_LED_COLOR_GREEN]
        self.fan_led_reg = I2C_DIR + f"fan{self._index}_led"

    def get_index(self):
        """
        Retrieves the index of the Fan Drawer
        Returns:
            int: the Fan Drawer's index
        """
        return self._index

    def get_presence(self):
        """
        Retrieves the presence of the Fan Drawer
        Returns:
            bool: return True if the Fan Drawer is present
        """
        return self._fan_list[0].get_presence()

    def get_model(self):
        """
        Retrieves the model number of the Fan Drawer
        Returns:
            string: Part number of Fan Drawer
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the Fan Drawer
        Returns:
            string: Serial number of Fan
        """
        return 'N/A'

    def get_part_number(self):
        """
        Retrieves the part number of the Fan Drawer

        Returns:
            string: Part number of Fan
        """
        return self._fan_list[0].get_part_number()

    def get_status(self):
        """
        Retrieves the operational status of the Fan Drawer
        Returns:
            bool: True if Fan is operating properly, False if not
        """
        good_fan = 0
        for fan in self._fan_list:
            if fan.get_status():
                good_fan = good_fan + 1

        if good_fan == FANS_PER_DRAWER:
            return True
        return False

    def get_direction(self):
        """
        Retrieves the direction of the Fan Drawer
        Returns:
            string: direction string
        """
        return self._fan_list[0].get_direction()

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
        if color not in self.fan_led_color:
            return False

        rv = write_sysfs_file(self.fan_led_reg, color)
        if rv != 'ERR':
            return True
        return False

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        return self._fan_list[0].get_status_led()

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

class RealDrawer(NokiaFanDrawer):
    """
    For Nokia platforms with fan drawer(s)
    """
    def __init__(self, index):
        super().__init__(index)
        self._name = f'drawer{self._index}'

    def get_name(self):
        """
        return module name
        """
        return self._name
