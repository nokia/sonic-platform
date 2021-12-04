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

        self.revision = 'Unknown'
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

        # Module list
        # self._get_module_list()
        self._module_list = []
        self.module_module_initialized = False

        # PSU list
        # self._get_psu_list()
        self._psu_list = []
        self.psu_module_initialized = False

        # FAN list
        # self._get_fantray_list()
        self._fan_drawer_list = []
        self.fan_drawer_module_initialized = False

        # Thermal list
        # self._get_thermal_list()

        # Component List
        # self._get_component_list()
        self._component_list = []
        self.component_module_initialized = False

        # SFP
        self.sfp_module_initialized = False
        self.sfp_event_initialized = False

        # Watchdog
        if self._watchdog is None:
            self._watchdog = Watchdog("dog")
            logger.log_info('HW Watchdog initialized')

        # system eeprom
        self._eeprom = None

    def init_all(self):
        self._get_module_list()
        self._get_psu_list()
        self._get_fantray_list()
        self._get_thermal_list()
        self._get_component_list()
        self.get_eeprom()

    def get_presence(self):
        self.get_all_modules()
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

    # API used by system-health monitoring
    def initizalize_system_led(self):
        return

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

    def _get_module_list(self):
        if self.module_module_initialized:
            return self._module_list

        # Get maximum power consumed by each module like cards, fan-trays etc
        self._get_modules_consumed_power()

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
            self.module_module_initialized = True
            return self._module_list

        # Only on CPM
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
        if not channel or not stub:
            return []
        ret, response = nokia_common.try_grpc(stub.GetChassisProperties,
                                              platform_ndk_pb2.ReqModuleInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return []

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
                        module = Module(j,
                                        ModuleBase.MODULE_TYPE_FABRIC+str(j),
                                        ModuleBase.MODULE_TYPE_FABRIC,
                                        hw_property.slot[j],
                                        self.chassis_stub)
                        module.set_maximum_consumed_power(self.fabric_card_power)
                        self._module_list.append(module)

        self.module_module_initialized = True
        return self._module_list

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

    def get_num_modules(self):
        """
        Retrieves the number of modules available on this chassis
        Returns:
            An integer, the number of modules available on this chassis
        """
        return (len(self._get_module_list()))

    def get_all_modules(self):
        """
        Retrieves all modules available on this chassis
        Returns:
            A list of objects representing all modules available on this chassis
        """
        return (self._get_module_list())

    def get_module(self, index):
        self._get_module_list()
        return super(Chassis, self).get_module(index)

    # PSU and power related
    def _get_psu_list(self):
        if not self.is_cpm:
            return []

        if self.psu_module_initialized:
            return self._psu_list

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
        if not channel or not stub:
            return []
        ret, response = nokia_common.try_grpc(stub.GetPsuNum,
                                              platform_ndk_pb2.ReqPsuInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return []

        self.num_psus = response.num_psus
        for i in range(self.num_psus):
            psu = Psu(i, self.psu_stub)
            self._psu_list.append(psu)

        self.psu_module_initialized = True
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

    def get_num_psus(self):
        """
        Retrieves the number of psus available on this chassis
        Returns:
            An integer, the number of psus available on this chassis
        """
        return (len(self._get_psu_list()))

    def get_all_psus(self):
        """
        Retrieves all psus available on this chassis
        Returns:
            A list of objects representing all psus available on this chassis
        """
        return (self._get_psu_list())

    # Fan related
    def _get_fantray_list(self):
        if not self.is_cpm:
            return []

        if self.fan_drawer_module_initialized:
            return self._fan_drawer_list

        # Get maximum power consumed by each module like cards, fan-trays etc
        self._get_modules_consumed_power()

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FAN_SERVICE)
        if not channel or not stub:
            return []
        ret, response = nokia_common.try_grpc(stub.GetFanNum,
                                              platform_ndk_pb2.ReqFanTrayOpsPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return []

        self.num_fantrays = response.fan_nums.num_fantrays

        fan_index = 0
        for drawer_index in range(self.num_fantrays):
            fan_drawer = FanDrawer(drawer_index)
            fan_drawer.set_maximum_consumed_power(self.fantray_power)
            fan = Fan(fan_index, drawer_index, False, self.fan_stub)
            fan_drawer._fan_list.append(fan)
            fan_index = fan_index + 1
            self._fan_drawer_list.append(fan_drawer)

        self.fan_drawer_module_initialized = True
        return self._fan_drawer_list

    def allow_fan_platform_override(self):
        from os import path
        if path.exists(nokia_common.NOKIA_ALLOW_FAN_OVERRIDE_FILE):
            return True
        return False

    def get_num_fan_drawers(self):
        """
        Retrieves the number of fan-drawers available on this chassis
        Returns:
            An integer, the number of drawers available on this chassis
        """
        return (len(self._get_fantray_list()))

    def get_all_fan_drawers(self):
        """
        Retrieves all fan-drawers available on this chassis
        Returns:
            A list of objects representing all fan-drawers available on this chassis
        """
        return (self._get_fantray_list())

    # Thermal related
    def _get_thermal_list(self):
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
        if not channel or not stub:
            return []
        ret, response = nokia_common.try_grpc(stub.GetThermalDevicesInfo,
                                              platform_ndk_pb2.ReqTempParamsPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return []

        all_temp_devices = response.temp_devices
        self.num_thermals = len(all_temp_devices.temp_device)
        i = 0

        list_change = False
        # If number of sensors are different
        if len(self._thermal_list) != self.num_thermals:
            list_change = True
        else:
            for i in range(self.num_thermals):
                temp_device_ = all_temp_devices.temp_device[i]
                thermal_name = self._thermal_list[i].get_name()
                if (thermal_name.find(temp_device_.sensor_name) == -1):
                    list_change = True
                    break

        # Empty previous list
        if list_change:
            del self._thermal_list[:]

            for i in range(self.num_thermals):
                temp_device_ = all_temp_devices.temp_device[i]
                thermal = Thermal(i, temp_device_.device_idx, temp_device_.sensor_name,
                                  temp_device_.fanalgo_sensor, self.thermal_stub)
                self._thermal_list.append(thermal)

        return self._thermal_list

    def get_num_thermals(self):
        """
        Retrieves the number of thermals available on this chassis
        Returns:
            An integer, the number of thermals available on this chassis
        """
        return (len(self._get_thermal_list()))

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this chassis
        Returns:
            A list of objects representing all thermals available on this chassis
        """
        return (self._get_thermal_list())

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager

    # Component related
    def _get_component_list(self):
        if self.component_module_initialized:
            return self._component_list

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FIRMWARE_SERVICE)
        if not channel or not stub:
            return []
        ret, response = nokia_common.try_grpc(stub.HwFirmwareGetComponents,
                                              platform_ndk_pb2.ReqHwFirmwareInfoPb())
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return []

        for i in range(len(response.firmware_info.component)):
            hw_dev = response.firmware_info.component[i]
            component = Component(i, hw_dev.dev_type, hw_dev.dev_name,
                                  hw_dev.dev_desc, self.firmware_stub)
            self._component_list.append(component)

        self.component_module_initialized = True
        return self._component_list

    def get_num_components(self):
        """
        Retrieves the number of components available on this chassis
        Returns:
            An integer, the number of components available on this chassis
        """
        return (len(self._get_component_list()))

    def get_all_components(self):
        """
        Retrieves all components available on this chassis
        Returns:
            A list of objects representing all components available on this chassis
        """
        return (self._get_component_list())

    # SFP operations below
    def initialize_sfp(self):
        from sonic_platform.sfp import Sfp

        if not nokia_common.is_cpm():
            if self.sfp_module_initialized:
                logger.log_error("SFPs are already initialized! stub {}".format(self.sfp_stub))
                return

            op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_NORMAL
            channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
            if not channel or not stub:
                logger.log_error("Failure retrieving channel, stub in initialize_sfp")
                return False

            ret, response = nokia_common.try_grpc(stub.GetSfpNumAndType,
                                                  platform_ndk_pb2.ReqSfpOpsPb(type=op_type))
            nokia_common.channel_shutdown(channel)
            if ret is False:
                logger.log_error("Failure on GetSfpNumAndType in initialize_sfp")
                return False

            msg = response.sfp_num_type
            logger.log_info("GetSfpNumAndType: {}".format(msg))
            self.num_sfp = msg.num_ports

            # index 0 is placeholder with no valid entry
            # self._sfp_list.append(None)

            self.sfp_stub = None
            for index in range(1, self.num_sfp+1):
                if index <= msg.type1_hw_port_id_end:
                    sfp_type = msg.type1_port
                elif index <= msg.type2_hw_port_id_end:
                    sfp_type = msg.type2_port
                elif index <= msg.type3_hw_port_id_end:
                    sfp_type = msg.type3_port
                else:
                    sfp_type = platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_INVALID

                if sfp_type == platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_INVALID:
                    logger.log_error("GetSfpNumAndType sfp_type is INVALID at index {}".format(index))
                    # trust msg.num_ports for now and just initialize with QSFPDD type
                    sfp_type = platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_QSFPDD

                sfp = Sfp(index, sfp_type, self.sfp_stub)
                logger.log_debug("Created SFP {} index {} with type {}".format(sfp, index, sfp_type))
                self._sfp_list.append(sfp)
                # force 1st read to dynamically set type and detect dom capability
                sfp.get_transceiver_info()
                sfp._dom_capability_detect()
            self.sfp_module_initialized = True
            logger.log_info("SFPs are now initialized... stub {}".format(self.sfp_stub))
        else:
            self.sfp_module_initialized = True
            logger.log_info("CPM has no SFPs... skipping initialization")

    def get_change_event(self, timeout=0):
        # logger.log_error("Get-change-event with thread-{} start ".format(
        #    str(os.getpid())+str(threading.current_thread().ident)))
        if not self.sfp_event_initialized:
            from sonic_platform.sfp_event import sfp_event
            # use the same stub as direct sfp ops does
            self.sfp_event_stub = self.sfp_stub

            if not self.sfp_module_initialized:
               self.initialize_sfp()

            logger.log_info("Initializing sfp_event with num {} and stub {} : sfp stub {}".format(
                self.num_sfp, self.sfp_event_stub, self.sfp_stub))
            self.sfp_event_list = sfp_event(self.num_sfp, self.sfp_event_stub)
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
        if nokia_common.is_cpm():
            return 0

        if not self.sfp_module_initialized:
            self.initialize_sfp()

        return (len(self._sfp_list))

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
        Retrieves sfp represented by (0-based) index <index>
        Args:
            index: An integer, the index (0-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 0.
            For example, 0 for Ethernet0, 1 for Ethernet4 and so on.
        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None
        if not self.sfp_module_initialized:
            self.initialize_sfp()

        try:
            # The index will start from 0 (though 0 is a placeholder with value None)
            sfp = self._sfp_list[index-1]
        except IndexError:
            logger.log_error("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))

        """
        logger.log_error("get_sfp retrieving index {} is {} and length of _sfp_list {}".format(index,
                          sfp, len(self._sfp_list) - 1))
        """

        return sfp

    # System eeprom below
    def get_name(self):
        """
        Retrieves the hardware product name for the chassis

        Returns:
            A string containing the hardware product name for this chassis.
        """
        return self.get_eeprom().get_product_name()

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self.get_eeprom().get_base_mac()

    def get_serial(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self.get_eeprom().get_serial_number()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        return self.get_eeprom().get_serial_number()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self.get_eeprom().get_part_number()

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self.get_eeprom().get_system_eeprom_info()

    def get_eeprom(self):
        """
        Retrieves the Sys Eeprom instance for the chassis.
        Returns :
            The instance of the Sys Eeprom
        """
        if self._eeprom is None:
            self._eeprom = Eeprom()
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

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.revision

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
