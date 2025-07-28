"""
    Nokia IXR7220 D4-36D

    Module contains an implementation of SONiC Platform Base API and
    provides the PSUs' information which are available in the platform
"""

try:
    import os
    import subprocess
    from sonic_platform.sysfs import read_sysfs_file
    from sonic_platform_base.psu_base import PsuBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

PSU_NUM = 2
PSU_DIR = ["/sys/bus/i2c/devices/9-0059/",
           "/sys/bus/i2c/devices/9-0058/"]
REG_DIR = "/sys/bus/i2c/devices/10-0060/"
PSU_EEPROM_I2C = [["9", "51"],
                  ["9", "50"]]

PSU_OUTPUT_VOLTAGE_MIN = 11 * 1000
PSU_OUTPUT_VOLTAGE_MAX = 14 * 1000

sonic_logger = logger.Logger(PsuBase.DEVICE_TYPE)
sonic_logger.set_min_log_priority_error()

class Psu(PsuBase):
    """Nokia platform-specific PSU class for 7220 D4-36D """

    def __init__(self, psu_index):
        PsuBase.__init__(self)
        # PSU is 1-based in Nokia platforms
        self.index = psu_index + 1
        self._fan_list = []
        self.psu_dir = PSU_DIR[psu_index]
        self.eeprom = None

    def _read_bin_file(self, sysfs_file, length):
        """
        On successful read, returns the value read from given
        sysfs file and on failure returns ERR
        """
        rv = 'ERR'

        try:
            with open(sysfs_file, 'rb') as fd:
                rv = fd.read(length)
                fd.close()
        except FileNotFoundError:
            print(f"Error: {sysfs_file} doesn't exist.")
        except PermissionError:
            print(f"Error: Permission denied when reading file {sysfs_file}.")
        except IOError:
            print(f"IOError: An error occurred while reading file {sysfs_file}.")
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
            result = read_sysfs_file(REG_DIR+f"psu{i+1}_pwr_ok")
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
        result = read_sysfs_file(REG_DIR + f"psu{self.index}_present")

        eeprom_path = f"/sys/bus/i2c/devices/{PSU_EEPROM_I2C[self.index-1][0]}-00{PSU_EEPROM_I2C[self.index-1][1]}/eeprom"
        if result == '0':
            if not os.path.exists(eeprom_path):
                i2c_address = f"0x{PSU_EEPROM_I2C[self.index-1][1]}"
                if os.path.exists(f"/sys/bus/i2c/devices/{PSU_EEPROM_I2C[self.index-1][0]}-00{PSU_EEPROM_I2C[self.index-1][1]}"):
                    i2c_file = f"/sys/bus/i2c/devices/i2c-{PSU_EEPROM_I2C[self.index-1][0]}/delete_device"
                    os.system(f"echo {i2c_address} > {i2c_file}")
                i2c_file = f"/sys/bus/i2c/devices/i2c-{PSU_EEPROM_I2C[self.index-1][0]}/new_device"
                os.system(f"echo 24c02 {i2c_address} > {i2c_file}")
            return True
        else:
            if os.path.exists(eeprom_path):
                i2c_address = f"0x{PSU_EEPROM_I2C[self.index-1][1]}"
                i2c_file = f"/sys/bus/i2c/devices/i2c-{PSU_EEPROM_I2C[self.index-1][0]}/delete_device"
                os.system(f"echo {i2c_address} > {i2c_file}")

        return False

    def get_model(self):
        """
        Retrieves the part number of the PSU

        Returns:
            string: Part number of PSU
        """
        if self.get_presence():
            if self.eeprom is None:
                try:
                    eeprom_path = f"/sys/bus/i2c/devices/{PSU_EEPROM_I2C[self.index-1][0]}-00{PSU_EEPROM_I2C[self.index-1][1]}/eeprom"
                    subprocess.run(["chmod", "644", eeprom_path], stderr=subprocess.STDOUT)
                    self.eeprom = self._read_bin_file(eeprom_path, 256)
                except:
                    return 'N/A'
            return self.eeprom[12:20].decode()

        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the PSU

        Returns:
            string: Serial number of PSU
        """
        if self.get_presence():
            if self.eeprom is None:
                try:
                    eeprom_path = f"/sys/bus/i2c/devices/{PSU_EEPROM_I2C[self.index-1][0]}-00{PSU_EEPROM_I2C[self.index-1][1]}/eeprom"
                    subprocess.run(["chmod", "644", eeprom_path], stderr=subprocess.STDOUT)
                    self.eeprom = self._read_bin_file(eeprom_path, 256)
                except:
                    return 'N/A'
            return self.eeprom[88:99].decode()

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
            if self.eeprom is None:
                try:
                    eeprom_path = f"/sys/bus/i2c/devices/{PSU_EEPROM_I2C[self.index-1][0]}-00{PSU_EEPROM_I2C[self.index-1][1]}/eeprom"
                    subprocess.run(["chmod", "644", eeprom_path], stderr=subprocess.STDOUT)
                    self.eeprom = self._read_bin_file(eeprom_path, 256)
                except:
                    return 'N/A'
            return self.eeprom[21:32].decode()

        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        result = read_sysfs_file(REG_DIR+f"psu{self.index}_pwr_ok")
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
        if self.get_presence():
            v_out = read_sysfs_file(self.psu_dir+"psu_v_out")
        else:
            v_out = 0.0

        return float(v_out)/1000

    def get_current(self):
        """
        Retrieves present electric current supplied by PSU

        Returns:
            A float number, the electric current in amperes, e.g 15.4
        """
        if self.get_presence():
            i_out = read_sysfs_file(self.psu_dir+"psu_i_out")
        else:
            i_out = 0.0

        return float(i_out)/1000

    def get_power(self):
        """
        Retrieves current energy supplied by PSU

        Returns:
            A float number, the power in watts, e.g. 302.6
        """
        if self.get_presence():
            p_out = read_sysfs_file(self.psu_dir+"psu_p_out")
        else:
            p_out = 0.0

        return float(p_out)/1000

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
        return float(PSU_OUTPUT_VOLTAGE_MAX/1000)

    def get_voltage_low_threshold(self):
        """
        Retrieves the low threshold PSU voltage output

        Returns:
            A float number, the low threshold output voltage in volts,
            e.g. 12.1
        """
        return float(PSU_OUTPUT_VOLTAGE_MIN/1000)

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
        if not self.get_presence():
            return self.STATUS_LED_COLOR_OFF

        if self.get_status():
            return self.STATUS_LED_COLOR_GREEN

        return self.STATUS_LED_COLOR_AMBER

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
