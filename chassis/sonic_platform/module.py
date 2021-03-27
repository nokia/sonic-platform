# Name: module.py, version: 1.0
#
# Description: Module contains the definitions of pluggable-modules APIs
# for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform_base.module_base import ModuleBase
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

DESCRIPTION_MAPPING = {
    "imm32-100g-qsfp28+4-400g-qsfpdd": "Nokia-IXR7250-32x100G-4x400G",
    "cpm-ixr": "Nokia-IXR7250-SUP",
    "cpm2-ixr": "Nokia-IXR7250-SUP",
    "imm36-400g-qsfpdd": "Nokia-IXR7250-32x100G"
}


class Module(ModuleBase):
    """Nokia IXR-7250 Platform-specific Module class"""

    def __init__(self, module_index, module_name, module_type, module_slot, stub):
        super(Module, self).__init__()
        self.module_index = module_index
        self.module_name = module_name
        self.module_type = module_type
        self.hw_slot = module_slot
        self.chassis_stub = stub
        self.oper_status = ModuleBase.MODULE_STATUS_EMPTY
        self.midplane_status = nokia_common.NOKIA_INVALID_IP
        self.max_consumed_power = 0.0

    def get_name(self):
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.module_name

    def get_presence(self):
        if self.get_oper_status() == ModuleBase.MODULE_STATUS_EMPTY:
            return False
        else:
            return True

    def get_status(self):
        if self.get_oper_status() == ModuleBase.MODULE_STATUS_ONLINE:
            return True
        else:
            return False

    def get_description(self):
        """
        Retrieves the description of the module

        Returns:
            string: The description of the module
        """
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return 'Unavailable'
        platform_module_type = self.get_platform_type()
        ret, response = nokia_common.try_grpc(
            stub.GetModuleName,
            platform_ndk_pb2.ReqModuleInfoPb(module_type=platform_module_type, hw_slot=self.get_slot()))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return 'Unavailable'

        description = response.name
        if response.name in DESCRIPTION_MAPPING:
            description = DESCRIPTION_MAPPING[response.name]
            if platform_module_type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_CONTROL:
                if nokia_common.get_chassis_type() == platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_IXR6:
                    description = "Nokia-IXR7250-SUP-6"
                else:
                    description = "Nokia-IXR7250-SUP-10"
        return description

    def get_slot(self):
        """
        Retrieves the slot where the module is present

        Returns:
            int: slot representation, usually number
        """
        return self.hw_slot

    def get_type(self):
        """
        Retrieves the type of the module

        Returns:
            int: module-type
        """
        return self.module_type

    def get_oper_status(self):
        """
        Retrieves the operational status of the module

        Returns:
            string: The status-string of the module
        """
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return self.oper_status
        platform_module_type = self.get_platform_type()
        ret, response = nokia_common.try_grpc(
            stub.GetModuleStatus,
            platform_ndk_pb2.ReqModuleInfoPb(module_type=platform_module_type, hw_slot=self.get_slot()))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.oper_status

        self.oper_status = response.status
        return response.status

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device or
                     -1 if cannot determine the position
        """
        return self.hw_slot

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def reboot(self, reboot_type):
        """
        Request to reboot/reset the module

        Returns:
            bool: True if the request has been successful, False if not
        """
        return False

    def set_admin_state(self, up):
        """
        Request to keep the module in administratively up/down state

        Returns:
            bool: True if the request has been successful, False if not
        """
        return False

    def get_platform_type(self):
        """
        Retrieves the type of the card

        Returns:
            string: card-type
        """
        if self.get_type() == self.MODULE_TYPE_SUPERVISOR:
            return platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_CONTROL
        elif self.get_type() == self.MODULE_TYPE_LINE:
            return platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_LINE
        elif self.get_type() == self.MODULE_TYPE_FABRIC:
            return platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_FABRIC
        else:
            return 'Unknown card-type'

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
        if self.get_oper_status() == ModuleBase.MODULE_STATUS_EMPTY:
            return 0.0
        return self.max_consumed_power

    def get_midplane_ip(self):
        """
        Retrieves the midplane IP-address of the module in a modular chassis
        """
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return self.midplane_ip
        ret, response = nokia_common.try_grpc(stub.GetMidplaneIP,
                                              platform_ndk_pb2.ReqModuleInfoPb(hw_slot=self.get_slot()))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return self.midplane_ip

        self.midplane_ip = response.midplane_ip
        return response.midplane_ip

    def is_midplane_reachable(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.IsMidplaneReachable,
                                              platform_ndk_pb2.ReqModuleInfoPb(hw_slot=self.get_slot()))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return False
        return response.midplane_status
