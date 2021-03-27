# Name: component.py, version: 1.0
#
# Description: Module contains the definitions of firmware component
# related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#
#
try:
    from sonic_platform_base.component_base import ComponentBase
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class Component(ComponentBase):
    """Nokia Platform-specific Component class"""

    def __init__(self, component_index, dev_type, name, desc, stub):
        self.index = component_index
        self.stub = stub
        self.dev_type = dev_type
        self.name = name
        self.description = desc

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.name

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self.description

    def get_model(self):
        """
        Retrieves the part number of the component
        Returns:
            string: Part number of component
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the component
        Returns:
            string: Serial number of component
        """
        return 'N/A'

    def get_presence(self):
        """
        Retrieves the presence of the component
        Returns:
            bool: True if  present, False if not
        """
        return True

    def get_status(self):
        """
        Retrieves the operational status of the component
        Returns:
            bool: True if component is operating properly, False if not
        """
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
        return False

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FIRMWARE_SERVICE)
        if not channel or not stub:
            return nokia_common.NOKIA_INVALID_FIRMWARE_VERSION
        ret, response = nokia_common.try_grpc(stub.ReqHwFirmwareVersion,
                                              platform_ndk_pb2.ReqHwFirmwareInfoPb(dev_type=self.dev_type))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return nokia_common.NOKIA_INVALID_FIRMWARE_VERSION

        return response.version

    def get_available_firmware_version(self, image_path):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the available firmware version of the component
        """
        return "N/A"

    def get_firmware_update_notification(self, image_path):
        """
        Retrieves a notification on what should be done in order to complete
        the component firmware update

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the component firmware update notification if required.
            By default 'None' value will be used, which indicates that no actions are required
        """
        return 'None'

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        return False

    def update_firmware(self, image_path):
        """
        Updates firmware of the component

        This API performs firmware update: it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.) - this will be done automatically by API

        Args:
            image_path: A string, path to firmware image

        Raises:
            RuntimeError: update failed
        """
        return False
