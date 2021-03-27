# Name: fan_drawer.py, version: 1.0
#
# Description: Module contains the definitions of FAN drawer related APIs
# for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform_base.fan_drawer_base import FanDrawerBase
    from platform_ndk import nokia_common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class FanDrawer(FanDrawerBase):
    """Nokia IXR-7250 Platform-specific FanDrawer class"""

    def __init__(self, fantray_index):
        self._fan_list = []
        self.fantray_idx = fantray_index
        self.max_consumed_power = 0.0
        self.is_cpm = 1
        if nokia_common.is_cpm() == 0:
            self.is_cpm = 0

    def get_all_fans(self):
        return self._fan_list

    def get_name(self):
        """
        Retrieves the fantray name
        Returns:
            string: The name of the device
        """
        return "FanTray{}".format(self.fantray_idx)

    def get_model(self):
        """
        Retrieves the part number of the FAN
        Returns:
            string: Part number of FAN
        """
        if len(self._fan_list) == 0:
            return nokia_common.NOKIA_INVALID_STRING

        return self._fan_list[0].get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the FAN
        Returns:
            string: Serial number of FAN
        """
        if len(self._fan_list) == 0:
            return nokia_common.NOKIA_INVALID_STRING

        return self._fan_list[0].get_serial()

    def get_presence(self):
        """
        Retrieves the presence of the FAN
        Returns:
            bool: True if fan is present, False if not
        """
        if len(self._fan_list) == 0:
            return False

        return self._fan_list[0].get_presence()

    def get_status(self):
        """
        Retrieves the operational status of the FAN
        Returns:
            bool: True if FAN is operating properly, False if not
        """
        if len(self._fan_list) == 0:
            return False

        return self._fan_list[0].get_status()

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if set success, False if fail.
        """
        if len(self._fan_list) == 0:
            return False
        return self._fan_list[0].set_status_led(color)

    def get_status_led(self):
        """
        Gets the state of the Fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if len(self._fan_list) == 0:
            return self.STATUS_LED_COLOR_OFF
        return self._fan_list[0].get_status_led()

    def set_maximum_consumed_power(self, consumed_power):
        """
        Set the maximum power drawn by this card.

        consumed_power - A float, with value of the maximum consumable
            power of the module.
        """
        self.max_consumed_power = consumed_power

    def get_maximum_consumed_power(self):
        """
        Retrieves the maximum power drawn by this card.

        Returns:
            A float, with value of the maximum consumable power of the
            module.
        """
        return self.max_consumed_power

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device or
                     -1 if cannot determine the position
        """
        return self.fantray_idx

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
