"""
    Nokia IXR7220-D4-36D

    Module contains an implementation of SONiC Platform Base API and
    provides the Thermals' information which are available in the platform
"""

try:
    import os
    import glob
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common import logger
    from swsscommon.swsscommon import SonicV2Connector
    from sonic_platform.sysfs import read_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

sonic_logger = logger.Logger('thermal')

class Thermal(ThermalBase):
    """Nokia platform-specific Thermal class"""

    HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/"
    THRESHOLD = [60.0, 60.0, 60.0, 60.0, 60.0, 100.0, 90.0]
    CRITICAL_THRESHOLD = [70.0, 70.0, 70.0, 70.0, 70.0, 104.0, 100.0]
    I2C_DEV_LIST = ["15-0048", "15-0049", "15-004a", "15-004c", "15-004b"]
    THERMAL_NAME = ["MB Front", "MB Rear", "MB Left",
                    "MB Right", "CPU Board", "CPU", "ASIC TD4"]

    def __init__(self, thermal_index):
        ThermalBase.__init__(self)
        self.index = thermal_index + 1
        self.dependency = None
        self._minimum = None
        self._maximum = None
        self.thermal_high_crit_thresh_file = None
        self.thermal_high_thres = None
        self.__create_i2c_tree()

    def __create_i2c_tree(self):

        # special cases
        if self.index == len(self.THERMAL_NAME):     # MAC internal sensor
            return
        elif self.index == len(self.THERMAL_NAME)-1: # CPU internal sensor
            self.device_path = glob.glob("/sys/bus/platform/devices/coretemp.0/hwmon/hwmon*/")
            self.thermal_tmp_file = self.device_path[0] + "temp1_input"
        else:
            if not os.path.exists(f"/sys/bus/i2c/devices/{self.I2C_DEV_LIST[self.index-1]}"):
                i2c_addr = "0x" + self.I2C_DEV_LIST[self.index-1][-2:]
                os.system(f"echo lm75 {i2c_addr} > /sys/bus/i2c/devices/i2c-15/new_device")

            self.device_path = glob.glob(self.HWMON_DIR.format(self.I2C_DEV_LIST[self.index-1]))
            self.thermal_tmp_file = self.device_path[0] + "temp1_input"

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        return self.THERMAL_NAME[self.index - 1]

    def get_presence(self):
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        if self.dependency:
            return self.dependency.get_presence()
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal

        Returns:
            string: Serial number of Thermal
        """
        return 'NA'

    def get_status(self):
        """
        Retrieves the operational status of the thermal

        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        if self.dependency:
            return self.dependency.get_status()
        return True

    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        if self.index == len(self.THERMAL_NAME):
            db = SonicV2Connector()
            db.connect(db.STATE_DB)
            data_dict = db.get_all(db.STATE_DB, 'ASIC_TEMPERATURE_INFO')
            thermal_tmp = float(data_dict['maximum_temperature'])
        else:
            thermal_tmp = read_sysfs_file(self.thermal_tmp_file)
            if (thermal_tmp != 'ERR'):
                thermal_tmp = float(thermal_tmp) / 1000
            else:
                thermal_tmp = 0

        if self._minimum is None or self._minimum > thermal_tmp:
            self._minimum = thermal_tmp
        if self._maximum is None or self._maximum < thermal_tmp:
            self._maximum = thermal_tmp

        return float(f"{thermal_tmp:.3f}")

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        return self.THRESHOLD[self.index - 1]

    def set_high_threshold(self, temperature):
        """
        Sets the high threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
        return False

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return self.CRITICAL_THRESHOLD[self.index - 1]

    def set_high_critical_threshold(self):
        """
        Sets the high_critical threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
        return False

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal
        Returns:
            A float number, the low threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        return 0.0

    def set_low_threshold(self, temperature):
        """
        Sets the low threshold temperature of thermal

        Args :
            temperature: A float number up to nearest thousandth of one
            degree Celsius, e.g. 30.125
        Returns:
            A boolean, True if threshold is set successfully, False if
            not
        """
        # Thermal threshold values are pre-defined based on HW.
        return False

    def get_minimum_recorded(self):
        """
        Retrieves minimum recorded temperature
        """
        self.get_temperature()
        return self._minimum

    def get_maximum_recorded(self):
        """
        Retrieves maxmum recorded temperature
        """
        self.get_temperature()
        return self._maximum

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
