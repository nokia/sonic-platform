"""
    NOKIA 7250 IXR-X3B

    Module contains an implementation of SONiC Platform Base API and
    provides the Fan Drawer status which is available in the platform
"""

try:
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from sonic_py_common import logger
    import os
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

FANS_PER_DRAWER = 4
I2C_BUS = [11, 12, 13]
FAN_INDEX_IN_DRAWER = [1, 2, 4, 5]
EEPROM_INFO = ['54', 'fan_eeprom']
DRIVER_INFO = ['20', 'max31790']
LED_INFO = ['60', 'fan_led']
REG_DIR = "/sys/bus/pci/devices/0000:01:00.0/"

sonic_logger = logger.Logger('fan_drawer')
sonic_logger.set_min_log_priority_info()

class NokiaFanDrawer(FanDrawerBase):
    def __init__(self, index):
        super().__init__()
        self._index = index + 1
        self.reg_dir = REG_DIR
        self.fan_led_color = ['off', 'green', 'amber', 'green_blink']

        # Possible fan directions (relative to port-side of device)
        self.fan_direction_intake = "intake"
        self.i2c_index = I2C_BUS[index]
        self.eeprom_dir = f"/sys/bus/i2c/devices/{self.i2c_index}-00{EEPROM_INFO[0]}/"
        self.driver_dir = f"/sys/bus/i2c/devices/{self.i2c_index}-00{DRIVER_INFO[0]}/"    
        self.led_dir = f"/sys/bus/i2c/devices/{self.i2c_index}-00{LED_INFO[0]}/"

        self.fan_wd_en = "i2cset -y {} 0x20 0x0 0x26"
        self.new_cmd = "echo {} 0x{} > /sys/bus/i2c/devices/i2c-{}/new_device"
        self.del_cmd = "echo 0x{} > /sys/bus/i2c/devices/i2c-{}/delete_device"
        
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
        result = read_sysfs_file(self.reg_dir + f'fandraw_{self._index}_prs')
        if result == '0': # present
            if not os.path.exists(self.eeprom_dir):
                os.system(self.new_cmd.format(EEPROM_INFO[1], EEPROM_INFO[0], self.i2c_index))
            if not os.path.exists(self.driver_dir):
                os.system(self.fan_wd_en.format(self.i2c_index))
                os.system(self.new_cmd.format(DRIVER_INFO[1], DRIVER_INFO[0], self.i2c_index))
            if not os.path.exists(self.led_dir):
                os.system(self.new_cmd.format(LED_INFO[1], LED_INFO[0], self.i2c_index))
            return True
        # not present
        if os.path.exists(self.eeprom_dir):
            os.system(self.del_cmd.format(EEPROM_INFO[0], self.i2c_index))
        if os.path.exists(self.driver_dir):
            os.system(self.del_cmd.format(DRIVER_INFO[0], self.i2c_index))
        if os.path.exists(self.led_dir):
            os.system(self.del_cmd.format(LED_INFO[0], self.i2c_index))
        return False

    def get_model(self):
        """
        Retrieves the model number of the Fan Drawer
        Returns:
            string: Part number of Fan Drawer
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "part_number")
            return result.strip()
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the Fan Drawer
        Returns:
            string: Serial number of Fan
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "serial_number")
            return result.strip()
        return 'N/A'

    def get_part_number(self):
        """
        Retrieves the part number of the Fan Drawer

        Returns:
            string: Part number of Fan
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "part_number")
            return result.strip()
        return 'N/A'

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
        if not self.get_presence():
            return False
        
        try:
            value = self.fan_led_color.index(color)
            result = write_sysfs_file(self.led_dir + 'fan_led', str(value))
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
            return 'N/A'

        result = read_sysfs_file(self.led_dir + 'fan_led')
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
