"""
    Nokia

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

FANS_PER_DRAWER = 2
FAN_PN_F2B = "3HE20602AA"
FAN_PN_B2F = "3HE20603AA"
GPIO_DIR = "/sys/class/gpio/gpio{}/"
GPIO_PORT = [800, 801, 802, 803, 804, 805, 806]
FPGA_DIR = "/sys/kernel/sys_fpga/"
I2C_BUS = [30, 31, 32, 33, 34, 35, 36]
sonic_logger = logger.Logger('fan_drawer')

class NokiaFanDrawer(FanDrawerBase):
    def __init__(self, index):
        super().__init__()
        self._index = index + 1
        self.gpio_dir = GPIO_DIR.format(GPIO_PORT[index])
        self.fan_led_color = ['off', 'green','amber', 'green_blink',
                            'amber_blink', 'alter_green_amber', 'off', 'off']

        # Possible fan directions (relative to port-side of device)
        self.fan_direction_intake = "intake"
        self.fan_direction_exhaust = "exhaust"
        i2c_bus = I2C_BUS[index]
        self.eeprom_dir = f"/sys/bus/i2c/devices/{i2c_bus}-0050/"
        self.new_cmd = f"echo eeprom_tlv 0x50 > /sys/bus/i2c/devices/i2c-{i2c_bus}/new_device"
        self.del_cmd = f"echo 0x50 > /sys/bus/i2c/devices/i2c-{i2c_bus}/delete_device"

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
        if result == '0': # present
            if not os.path.exists(self.eeprom_dir):
                os.system(self.new_cmd)
            return True
        # not present
        if os.path.exists(self.eeprom_dir):
            os.system(self.del_cmd)
        return False


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
        part_number = self.get_part_number()
        if part_number[:10] == FAN_PN_F2B:
            return self.fan_direction_intake
        if part_number[:10] == FAN_PN_B2F:
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
            value = self.fan_led_color.index(color)
            result = write_sysfs_file(FPGA_DIR + f'fan{self._index}_led', str(value))
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

        result = read_sysfs_file(FPGA_DIR + f'fan{self._index}_led')
        val = int(result, 16) & 0x7

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
