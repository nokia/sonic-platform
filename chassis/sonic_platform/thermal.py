# Name: thermal.py, version: 1.0
#
# Description: Module contains the definitions of Thermal sensors
# related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    import time
    from sonic_platform_base.thermal_base import ThermalBase
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Thermal(ThermalBase):
    """Nokia platform-specific Thermal class"""

    def __init__(self, thermal_index, map_index, thermal_name, fan_sensor, stub):
        self.index = thermal_index
        self.map_index = map_index
        # self.thermal_name = 'Thermal {}'.format(map_index)
        # Has (fan) suffix for fan-algo sensors
        if fan_sensor:
            self.thermal_name = thermal_name + '(fan)'
        else:
            self.thermal_name = thermal_name
        self.is_psu_thermal = False
        self.dependency = None
        self.stub = stub

        self.thermal_temperature = nokia_common.NOKIA_SONIC_PMON_INIT_TEMP
        self.thermal_high_threshold = 0.0
        self.thermal_low_threshold = 0.0
        self.thermal_high_critical_threshold = 0.0
        self.thermal_low_critical_threshold = 0.0
        self.thermal_min_temp = nokia_common.NOKIA_SONIC_PMON_MAX_TEMP_THRESHOLD
        self.thermal_max_temp = 0.0
        self.timestamp = 0

    def get_name(self):
        """
        Retrieves the name of the thermal

        Returns:
            string: The name of the thermal
        """
        return self.thermal_name

    def get_presence(self):
        """
        Retrieves the presence of the thermal

        Returns:
            bool: True if thermal is present, False if not
        """
        if self.dependency:
            return self.dependency.get_presence()
        else:
            return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the Thermal

        Returns:
            string: Model/part number of Thermal
        """
        return nokia_common.NOKIA_INVALID_STRING

    def get_serial(self):
        """
        Retrieves the serial number of the Thermal

        Returns:
            string: Serial number of Thermal
        """
        return nokia_common.NOKIA_INVALID_STRING

    def get_status(self):
        """
        Retrieves the operational status of the thermal

        Returns:
            A boolean value, True if thermal is operating properly,
            False if not
        """
        if self.dependency:
            return self.dependency.get_status()
        else:
            return True

    def _get_all_temperature_info(self):
        current_time = time.time()
        if self.timestamp != 0 and (current_time - self.timestamp) < 10:
            return

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
        if not channel or not stub:
            return

        ret, response = nokia_common.try_grpc(stub.GetThermalAllTempInfo,
                                              platform_ndk_pb2.ReqTempParamsPb(idx=self.map_index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return

        self.timestamp = current_time
        self.thermal_temperature = float(response.temp_info.curr_temp)
        if float(response.temp_info.min_temp) == nokia_common.NOKIA_INVALID_TEMP:
            self.thermal_min_temp = min(self.thermal_min_temp, self.thermal_temperature)
        else:
            self.thermal_min_temp = float(response.temp_info.min_temp)
            
        if float(response.temp_info.max_temp) == nokia_common.NOKIA_INVALID_TEMP:
            self.thermal_max_temp = max(self.thermal_max_temp, self.thermal_temperature)
        else:
            self.thermal_max_temp = float(response.temp_info.max_temp)
            
        self.thermal_low_threshold = float(response.temp_info.low_threshold)
        self.thermal_high_threshold = min(float(response.temp_info.high_threshold),
                                          nokia_common.NOKIA_SONIC_PMON_MAX_TEMP_THRESHOLD)
        
    def get_temperature(self):
        """
        Retrieves current temperature reading from thermal

        Returns:
            A float number of current temperature in Celsius up to
            nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self._get_all_temperature_info()
        return float("{:.3f}".format(self.thermal_temperature))

    def get_high_threshold(self):
        """
        Retrieves the high threshold temperature of thermal

        Returns:
            A float number, the high threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        self._get_all_temperature_info()
        return float("{:.3f}".format(self.thermal_high_threshold))

    def get_low_threshold(self):
        """
        Retrieves the low threshold temperature of thermal

        Returns:
            A float number, the low threshold temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        self._get_all_temperature_info()
        return float("{:.3f}".format(self.thermal_low_threshold))

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

    def get_high_critical_threshold(self):
        """
        Retrieves the high critical threshold temperature of thermal

        Returns:
            A float number, the high critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self.get_high_threshold()
        self.thermal_high_critical_threshold = min((self.thermal_high_threshold *
                                                    nokia_common.NOKIA_TEMP_HIGH_CRITICAL_THRESHOLD_MULTIPLIER),
                                                   nokia_common.NOKIA_SONIC_PMON_MAX_TEMP_THRESHOLD)

        return float("{:.3f}".format(self.thermal_high_critical_threshold))

    def get_low_critical_threshold(self):
        """
        Retrieves the low critical threshold temperature of thermal

        Returns:
            A float number, the low critical threshold temperature of thermal in Celsius
            up to nearest thousandth of one degree Celsius, e.g. 30.125
        """
        self.get_low_threshold()
        self.thermal_low_critical_threshold = (self.thermal_low_threshold *
                                               nokia_common.NOKIA_TEMP_LOW_CRITICAL_THRESHOLD_MULTIPLIER)
        return float("{:.3f}".format(self.thermal_low_critical_threshold))

    def get_minimum_recorded(self):
        """
        Retrieves the minimum recorded temperature of thermal

        Returns:
            A float number, the minimum recorded temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        self._get_all_temperature_info()
        return float("{:.3f}".format(self.thermal_min_temp))

    def get_maximum_recorded(self):
        """
        Retrieves the maximum recorded temperature of thermal

        Returns:
            A float number, the maximum recorded temperature of thermal in
            Celsius up to nearest thousandth of one degree Celsius,
            e.g. 30.125
        """
        self._get_all_temperature_info()
        return float("{:.3f}".format(self.thermal_max_temp))

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device or
                     -1 if cannot determine the position
        """
        return self.index + 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False
