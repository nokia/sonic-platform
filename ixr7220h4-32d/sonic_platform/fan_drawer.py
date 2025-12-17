"""
    Nokia IXR7220 H4-32D

    Module contains an implementation of SONiC Platform Base API and
    provides the Fan Drawer status which is available in the platform
"""

try:
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from sonic_platform.eeprom import Eeprom
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

H4_32D_FANS_PER_DRAWER = 2
FAN_PN_F2B = "3HE19865AARA01"
FAN_PN_B2F = "3HE19866AARA01"
GPIO_DIR = "/sys/class/gpio/gpio{}/"
GPIO_PORT = [698, 699, 700, 701, 702, 703, 704]
INDEX_FAN_LED = [0, 4, 8, 12, 16, 20 , 24]
FPGA_FAN_LED = "/sys/kernel/delta_fpga/fan-tray-led"
sonic_logger = logger.Logger('fan_drawer')

class NokiaFanDrawer(FanDrawerBase):
    """
    Nokia platform-specific NokiaFanDrawer class
    """
    def __init__(self, index):
        super().__init__()
        self._index = index + 1
        self.gpio_dir = GPIO_DIR.format(GPIO_PORT[index])
        self.eeprom = Eeprom(False, 0, True, index)
        self.system_led_supported_color = ['off', 'green','amber', 'green_blink',
                                           'amber_blink', 'alter_green_amber', 'off', 'off']

        # Possible fan directions (relative to port-side of device)
        self.fan_direction_intake = "intake"
        self.fan_direction_exhaust = "exhaust"

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
        result = read_sysfs_file(self.gpio_dir + 'value')
        if result == '0':
            return True
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
            if fan.get_status():
                good_fan = good_fan + 1

        if good_fan == H4_32D_FANS_PER_DRAWER:
            return True
        return False

    def get_direction(self):
        """
        Retrieves the direction of the Fan Drawer
        Returns:
            string: direction string
        """
        part_number = self.get_part_number()
        if part_number == FAN_PN_F2B:
            return self.fan_direction_intake
        if part_number == FAN_PN_B2F:
            return self.fan_direction_exhaust
        return 'N/A'

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

        try:
            reg_value = self.system_led_supported_color.index(color)
            result = read_sysfs_file(FPGA_FAN_LED)
            val = int(result, 16)
            value = val & ~(0xF<<INDEX_FAN_LED[self._index-1])
            value =  value | (reg_value<<INDEX_FAN_LED[self._index-1])
            result = write_sysfs_file(FPGA_FAN_LED, str(value))
            if result:
                return True
            return False
        except ValueError:
            return False


    def get_status_led(self):
        """
        Gets the state of the fan drawer LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        if not self.get_presence():
            return self.STATUS_LED_COLOR_OFF
        result = read_sysfs_file(FPGA_FAN_LED)
        val = int(result, 16)
        result = (val & (0x7<<INDEX_FAN_LED[self._index-1])) >> INDEX_FAN_LED[self._index-1]
        if result < len(self.system_led_supported_color):
            return self.system_led_supported_color[result]
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
