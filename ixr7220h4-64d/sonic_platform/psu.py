########################################################################
# Nokia IXR7220 H4-64D
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################

try:
    import os
    import time
    import struct    
    from mmap import *
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

sonic_logger = logger.Logger('psu')
PSU_NUM = 2
PSU_DIR = ["/sys/bus/i2c/devices/41-0059/",
           "/sys/bus/i2c/devices/33-0058/"]
REG_DIR = "/sys/bus/i2c/devices/6-0060/"
PSU_EEPROM = ["/sys/bus/i2c/devices/41-0051/eeprom",
              "/sys/bus/i2c/devices/33-0050/eeprom"]

class Psu(PsuBase):
    """Nokia platform-specific PSU class for 7220 H4-64D """

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # PSU is 1-based in Nokia platforms
        self.index = psu_index + 1
        self._fan_list = []
        self.psu_dir = PSU_DIR[psu_index]        

        # PSU eeprom
        self.eeprom = self._read_bin_file(PSU_EEPROM[psu_index], 256)

        self.MAX_VOLTAGE = 14
        self.MIN_VOLTAGE = 10

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

        # Ensure that the write operation has succeeded
        if ((self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if ((self._read_sysfs_file(sysfs_file)) != value ):
                rv = 'ERR'

        return rv
    
    def _read_bin_file(self, sysfs_file, len):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'rb') as fd:
                rv = fd.read(len)
                fd.close()
        except Exception as e:
            rv = 'ERR'
        
        return rv
    
    def _get_active_psus(self):
        """
        Retrieves the operational status of the PSU and
        calculates number of active PSU's

        Returns:
            Integer: Number of active PSU's
        """  
        active_psus = 0

        for i in range(PSU_NUM):
            result = self._read_sysfs_file(REG_DIR+"psu{}_pwr_ok".format(i+1))
            if result == '1':
                active_psus = active_psus + 1

        return active_psus

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        result = self._read_sysfs_file(REG_DIR + "psu{}_present".format(self.index))

        if result == '0':
            return True

        return False

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        return self.eeprom[18:30].decode()

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        return self.eeprom[88:99].decode()

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
        return self.eeprom[34:48].decode()

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        result = self._read_sysfs_file(REG_DIR+"psu{}_pwr_ok".format(self.index))
        
        if result == '1':
            return True

        return False

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        if(self.get_presence()):
            result = self._read_sysfs_file(self.psu_dir+"in2_input")
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
        
        if(self.get_presence()):
            result = self._read_sysfs_file(self.psu_dir+"curr2_input")
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
        if(self.get_presence()):
            result = self._read_sysfs_file(self.psu_dir+"power1_input")
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
        return self.MAX_VOLTAGE

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return self.MIN_VOLTAGE
    
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
        result = self._read_sysfs_file(REG_DIR+"psu{}_pwr_ok".format(self.index))
        
        if result == '1':
            return True

        return False

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if self.get_presence():
            if self.get_status():
                return self.STATUS_LED_COLOR_GREEN
            elif self.get_power() == 0.0:
                return self.STATUS_LED_COLOR_OFF
            else:
                self.STATUS_LED_COLOR_RED
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
        if self.get_presence():
            if self.get_status():
                return self.STATUS_LED_COLOR_GREEN
            elif self.get_power() == 0.0:
                return self.STATUS_LED_COLOR_OFF
            else:
                self.STATUS_LED_COLOR_AMBER
        return 'N/A'

    def set_status_master_led(self, color):
        """
        Sets the state of the front panel PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        
        return False
