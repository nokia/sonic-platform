"""
    NOKIA 7250 IXR-X3B

    Module contains an implementation of SONiC Platform Base API and
    provides the Thermals' information which are available in the platform
"""

try:
    import glob
    from sonic_platform_base.thermal_base import ThermalBase
    from sonic_py_common import logger
    from swsscommon import swsscommon
    from swsscommon.swsscommon import SonicV2Connector
    from sonic_platform.sysfs import read_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

sonic_logger = logger.Logger('thermal')
sonic_logger.set_min_log_priority_info()
THERMAL_NUM = 15

class Thermal(ThermalBase):
    """Nokia platform-specific Thermal class"""

    HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/"
    I2C_DEV_LIST = ["7-001e", "19-0049", "19-004a", "19-004b", "1-0049", "0-0018", "0-0019"]
    THERMAL_NAME = ["FPGA", "MB Left", "MB Right", "MB Center", "MB CPU", "DDR1", "DDR2", "Max Port Temp.",
                    "ASIC0_DRAM0", "ASIC0_DRAM1", "ASIC1_DRAM0", "ASIC1_DRAM1", "ASIC0", "ASIC1", "CPU"]
    THRESHHOLD = [78.0, 68.0, 68.0, 68.0, 68.0, 75.0, 75.0, 71.0, 83.0, 83.0, 83.0, 83.0, 93.0, 93.0, 88.0]

    def __init__(self, thermal_index, sfps):
        ThermalBase.__init__(self)
        self.index = thermal_index + 1
        self.is_fan_thermal = False
        self.dependency = None
        self._minimum = None
        self._maximum = None
        self.thermal_high_threshold_file = None

        # sysfs file for crit high threshold value if supported for this sensor
        self.thermal_high_crit_threshold_file = None

        if self.index == THERMAL_NUM:    # CPU
            self.device_path = glob.glob("/sys/devices/pci0000:00/0000:00:18.3/hwmon/hwmon*/")
            self.thermal_temperature_file = self.device_path[0] + "temp1_input"
        elif self.index >= THERMAL_NUM-6 and self.index <= THERMAL_NUM-1:    # MAC internal sensor
            self.thermal_temperature_file = None
        elif self.index == THERMAL_NUM-7:    # Max temperature of all optics
            self.thermal_temperature_file = None
            self.sfps = sfps
        else:
            self.device_path = glob.glob(self.HWMON_DIR.format(self.I2C_DEV_LIST[self.index - 1]))
            self.thermal_temperature_file = self.device_path[0] + "temp1_input"

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
        if self.index >= THERMAL_NUM-6 and self.index <= THERMAL_NUM-1:
            if self.index == THERMAL_NUM-1 or self.index == THERMAL_NUM-3 or self.index == THERMAL_NUM-4:
                namespace = 'asic1'
            if self.index == THERMAL_NUM-2 or self.index == THERMAL_NUM-5 or self.index == THERMAL_NUM-6:
                namespace = 'asic0'
            if not swsscommon.SonicDBConfig.isGlobalInit():
                swsscommon.SonicDBConfig.initializeGlobalConfig()
            db = SonicV2Connector(use_unix_socket_path=True, namespace=namespace)
            db.connect(db.STATE_DB)
            data_dict = db.get_all(db.STATE_DB, 'ASIC_TEMPERATURE_INFO')
            if self.index == THERMAL_NUM-1 or self.index == THERMAL_NUM-2:
                thermal_temperature = float(data_dict['maximum_temperature'])
            elif self.index == THERMAL_NUM-6 or self.index == THERMAL_NUM-4:
                thermal_temperature = float(data_dict['temperature_9'])
            elif self.index == THERMAL_NUM-5 or self.index == THERMAL_NUM-3:
                thermal_temperature = float(data_dict['temperature_10'])
        elif self.index == THERMAL_NUM-7:
            thermal_temperature = 0
            for sfp in self.sfps:
                try:
                    if sfp.get_presence():
                        temp = sfp.get_temperature()
                    else:
                        temp = None
                except:
                    temp = None
                if (temp is not None) and (temp > thermal_temperature):
                    thermal_temperature = temp
        else:
            thermal_temperature = read_sysfs_file(self.thermal_temperature_file)
            if (thermal_temperature != 'ERR'):
                thermal_temperature = float(thermal_temperature) / 1000
                if self.index == THERMAL_NUM:
                    thermal_temperature += 10.0
            else:
                thermal_temperature = 0
        
        if self._minimum is None or self._minimum > thermal_temperature:
            self._minimum = thermal_temperature
        if self._maximum is None or self._maximum < thermal_temperature:
            self._maximum = thermal_temperature

        return float(f"{thermal_temperature:.3f}")

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        return self.THRESHHOLD[self.index - 1]

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
        return 'N/A'

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
