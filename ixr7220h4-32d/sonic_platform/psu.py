"""
    Nokia IXR7220 H4-32D

    Module contains an implementation of SONiC Platform Base API and
    provides the PSUs' information which are available in the platform
"""

try:
    import sys
    from sonic_platform.sysfs import read_sysfs_file
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

PSU_DIR = ["/sys/bus/i2c/devices/7-0058/",
           "/sys/bus/i2c/devices/8-0058/"]
FPGA_PSU_PRESENT = ["/sys/kernel/delta_fpga/psu-1-present",
                    "/sys/kernel/delta_fpga/psu-2-present"]
FPGA_PSU_OK = ["/sys/kernel/delta_fpga/psu-1-powergood",
               "/sys/kernel/delta_fpga/psu-2-powergood"]
FPGA_POWER_LED = ["/sys/kernel/delta_fpga/pwr1-led",
                  "/sys/kernel/delta_fpga/pwr2-led"]
MAX_VOLTAGE = 13
MIN_VOLTAGE = 11

sonic_logger = logger.Logger('psu')
sonic_logger.set_min_log_priority_error()

class Psu(PsuBase):
    """Nokia platform-specific PSU class for 7220 H4-32D """

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # PSU is 1-based in Nokia platforms
        self.index = psu_index + 1
        self._fan_list = []
        self.psu_dir = PSU_DIR[psu_index]
        self.system_led_supported_color = ['off', 'green', 'amber',
                                           'green_blink', 'amber_blink', 'alter_green_amber','off']

    def _get_active_psus(self):
        """
        Retrieves the operational status of the PSU and
        calculates number of active PSU's

        Returns:
            Integer: Number of active PSU's
        """
        active_psus = 0
        for psu_path in FPGA_PSU_OK:
            result = read_sysfs_file(psu_path)
            if result == '0x1':
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

        result = read_sysfs_file(FPGA_PSU_PRESENT[self.index-1])
        if result == '0x0':
            return True

        return False

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        if self.get_presence():
            psu_sysfs_str = self.psu_dir + "psu_mfr_model"
            result = read_sysfs_file(psu_sysfs_str)
            return result
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        if self.get_presence():
            psu_sysfs_str = self.psu_dir + "psu_mfr_serial"
            result = read_sysfs_file(psu_sysfs_str)
            return result
        return 'N/A'


    def get_revision(self):
        """
        Retrieves the HW revision of the PSU

        Returns:
            string: HW revision of PSU
        """
        return 'N/A'

    def get_part_number(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        if self.get_presence():
            psu_sysfs_str = self.psu_dir + "psu_mfr_model"
            result = read_sysfs_file(psu_sysfs_str)
            return result
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        result = read_sysfs_file(FPGA_PSU_OK[self.index-1])
        if result == '0x0':            
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
            result = read_sysfs_file(self.psu_dir+"in2_input")
            if result is None:
                psu_voltage = 0.0
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
            result = read_sysfs_file(self.psu_dir+"curr2_input")
            if result is None:
                psu_current = 0.0
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
            result = read_sysfs_file(self.psu_dir+"power1_input")
            if result is None:
                psu_power = 0.0
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
        result = read_sysfs_file(FPGA_POWER_LED[self.index - 1])
        if result is None:
            return 'N/A'
        result = int(result, 16)

        if result < len(self.system_led_supported_color):
            return self.system_led_supported_color[result]
        if result == len(self.system_led_supported_color):
            if self.get_presence():
                if self.get_status():
                    return self.STATUS_LED_COLOR_GREEN
                return self.STATUS_LED_COLOR_AMBER
            return self.STATUS_LED_COLOR_OFF
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
        return self.get_status_led()

    def set_status_master_led(self, _color):
        """
        Sets the state of the front panel PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """

        return False
