"""
    Nokia IXR7220 H6-64

    Module contains an implementation of SONiC Platform Base API and
    provides the PSUs' information which are available in the platform
"""

try:
    from sonic_platform.sysfs import read_sysfs_file
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
    import os
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

PSU_NUM = 4
REG_DIR  = "/sys/bus/i2c/devices/73-0061/"
MAX_VOLTAGE = 264
MIN_VOLTAGE = 180
I2C_BUS = [94, 95, 96, 97]
PSU_ADDR = ["58", "59", "5a", "5b"]
EEPROM_ADDR = ['50', '51', '52', '53']

sonic_logger = logger.Logger('psu')
sonic_logger.set_min_log_priority_info()

class Psu(PsuBase):
    """Nokia platform-specific PSU class for 7220 H6-64 """
    def __init__(self, psu_index):
        PsuBase.__init__(self)
        self.psu_led_color = ['off', 'green', 'green_blink', 'amber', 
                                'amber_blink', 'green_blink_fast']
        # PSU is 1-based in Nokia platforms
        self.index = psu_index + 1
        self.psu_dir = f"/sys/bus/i2c/devices/{I2C_BUS[psu_index]}-00{PSU_ADDR[psu_index]}/"
        self.eeprom_dir = f"/sys/bus/i2c/devices/{I2C_BUS[psu_index]}-00{EEPROM_ADDR[psu_index]}/"
        self.new_psu_cmd = f"echo pmbus_psu 0x{PSU_ADDR[psu_index]} > /sys/bus/i2c/devices/i2c-{I2C_BUS[psu_index]}/new_device"
        self.new_eeprom_cmd = f"echo eeprom_fru 0x{EEPROM_ADDR[psu_index]} > /sys/bus/i2c/devices/i2c-{I2C_BUS[psu_index]}/new_device"
        self.del_eeprom_cmd = f"echo 0x{EEPROM_ADDR[psu_index]} > /sys/bus/i2c/devices/i2c-{I2C_BUS[psu_index]}/delete_device"

    def _get_active_psus(self):
        """
        Retrieves the operational status of the PSU and
        calculates number of active PSU's

        Returns:
            Integer: Number of active PSU's
        """
        active_psus = 0
        for i in range(PSU_NUM):
            result = read_sysfs_file(REG_DIR+f"psu{i+1}_ok")
            if result == '1':
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
        result = read_sysfs_file(REG_DIR + f"psu{self.index}_pres")
        if result == '0': # present
            if not os.path.exists(self.psu_dir):
                os.system(self.new_psu_cmd)
            if not os.path.exists(self.eeprom_dir):
                os.system(self.new_eeprom_cmd)
            return True
        # not present
        if os.path.exists(self.eeprom_dir):
            os.system(self.del_eeprom_cmd)
        return False

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        if self.get_presence():
            return read_sysfs_file(self.eeprom_dir+"product_name")

        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        if self.get_presence():
            return read_sysfs_file(self.eeprom_dir+"serial_number")

        return 'N/A'

    def get_revision(self):
        """
        Retrieves the HW revision of the PSU

        Returns:
            string: HW revision of PSU
        """
        if self.get_presence():
            return read_sysfs_file(self.eeprom_dir+"product_version")
        return 'N/A'

    def get_part_number(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        if self.get_presence():
            return read_sysfs_file(self.eeprom_dir+"part_number")

        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        return '1' == read_sysfs_file(REG_DIR+f"psu{self.index}_ok")

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        if self.get_presence():
            result = read_sysfs_file(self.psu_dir+"psu_v_in")
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
            result = read_sysfs_file(self.psu_dir+"psu_i_in")
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
            result = read_sysfs_file(self.psu_dir+"psu_p_in")
            psu_power = (float(result))/1000
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
        return '1' == read_sysfs_file(REG_DIR+f"psu{self.index}_ok")

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if not self.get_presence():
            return 'N/A'

        result = read_sysfs_file(self.psu_dir+"psu_led")
        val = int(result)
        if val < len(self.psu_led_color):
            return self.psu_led_color[val]
        return 'N/A'

    def set_status_led(self, _color):
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
        result = read_sysfs_file(REG_DIR+"led_psu")
        if result == '1':
            return 'green'
        else:
            return 'amber'

    def set_status_master_led(self, _color):
        """
        Sets the state of the front panel PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        return False
    
    def get_mfg_date(self):
        """
        Retrieves the manufacturing date in the PSU EEPROM

        Returns:
            string: mfg_date of PSU
        """
        if self.get_presence() and os.path.exists(self.eeprom_dir+"mfg_date"):
            return read_sysfs_file(self.eeprom_dir+"mfg_date")

        return 'N/A'
    
    def get_fw_rev(self):
        """
        Retrieves the firmware revision of the PSU

        Returns:
            string: firmware revision of PSU
        """
        if self.get_presence():
            return read_sysfs_file(self.psu_dir+"psu_rev")

        return 'N/A'
