# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of PSU related APIs
# for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    import time
    from sonic_platform_base.psu_base import PsuBase
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    """Nokia IXR-7250 Platform-specific PSU class"""

    def __init__(self, psu_index, stub):
        PsuBase.__init__(self)
        self.index = psu_index + 1
        self.stub = stub
        self.is_cpm = 1
        if nokia_common.is_cpm() == 0:
            self.is_cpm = 0
        self.reset()

    def reset(self):
        self.model = 'Unknown'
        self.serial = 'Unknown'
        self.revision = 'Unknown'
        self.min_voltage = 0.0
        self.max_voltage = 0.0
        self.ambient_temperature = 0.0
        self.max_temperature = 0.0
        self.not_available_error = 'N/A'
        self.output_current = 0.0
        self.output_voltage = 0.0
        self.output_power = 0.0
        self.input_current = 0.0
        self.input_voltage = 0.0
        self.input_power = 0.0
        self.max_supplied_power = 0.0
        self.presence = False
        self.status = False
        self.timestamp = 0

    def _get_psu_bulk_info(self):
        if self.is_cpm == 0:
            return False

        current_time = time.time()
        if current_time > self.timestamp and current_time - self.timestamp <= 3:
            return True

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return status
        ret, response = nokia_common.try_grpc(stub.GetPsuStatusInfo,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return False

        status = response.status_info.psu_presence
        # Reset if PSU presence is False
        if status is False:
            self.reset()
        else:
            self.presence = response.status_info.psu_presence
            self.status = response.status_info.psu_status
            self.output_current = round(response.status_info.output_current, 2)
            self.output_voltage = round(response.status_info.output_voltage, 2)
            self.output_power = round(response.status_info.output_power, 2)
            self.ambient_temperature = response.status_info.ambient_temp
            self.min_voltage = response.status_info.min_voltage
            self.max_voltage = response.status_info.max_voltage
            self.max_temperature = response.status_info.max_temperature
            self.max_supplied_power = round(response.status_info.supplied_power, 2)
            self.input_current = round(response.status_info.input_current, 2)
            self.input_voltage = round(response.status_info.input_voltage, 2)
            self.input_power = round(response.status_info.input_power, 2)

        current_time = time.time()
        self.timestamp = current_time
        return True

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return "PSU{}".format(self.index)

    def get_model(self):
        """
        Retrieves the part number of the PSU
        Returns:
            string: Part number of PSU
        """
        if self.is_cpm == 0:
            return self.not_available_error

        if self.model != 'Unknown':
           return self.model

        if self.get_presence() is False:
            return self.not_available_error

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return nokia_common.NOKIA_INVALID_STRING
        ret, response = nokia_common.try_grpc(stub.GetPsuModel,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)
        if ret is False:
            return nokia_common.NOKIA_INVALID_STRING

        self.product = response.fru_info.product_name
        self.model = response.fru_info.part_number
        self.serial = response.fru_info.serial_number

        return self.model

    def get_serial(self):
        """
        Retrieves the serial number of the PSU
        Returns:
            string: Serial number of PSU
        """
        if self.is_cpm == 0:
            return 'N/A'

        if self.serial != 'Unknown':
            return self.serial

        if self.get_presence() is False:
            return self.not_available_error

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return nokia_common.NOKIA_INVALID_STRING
        ret, response = nokia_common.try_grpc(stub.GetPsuSerial,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)
        if ret is False:
            return nokia_common.NOKIA_INVALID_STRING

        self.product = response.fru_info.product_name
        self.model = response.fru_info.part_number
        self.serial = response.fru_info.serial_number

        return self.serial

    def get_presence(self):
        """
        Retrieves the presence of the Power Supply Unit (PSU)

        Returns:
            bool: True if PSU is present, False if not
        """
        if self._get_psu_bulk_info() is False:
            return False
        else:
            return self.presence

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        if self._get_psu_bulk_info() is False:
            return False
        else:
            return self.status

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        status = False
        power = self.get_power()

        if (self.get_status() and (power > 0.0) and (power < self.get_maximum_supplied_power())):
            status = True

        return status

    def _get_cached_status(self):
        return self.status

    def get_status_led(self):
        """
        Gets the state of the PSU status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if self.get_powergood_status():
            return self.STATUS_LED_COLOR_GREEN
        else:
            return self.STATUS_LED_COLOR_OFF

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

    def get_num_fans(self):
        return 0

    def get_all_fans(self):
        return []

    def get_temperature(self):
        self._get_psu_bulk_info()
        return self.ambient_temperature

    def get_temperature_high_threshold(self):
        if self._get_cached_status() is False:
            return self.max_temperature

        if self.max_temperature != 0.0:
            return self.max_temperature

        self._get_psu_bulk_info()
        return self.max_temperature

    def get_current(self):
        self._get_psu_bulk_info()
        return self.output_current

    def get_power(self):
        """
        Get PSU bulk info includes return output_power
        """
        self._get_psu_bulk_info()
        return self.output_power

    def get_voltage(self):
        self._get_psu_bulk_info()
        return self.output_voltage

    def get_input_voltage(self):
        """
        Retrieves current PSU voltage input

        Returns:
            A float number, the input voltage in volts,
            e.g. 12.1
        """
        self._get_psu_bulk_info()
        return self.input_voltage

    def get_input_current(self):
        """
        Retrieves present electric current supplied to PSU

        Returns:
            A float number, electric current in amperes,
            e.g. 15.4
        """
        self._get_psu_bulk_info()
        return self.input_current

    def get_input_power(self):
        """
        Retrieves current energy supplied to PSU

        Returns:
            A float number, the power in watts,
            e.g. 302.6
        """
        self._get_psu_bulk_info()
        return self.input_power

    def get_voltage_high_threshold(self):
        if self.max_voltage != 0.0:
            return self.max_voltage

        if self._get_cached_status() is False:
            return self.max_voltage

        self._get_psu_bulk_info()
        return self.max_voltage

    def get_voltage_low_threshold(self):
        if self.min_voltage != 0.0:
            return self.min_voltage

        if self._get_cached_status() is False:
            return self.min_voltage

        self._get_psu_bulk_info()
        return self.min_voltage

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum output power in watts of a power supply unit (PSU)
        """
        self._get_psu_bulk_info()
        return self.max_supplied_power

    @classmethod
    def get_status_master_led(cls):
        """
        Get the PSU master led register with the input color
        """
        color = Psu.STATUS_LED_COLOR_OFF
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_MASTER_PSU_STATUS
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
        if not channel or not stub:
            return color
        ret, response = nokia_common.try_grpc(stub.GetLed,
                                              platform_ndk_pb2.ReqLedInfoPb(led_type=led_type))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return color

        color = nokia_common.led_info_to_color(response.led_get.led_info[0])
        return color

    @classmethod
    def set_status_master_led(cls, color):
        """
        Set the PSU master led register with the input color
        """
        # Linecard doesn't have psu_master_led
        if nokia_common.is_cpm() == 0:
            return True
        
        cls.psu_master_led_color = color
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_MASTER_PSU_STATUS
        _led_info = nokia_common.led_color_to_info(color)

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.SetLed,
                                              platform_ndk_pb2.ReqLedInfoPb(led_type=led_type, led_info=_led_info))
        nokia_common.channel_shutdown(channel)

        return True

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device or
            -1 if cannot determine the position
        """
        return self.index 

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.revision
