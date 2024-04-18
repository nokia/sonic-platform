########################################################################
# Nokia IXR7220 H5-64D
#
# Module contains an implementation of SONiC Platform Base API and
# provides the PSUs' information which are available in the platform
#
########################################################################

try:
    import os
    import time
    import struct
    from os import *
    from mmap import *
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

sonic_logger = logger.Logger('psu')
H5_64D_PSU = 2
PSU_DIR = ["/sys/bus/i2c/devices/2-0058/",
           "/sys/bus/i2c/devices/3-0058/"]
RESOURCE = "/sys/bus/pci/devices/0000:02:00.0/resource0"
REG_BRD_CTRL4 = 0x0020
REG_FRONT_LED = [0x008C, 0x0090]
INDEX_PSU_PRES = [16, 20]
INDEX_PSU_OK = [17, 21]

class Psu(PsuBase):
    """Nokia platform-specific PSU class for 7220 H4-32D """

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # PSU is 1-based in Nokia platforms
        self.index = psu_index + 1
        self._fan_list = []
        self.psu_dir = PSU_DIR[psu_index]
        

        # PSU eeprom
        #self.eeprom = Eeprom(is_psu=True, psu_index=self.index)
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
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if ((self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if ((self._read_sysfs_file(sysfs_file)) != value ):
                rv = 'ERR'

        return rv
    
    def pci_set_value(resource, data, offset):
        fd = open(resource, O_RDWR)
        mm = mmap(fd, 0)
        mm.seek(offset)
        mm.write(struct.pack('I', data))
        mm.close()
        close(fd)

    def pci_get_value(resource, offset):
        fd = open(resource, O_RDWR)
        mm = mmap(fd, 0)
        mm.seek(offset)
        read_data_stream = mm.read(4)
        reg_val = struct.unpack('I', read_data_stream)
        mm.close()
        close(fd)
        return reg_val

    def _get_active_psus(self):
        """
        Retrieves the operational status of the PSU and
        calculates number of active PSU's

        Returns:
            Integer: Number of active PSU's
        """  
        active_psus = 0

        val = self.pci_get_value(RESOURCE, REG_BRD_CTRL4)
        for i in range(H5_64D_PSU):             
            psu_result = (val[0] & (1<<INDEX_PSU_OK[i])) >> INDEX_PSU_OK[i] 
            if psu_result == '0':
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
        val = self.pci_get_value(RESOURCE, REG_BRD_CTRL4) 
        result = (val[0] & (1<<INDEX_PSU_PRES[self.index-1])) >> INDEX_PSU_PRES[self.index-1]        

        if result == '0':
            return True

        return False

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        if (self.get_presence()):
            psu_sysfs_str = self.psu_dir + "psu_mfr_model"
            result = self._read_sysfs_file(psu_sysfs_str)
            return result
        else:
            return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        if (self.get_presence()):
            psu_sysfs_str = self.psu_dir + "psu_mfr_serial"
            result = self._read_sysfs_file(psu_sysfs_str)
            return result
        else:
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
        if (self.get_presence()):
            psu_sysfs_str = self.psu_dir + "psu_mfr_model"
            result = self._read_sysfs_file(psu_sysfs_str)
            return result
        else:
            return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        val = self.pci_get_value(RESOURCE, REG_BRD_CTRL4) 
        result = (val[0] & (1<<INDEX_PSU_OK[self.index-1])) >> INDEX_PSU_OK[self.index-1] 

        if result == '0':
            return True

        return False

    def get_voltage(self):
        """
        Retrieves current PSU voltage output

        Returns:
            A float number, the output voltage in volts,
            e.g. 12.1
        """
        if(self.get_status()):
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
        
        if(self.get_status()):
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
        # psu_voltage = self.get_voltage()
        # psu_current = self.get_current()
        # psu_power = psu_voltage * psu_current
        if(self.get_status()):
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
        val = self.pci_get_value(RESOURCE, REG_BRD_CTRL4) 
        result = (val[0] & (1<<INDEX_PSU_OK[self.index-1])) >> INDEX_PSU_OK[self.index-1] 

        if result == '0':
            return True

        return False

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        val = self.pci_get_value(RESOURCE, REG_FRONT_LED[self.index-1]) 
        result = val[0] & 0x7
     
        if result == 0 or result == 6:
            return self.STATUS_LED_COLOR_OFF
        elif result == 1:
            return self.STATUS_LED_COLOR_GREEN
        elif result == 2:
            return self.STATUS_LED_COLOR_AMBER
        elif result == 3:
            return self.STATUS_LED_COLOR_GREEN_BLINK
        elif result == 4:
            return self.STATUS_LED_COLOR_AMBER_BLINK
        elif result == 7:
            if self.get_presence():
                if self.get_status():
                    return self.STATUS_LED_COLOR_GREEN
                else:
                    return self.STATUS_LED_COLOR_AMBER
            else:
                return self.STATUS_LED_COLOR_OFF
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
        val = self.pci_get_value(RESOURCE, REG_FRONT_LED[self.index-1]) 
        result = val[0] & 0x7
     
        if result == 0 or result == 6:
            return self.STATUS_LED_COLOR_OFF
        elif result == 1:
            return self.STATUS_LED_COLOR_GREEN
        elif result == 2:
            return self.STATUS_LED_COLOR_AMBER
        elif result == 3:
            return self.STATUS_LED_COLOR_GREEN_BLINK
        elif result == 4:
            return self.STATUS_LED_COLOR_AMBER_BLINK
        elif result == 7:
            if self.get_presence():
                if self.get_status():
                    return self.STATUS_LED_COLOR_GREEN
                else:
                    return self.STATUS_LED_COLOR_AMBER
            else:
                return self.STATUS_LED_COLOR_OFF
        else:
            return 'N/A'

    def set_status_master_led(self, color):
        """
        Sets the state of the front panel PSU status LED

        Returns:
            bool: True if status LED state is set successfully, False if
                  not
        """
        
        return False
