"""
    NOKIA 7250 IXR-X1B

    Module contains an implementation of SONiC Platform Base API and
    provides the PSUs' information which are available in the platform
"""

try:
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
    import os
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

PSU_NUM = 2
I2C_BUS = [14, 15]
PSU_ADDR = '5b'
PSU_EEPROM_ADDR = '53'
MAX_VOLTAGE = 264
MIN_VOLTAGE = 180
REG_DIR = "/sys/bus/pci/devices/0000:01:00.0/"

sonic_logger = logger.Logger('psu')
sonic_logger.set_min_log_priority_error()

class Psu(PsuBase):
    """Nokia platform-specific PSU class for 7250 X1B """

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # PSU is 1-based in Nokia platforms
        self.index = psu_index + 1
        self._fan_list = []
        self.psu_dir = f"/sys/bus/i2c/devices/{I2C_BUS[psu_index]}-00{PSU_ADDR}/"
        self.eeprom_dir = f"/sys/bus/i2c/devices/{I2C_BUS[psu_index]}-00{PSU_EEPROM_ADDR}/"
        self.new_cmd = f"echo psu_eeprom 0x{PSU_EEPROM_ADDR} > /sys/bus/i2c/devices/i2c-{I2C_BUS[psu_index]}/new_device"
        self.del_cmd = f"echo 0x{PSU_EEPROM_ADDR} > /sys/bus/i2c/devices/i2c-{I2C_BUS[psu_index]}/delete_device"
        self.psu_led_color = ['off', 'green', 'amber']
        self._master_psu_led = 'off'
        self._master_psu_led_set = False

    def _get_active_psus(self):
        """
        Retrieves the operational status of the PSU and
        calculates number of active PSU's

        Returns:
            Integer: Number of active PSU's
        """
        active_psus = 0
        for i in range(PSU_NUM):
            if self.get_status():
                active_psus = active_psus + 1

        return active_psus

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return f"PSU{self.index}"

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        result = read_sysfs_file(self.psu_dir + "psu_status")
        if int(result, 10) < 0:
            if os.path.exists(self.eeprom_dir):
                os.system(self.del_cmd)
            return False
        else:
            if not os.path.exists(self.eeprom_dir):
                os.system(self.new_cmd)
            return True
        
    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """ 
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "part_number")
            return result.strip()

        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "serial_number")
            return result.strip()
        
        return 'N/A'

    def get_revision(self):
        """
        Retrieves the HW revision of the PSU

        Returns:
            string: HW revision of PSU
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "mfg_date")
            return result.strip()
        
        return 'N/A'

    def get_part_number(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "part_number")
            return result.strip()

        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        result = read_sysfs_file(self.psu_dir + "psu_status")        
        if (int(result, 10) & 0x800) >> 11 == 0:
            return True
        
        return False

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        if self.get_presence():
            result = read_sysfs_file(self.psu_dir + "in1_input")
            psu_voltage = (float(result))/1000
        else:
            psu_voltage = 0.0

        return psu_voltage

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        if self.get_presence():
            result = read_sysfs_file(self.psu_dir + "curr1_input")
            psu_current = (float(result))/1000
        else:
            psu_current = 0.0

        return psu_current

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        if self.get_presence():
            result = read_sysfs_file(self.psu_dir + "power1_input")
            psu_power = (float(result))/1000000
        else:
            psu_power = 0.0

        return psu_power

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index

    def get_voltage_high_threshold(self):
        """
        Retrieves the high threshold PSU voltage output

        Returns:
            A float number, the high threshold output voltage in volts,
            e.g. 12.1
        """
        return MAX_VOLTAGE

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return MIN_VOLTAGE

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU
        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        return self.get_status()

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if self.get_presence():
            if self.get_status():
                return self.STATUS_LED_COLOR_GREEN
            else:
                return self.STATUS_LED_COLOR_AMBER
        else:
            return 'N/A'

    def set_status_led(self, color):
        """
        Sets the state of the PSU status LED
        Args:
            color: A string representing the color with which to set the
                   PSU status LED
        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        return False

    def get_status_master_led(self):
        """
        Gets the state of the front panel PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if self._master_psu_led_set:
            return self._master_psu_led
        else:
            return 'N/A'
    
    def set_status_master_led(self, color):
        """
        Sets the state of the front panel PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        if color not in self.psu_led_color:
            return False
        
        color_to_value = {
            'off': '0x0',
            'green': '0x6400',
            'amber': '0xa4c700'
        }
        value = color_to_value.get(color)
        
        status = write_sysfs_file(REG_DIR + 'led_psu', value)
        if status == 'ERR':
            return False
        self._master_psu_led = color
        self._master_psu_led_set = True
        return True
