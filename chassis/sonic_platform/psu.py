# Name: psu.py, version: 1.0
#
# Description: Module contains the definitions of PSU related APIs
# for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform_base.psu_base import PsuBase
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Psu(PsuBase):
    """Nokia IXR-7250 Platform-specific PSU class"""

    def __init__(self, psu_index, stub):
        self.index = psu_index + 1
        self.stub = stub
        self.is_cpm = 1
        if nokia_common.is_cpm() == 0:
            self.is_cpm = 0
        self.model = 'Unknown'
        self.serial = 'Unknown'
        self.min_voltage = 0.0
        self.max_voltage = 0.0
        self.ambient_temperature = 0.0
        self.max_temperature = 0.0
        self.not_available_error = 'N/A'
        self.output_current = 0.0
        self.output_voltage = 0.0
        self.output_power = 0.0
        # Overriding _fan_list class variable defined in PsuBase, to
        # make it unique per Psu object
        # self._fan_list = []

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
        status = False
        if self.is_cpm == 0:
            return status

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return status
        ret, response = nokia_common.try_grpc(stub.GetPsuPresence,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return status

        status = response.psu_presence
        return status

    def get_status(self):
        """
        Retrieves the operational status of the PSU

        Returns:
            bool: True if PSU is operating properly, False if not
        """
        status = False
        if self.is_cpm == 0:
            return status

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return status
        ret, response = nokia_common.try_grpc(stub.GetPsuStatus,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return status

        status = response.psu_status
        return status

    def get_powergood_status(self):
        """
        Retrieves the powergood status of PSU

        Returns:
            A boolean, True if PSU has stablized its output voltages and
            passed all its internal self-tests, False if not.
        """
        status = False
        if self.get_status():
            status = True

        return status

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
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.ambient_temperature
        ret, response = nokia_common.try_grpc(stub.GetPsuTemperature,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.ambient_temperature

        self.ambient_temperature = response.ambient_temp
        return self.ambient_temperature

    def get_temperature_high_threshold(self):
        if self.max_temperature != 0.0:
            return self.max_temperature

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.max_temperature
        ret, response = nokia_common.try_grpc(stub.GetPsuMaxTemperature,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.max_temperature

        self.min_voltage = response.fault_info.min_voltage
        self.max_voltage = response.fault_info.max_voltage
        self.max_temperature = response.fault_info.max_temperature

        return self.max_temperature

    def get_current(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.output_current
        ret, response = nokia_common.try_grpc(stub.GetPsuOutputCurrent,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.output_current

        self.output_current = response.output_current
        return self.output_current

    def get_power(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.output_power
        ret, response = nokia_common.try_grpc(stub.GetPsuOutputPower,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.output_power

        self.output_power = response.output_power
        return self.output_power

    def get_voltage(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.output_voltage
        ret, response = nokia_common.try_grpc(stub.GetPsuOutputVoltage,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.output_voltage

        self.output_voltage = response.output_voltage
        return self.output_voltage

    def get_voltage_high_threshold(self):
        if self.max_voltage != 0.0:
            return self.max_voltage

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.max_voltage
        ret, response = nokia_common.try_grpc(stub.GetPsuMaxTemperature,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.max_voltage

        self.min_voltage = response.fault_info.min_voltage
        self.max_voltage = response.fault_info.max_voltage
        self.max_temperature = response.fault_info.max_temperature

        return self.max_voltage

    def get_voltage_low_threshold(self):
        if self.min_voltage != 0.0:
            return self.min_voltage

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.min_voltage
        ret, response = nokia_common.try_grpc(stub.GetPsuMaxTemperature,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.min_voltage

        self.min_voltage = response.fault_info.min_voltage
        self.max_voltage = response.fault_info.max_voltage
        self.max_temperature = response.fault_info.max_temperature

        return self.min_voltage

    def get_maximum_supplied_power(self):
        """
        Retrieves the maximum output power in watts of a power supply unit (PSU)
        """
        if self.is_cpm == 0:
            return 0.0

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return self.max_supplied_power
        ret, response = nokia_common.try_grpc(stub.GetPsuMaximumPower,
                                              platform_ndk_pb2.ReqPsuInfoPb(psu_idx=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.max_supplied_power

        self.max_supplied_power = response.supplied_power
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
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
