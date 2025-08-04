"""
    Nokia IXR7220 H6-64

    Module contains an implementation of SONiC Platform Base API and
    provides the Fan Drawer status which is available in the platform
"""

try:
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from sonic_platform.eeprom import Eeprom
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_py_common import logger
    import os
    import glob
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

FANS_PER_DRAWER = 2
HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/"
I2C_DEV_LIST = ["80-0033", "79-0033"]
EEPROM_ADDR = ['112', '120', '113', '121',
               '114', '122', '115', '123']
sonic_logger = logger.Logger('fan_drawer')

class NokiaFanDrawer(FanDrawerBase):
    """
    Nokia platform-specific NokiaFanDrawer class
    """
    def __init__(self, index):
        super().__init__()
        self._index = index + 1
        self.fan_led_color = ['off', 'green', 'amber', 'green_blink']

        self.fan_direction_intake = "intake"
        i2c_dev = I2C_DEV_LIST[self._index%2]
        hwmon_path = glob.glob(HWMON_DIR.format(i2c_dev))
        self.get_fan_presence_reg = hwmon_path[0] + f"fan{(index//2)+1}_present"
        self.fan_led_reg = hwmon_path[0] + f"fan{(index//2)+1}_led"
        self.eeprom_inited = False
        self.eeprom_dir = f"/sys/bus/i2c/devices/{EEPROM_ADDR[index]}-0050"
        self.new_eeprom_cmd = f"echo 24c64 0x50 > /sys/bus/i2c/devices/i2c-{EEPROM_ADDR[index]}/new_device"
        self.del_eeprom_cmd = f"echo 0x50 > /sys/bus/i2c/devices/i2c-{EEPROM_ADDR[index]}/delete_device"

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
        result = read_sysfs_file(self.get_fan_presence_reg)
        if result == '1': # present
            if not self.eeprom_inited:
                if not os.path.exists(self.eeprom_dir):
                    os.system(self.new_eeprom_cmd)
                self.eeprom = Eeprom(False, 0, True, self._index-1)
                self.eeprom_inited = True
            return True
        
        if os.path.exists(self.eeprom_dir):
            os.system(self.del_eeprom_cmd)
        self.eeprom_inited = False
        return False

    def get_model(self):
        """
        Retrieves the model number of the Fan Drawer
        Returns:
            string: Part number of Fan Drawer
        """
        if not self.get_presence():
            return 'N/A'
        return self.eeprom.modelstr()

    def get_serial(self):
        """
        Retrieves the serial number of the Fan Drawer
        Returns:
            string: Serial number of Fan
        """
        if not self.get_presence():
            return 'N/A'
        return self.eeprom.serial_number_str()

    def get_part_number(self):
        """
        Retrieves the part number of the Fan Drawer

        Returns:
            string: Part number of Fan
        """
        if not self.get_presence():
            return 'N/A'
        return self.eeprom.part_number_str()
    
    def get_service_tag(self):
        """
        Retrieves the servicetag number of the Fan Drawer

        Returns:
            string: servicetag number of Fan
        """
        if not self.get_presence():
            return 'N/A'
        return self.eeprom.service_tag_str()

    def get_manuf_date(self):
        """
        Retrieves the servicetag number of the Fan Drawer

        Returns:
            string: servicetag number of Fan
        """
        if not self.get_presence():
            return 'N/A'
        return self.eeprom.manuf_date_str()
    
    def get_status(self):
        """
        Retrieves the operational status of the Fan Drawer
        Returns:
            bool: True if Fan is operating properly, False if not
        """
        good_fan = 0
        if not self.get_presence():
            return False
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
        return self.fan_direction_intake

    def set_status_led(self, color):
        """
        Sets the state of the fan drawer status LED

        Args:
            color: A string representing the color with which to set the
                   fan drawer status LED

        Returns:
            bool: True if status LED state is set successfully, False if not
        """
        return False

    def get_status_led(self):
        """
        Gets the state of the fan drawer LED
        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings
        """
        if not self.get_presence():
            return 'N/A'

        result = read_sysfs_file(self.fan_led_reg)
        val = int(result)
        if val < len(self.fan_led_color):
            return self.fan_led_color[val]
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
        super(RealDrawer, self).__init__(index)
        self._name = f'drawer{self._index}'

    def get_name(self):
        """
        return module name
        """
        return self._name
