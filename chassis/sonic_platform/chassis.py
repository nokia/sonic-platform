# Name: chassis.py, version: 1.0
#
# Description: Module contains the definitions of SONiC platform APIs
# which provide the chassis specific details
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform_base.module_base import ModuleBase
    from sonic_platform.module import Module
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.fan_drawer import FanDrawer
    from sonic_platform.fan import Fan
    from sonic_platform.component import Component
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common.logger import Logger
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
logger = Logger("Chassis")

# Support for vodka only here.  Need to modify this logic later to discern actual
# number of ASICs and ports per prior to next platform
NUM_SFP = 36


class Chassis(ChassisBase):
    """
    NOKIA IXR7250 Platform-specific Chassis class
    """

    REBOOT_CAUSE_DICT = {
        'powerloss': ChassisBase.REBOOT_CAUSE_POWER_LOSS,
        'overtemp': ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER,
        'reboot': ChassisBase.REBOOT_CAUSE_NON_HARDWARE,
        'watchdog': ChassisBase.REBOOT_CAUSE_WATCHDOG,
        'under-voltage': ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER,
        'over-voltage': ChassisBase.REBOOT_CAUSE_HARDWARE_OTHER,
    }

    def __init__(self):
        ChassisBase.__init__(self)

        # logger.set_min_log_priority_info()

        # Chassis specific slot numbering
        self.is_chassis_modular = nokia_common.is_chassis_modular()
        self.cpm_instance = nokia_common._get_cpm_slot()
        self.my_instance = nokia_common._get_my_slot()
        self.is_cpm = nokia_common.is_cpm()

        # Create a GRPC channel
        self.chassis_stub = None
        self.fan_stub = None
        self.thermal_stub = None
        self.psu_stub = None
        self.firmware_stub = None

        # Get maximum power consumed by each module like cards, fan-trays etc
        self._get_modules_consumed_power()

        # Module list
        self.get_module_list()

        # PSU list
        self._get_psu_list()

        # FAN list
        self._get_fantray_list()

        # Thermal list
        self.get_thermal_list()

        # Component List
        self._get_component_list()

        # SFP
        self.sfp_module_initialized = False
        self.sfp_event_initialized = False

        # Watchdog
        if self._watchdog is None:
            self._watchdog = Watchdog("dog")
            logger.log_info('HW Watchdog initialized')

        # system eeprom
        self._eeprom = Eeprom()

    def get_presence(self):
        module = self._get_my_module()
        if module is not None:
            return module.get_presence()
        return False

    def get_status(self):
        module = self._get_my_module()
        if module is not None:
            return module.get_status()
        return False

    def get_reboot_cause(self):
        unknown = (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, None)
        # if reboot cause is non-hardware, return NON_HARDWARE
        return unknown

    def _get_my_module(self):
        module = None
        my_slot = self.get_my_slot()
        supervisor_slot = self.get_supervisor_slot()
        if supervisor_slot == my_slot:
            index = 0
        else:
            index = self.get_module_index(ModuleBase.MODULE_TYPE_LINE+str(my_slot-1))

        module = self.get_module(index)
        return module

    def get_status_led(self):
        color = Chassis.STATUS_LED_COLOR_OFF
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_BOARD_STATUS
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

    def set_status_led(self, color):
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_BOARD_STATUS
        _led_info = nokia_common.led_color_to_info(color)

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.SetLed,
                                              platform_ndk_pb2.ReqLedInfoPb(led_type=led_type, led_info=_led_info))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return False
        return True

    def is_modular_chassis(self):
        return self.is_chassis_modular

    def get_supervisor_slot(self):
        return self.cpm_instance

    def get_my_slot(self):
        return self.my_instance

    def get_module_list(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return
        ret, response = nokia_common.try_grpc(stub.GetChassisProperties,
                                              platform_ndk_pb2.ReqModuleInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return

        if self.is_modular_chassis() and not self.is_cpm:
            index = 0
            supervisor = Module(index,
                                ModuleBase.MODULE_TYPE_SUPERVISOR+str(index),
                                ModuleBase.MODULE_TYPE_SUPERVISOR,
                                self.get_supervisor_slot(), self.chassis_stub)
            supervisor.set_maximum_consumed_power(self.supervisor_power)

            index = 1
            module = Module(index,
                            ModuleBase.MODULE_TYPE_LINE+str(self.get_my_slot()-1),
                            ModuleBase.MODULE_TYPE_LINE,
                            self.get_my_slot(), self.chassis_stub)
            module.set_maximum_consumed_power(self.line_card_power)

            self._module_list.append(supervisor)
            self._module_list.append(module)
            logger.log_info('Not control card. Adding self into module list')
            return

        if self.is_modular_chassis() and self.is_cpm:
            for property_index in range(len(response.chassis_property.hw_property)):
                hw_property = response.chassis_property.hw_property[property_index]

                if hw_property.module_type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_CONTROL:
                    # Single CPM is supported in chassis
                    num_control_cards = 1
                    for j in range(num_control_cards):
                        module = Module(property_index,
                                        ModuleBase.MODULE_TYPE_SUPERVISOR+str(j),
                                        ModuleBase.MODULE_TYPE_SUPERVISOR,
                                        self.get_supervisor_slot(),
                                        self.chassis_stub)
                        module.set_maximum_consumed_power(self.supervisor_power)
                        self._module_list.append(module)

                elif hw_property.module_type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_LINE:
                    for j in range(hw_property.max_num):
                        module = Module(property_index,
                                        ModuleBase.MODULE_TYPE_LINE+str(j),
                                        ModuleBase.MODULE_TYPE_LINE,
                                        hw_property.slot[j],
                                        self.chassis_stub)
                        module.set_maximum_consumed_power(self.line_card_power)
                        self._module_list.append(module)

                elif hw_property.module_type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_FABRIC:
                    for j in range(hw_property.max_num):
                        module = Module(property_index,
                                        ModuleBase.MODULE_TYPE_FABRIC+str(j),
                                        ModuleBase.MODULE_TYPE_FABRIC,
                                        hw_property.slot[j],
                                        self.chassis_stub)
                        module.set_maximum_consumed_power(self.fabric_card_power)
                        self._module_list.append(module)

    def get_module_index(self, module_name):
        if not self.is_modular_chassis():
            return -1

        # For IMM on chassis, return supervisor-index as 0 and self index as 1
        if not self.is_cpm:
            if module_name.startswith(ModuleBase.MODULE_TYPE_SUPERVISOR):
                return 0
            else:
                return 1

        # For CPM on chassis
        module_index = -1
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return module_index
        ret, response = nokia_common.try_grpc(stub.GetChassisProperties,
                                              platform_ndk_pb2.ReqModuleInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return module_index

        for property_index in range(len(response.chassis_property.hw_property)):
            hw_property = response.chassis_property.hw_property[property_index]

            if hw_property.module_type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_CONTROL:
                # Single CPM is supported in chassis
                num_control_cards = 1
            elif hw_property.module_type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_LINE:
                num_line_cards = hw_property.max_num
            # elif hw_property.module_type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_FABRIC:
            #    num_fabric_cards = hw_property.max_num

        if module_name.startswith(ModuleBase.MODULE_TYPE_SUPERVISOR):
            module_index = 0
        elif module_name.startswith(ModuleBase.MODULE_TYPE_LINE):
            import re
            parse_nums = re.findall(r'\d+', module_name)
            module_number = int(parse_nums[0])
            module_index = num_control_cards + int(module_number)
        elif module_name.startswith(ModuleBase.MODULE_TYPE_FABRIC):
            import re
            parse_nums = re.findall(r'\d+', module_name)
            module_number = int(parse_nums[0])
            module_index = num_control_cards + num_line_cards + int(module_number)

        return module_index

    # PSU and power related
    def _get_psu_list(self):
        if not self.is_cpm:
            return []

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return
        ret, response = nokia_common.try_grpc(stub.GetPsuNum,
                                              platform_ndk_pb2.ReqPsuInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return

        self.num_psus = response.num_psus
        for i in range(self.num_psus):
            psu = Psu(i, self.psu_stub)
            self._psu_list.append(psu)
        return self._psu_list

    def _get_modules_consumed_power(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return
        ret, response = nokia_common.try_grpc(stub.GetModuleMaxPower,
                                              platform_ndk_pb2.ReqModuleInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return

        i = 0
        while i < len(response.power_info.module_power):
            module_info = response.power_info.module_power[i]
            i = i + 1
            type = module_info.module_type
            if type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_CONTROL:
                self.supervisor_power = module_info.module_maxpower
            elif type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_LINE:
                self.line_card_power = module_info.module_maxpower
            elif type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_FABRIC:
                self.fabric_card_power = module_info.module_maxpower
            elif type == platform_ndk_pb2.HwModuleType.HW_MODULE_TYPE_FANTRAY:
                self.fantray_power = module_info.module_maxpower

    # Fan related
    def _get_fantray_list(self):
        if not self.is_cpm:
            return []

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FAN_SERVICE)
        if not channel or not stub:
            return
        ret, response = nokia_common.try_grpc(stub.GetFanNum,
                                              platform_ndk_pb2.ReqFanTrayOpsPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return

        self.num_fantrays = response.fan_nums.num_fantrays

        fan_index = 0
        for drawer_index in range(self.num_fantrays):
            fan_drawer = FanDrawer(drawer_index)
            fan_drawer.set_maximum_consumed_power(self.fantray_power)
            fan = Fan(fan_index, drawer_index, False, self.fan_stub)
            fan_drawer._fan_list.append(fan)
            fan_index = fan_index + 1
            self._fan_drawer_list.append(fan_drawer)

        return self._fan_drawer_list

    def allow_fan_platform_override(self):
        from os import path
        if path.exists(nokia_common.NOKIA_ALLOW_FAN_OVERRIDE_FILE):
            return True
        return False

    # Thermal related
    def get_thermal_list(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
        if not channel or not stub:
            return
        ret, response = nokia_common.try_grpc(stub.GetThermalDevicesInfo,
                                              platform_ndk_pb2.ReqTempParamsPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return

        all_temp_devices = response.temp_devices
        self.num_thermals = len(all_temp_devices.temp_device)
        i = 0

        # Empty previous list
        if len(self._thermal_list) != self.num_thermals:
            del self._thermal_list[:]

            for i in range(self.num_thermals):
                temp_device_ = all_temp_devices.temp_device[i]
                thermal = Thermal(i, temp_device_.device_idx, temp_device_.sensor_name, self.thermal_stub)
                self._thermal_list.append(thermal)

    def get_all_thermals(self):
        self.get_thermal_list()
        return self._thermal_list

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    def _get_component_list(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FIRMWARE_SERVICE)
        if not channel or not stub:
            return
        ret, response = nokia_common.try_grpc(stub.HwFirmwareGetComponents,
                                              platform_ndk_pb2.ReqHwFirmwareInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return

        for i in range(len(response.firmware_info.component)):
            hw_dev = response.firmware_info.component[i]
            component = Component(i, hw_dev.dev_type, hw_dev.dev_name,
                                  hw_dev.dev_desc, self.firmware_stub)
            self._component_list.append(component)

    # SFP operations below
    def initialize_sfp(self):
        from sonic_platform.sfp import Sfp

        if not nokia_common.is_cpm():
            if self.sfp_module_initialized:
                logger.log_error("SFPs are already initialized! stub {}".format(self.sfp_stub))
                return

            self.sfp_stub = None
            for index in range(1, NUM_SFP+1):
                logger.log_info("Creating SFP '%d'" % index)
                # default to QSFP-DD type for the moment. Type gets set dynamically when module is read
                sfp = Sfp(index, 'QSFP-DD', self.sfp_stub)
                self._sfp_list.append(sfp)
                # force 1st read to dynamically set type and detect dom capability
                sfp.get_transceiver_info()
                sfp._dom_capability_detect()
            self.sfp_module_initialized = True
            logger.log_info("SFPs are now initialized... stub {}".format(self.sfp_stub))

    def get_change_event(self, timeout=0):
        # logger.log_error("Get-change-event with thread-{} start ".format(
        #    str(os.getpid())+str(threading.current_thread().ident)))
        if not self.sfp_event_initialized:
            from sonic_platform.sfp_event import sfp_event
            # use the same stub as direct sfp ops does
            self.sfp_event_stub = self.sfp_stub

            logger.log_info("Initializing sfp_event with num {} and stub {} : sfp stub {}".format(
                NUM_SFP, self.sfp_event_stub, self.sfp_stub))
            self.sfp_event_list = sfp_event(NUM_SFP, self.sfp_event_stub)
            self.sfp_event_list.initialize()
            self.sfp_event_initialized = True

        timeout = 0
        port_dict = {}
        self.sfp_event_list.check_sfp_status(port_dict, timeout)

        return True, {'sfp': port_dict}

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this chassis
        Returns:
            An integer, the number of sfps available on this chassis
        """
        if not self.sfp_module_initialized:
            self.initialize_sfp()

        return len(self._sfp_list)

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this chassis
        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        if not self.sfp_module_initialized:
            self.initialize_sfp()

        return self._sfp_list

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 1.
            For example, 1 for Ethernet0, 2 for Ethernet4 and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None
        if not self.sfp_module_initialized:
            self.initialize_sfp()

        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            logger.log_error("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))

        return sfp

    # System eeprom below
    def get_name(self):
        """
        Retrieves the hardware product name for the chassis

        Returns:
            A string containing the hardware product name for this chassis.
        """
        return self._eeprom.get_product_name()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.get_base_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial_number()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self._eeprom.get_serial_number()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.get_part_number()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom.get_system_eeprom_info()

    def get_eeprom(self):
        """
        Retrieves the Sys Eeprom instance for the chassis.
        Returns :
            The instance of the Sys Eeprom
        """
        return self._eeprom

    # Midplane
    def init_midplane_switch(self):
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

    # Test suitest
    def test_suite_chassis(self):
        print("Starting Chassis UT")
        from sonic_platform.test import test_chassis
        test_chassis.run_all(self)

    def test_suite_psu(self):
        print("Starting PSUs UT")
        from sonic_platform.test import test_psu
        test_psu.run_all(self._psu_list)

    # UT related helper apis
    def empty_fan_drawer_list(self):
        for fantray in self.get_all_fan_drawers():
            del fantray._fan_list[:]
        del self._fan_drawer_list[:]

    def empty_fan_list(self, fantray_idx):
        fantray = self.get_fan_drawer(fantray_idx)
        del fantray._fan_list[:]
