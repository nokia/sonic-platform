"""
    Module contains an implementation of SONiC Platform Base API and
    provides the platform information
"""

try:
    import os
    import sys
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from .fan_drawer import RealDrawer
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
    from sonic_platform.sfp_event import SfpEvent
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e


QSFP_PORT_NUM = 36
QSFP_DD_START = 28
PORT_START    =  1
PORT_END      = 36

CPUPLD_DIR = "/sys/bus/i2c/devices/1-0065/"
CPLD1_DIR  = "/sys/bus/i2c/devices/10-0060/"

# Device counts
FAN_DRAWERS      = 6
FANS_PER_DRAWER  = 2
PSU_NUM          = 2
THERMAL_NUM      = 7
MAX_D4_COMPONENT = 6
MAX_SELECT_DELAY = 10

sonic_logger = logger.Logger("chassis")
sonic_logger.set_min_log_priority_info()

class Chassis(ChassisBase):
    """
    Nokia platform-specific Chassis class
        customized for the 7220 D4-36D platform.
    """

    def __init__(self):
        ChassisBase.__init__(self)
        self.system_led_supported_color = ['green', 'blue', 'green_blink',
                                            'amber', 'off']

        self._watchdog = None
        self.sfp_event = None
        self.max_select_event_returned = None

        self.__initialize_eeprom()
        self.__initialize_fan()
        self.__initialize_psu()
        self.__initialize_sfp()
        self.__initialize_thermals()
        self.__initialize_components()

    def __initialize_eeprom(self):
        self._eeprom = Eeprom(False, 0, False, 0)

    def __initialize_fan(self):
        for drawer_index in range(FAN_DRAWERS):
            fandrawer = RealDrawer(drawer_index)
            self._fan_drawer_list.append(fandrawer)
            for fan_index in range(FANS_PER_DRAWER):
                fan = Fan(fan_index, drawer_index)
                fandrawer._fan_list.append(fan)
                self._fan_list.append(fan)

    def __initialize_psu(self):
        for index in range(0, PSU_NUM):
            psu = Psu(index)
            self._psu_list.append(psu)

    def __initialize_sfp(self):
        for index in range(PORT_START, PORT_START + QSFP_PORT_NUM):
            sfp_type = "QSFP28" if index <= QSFP_DD_START else "QSFPDD"
            eeprom_path = f"/sys/bus/i2c/devices/{24+index}-0050/eeprom"
            if not os.path.exists(eeprom_path):
                sonic_logger.log_info(f"path {eeprom_path} does not exist")
            sfp_node = Sfp(index, sfp_type, eeprom_path)
            self._sfp_list.append(sfp_node)

            self.sfp_event_initialized = False

    def __initialize_thermals(self):
        for index in range(0, THERMAL_NUM):
            thermal = Thermal(index)
            self._thermal_list.append(thermal)


    def __initialize_components(self):
        for index in range(0, MAX_D4_COMPONENT):
            component = Component(index)
            self._component_list.append(component)

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of physical SFP ports in a
            chassis starting from 1.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            # The index will start from 1
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write(f"SFP index {index} out of range (1-{len(self._sfp_list)})\n")
        return sfp

    def get_chassis_fan_type(self):
        """
        Retrieves the fan type (B2F, F2B) that currently is loaded into the chassis.
        All fans should be of same type, i.e., either B2F or F2B.

        Returns:
            Airflow direction: FAN_DIRECTION_INTAKE for F2B,
                               FAN_DIRECTION_EXHAUST for B2F,
                               o/w, empty string "".
        """
        fan_type = set()
        for fan in self._fan_list:
            fan_type.add(fan.get_direction())

        return list(fan_type)[0] if len(fan_type) == 1 else ''

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - True if call successful, False if not;
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
        """
        # Initialize SFP event first
        if not self.sfp_event_initialized:
            self.sfp_event = SfpEvent()
            self.sfp_event.initialize()
            self.max_select_event_returned = PORT_END
            self.sfp_event_initialized = True

        wait_for_ever = timeout == 0
        port_dict = {}

        if wait_for_ever:
            # xcrvd will call this monitor loop in the "SYSTEM_READY" state
            # sonic_logger.log_info(" wait_for_ever get_change_event %d" % timeout)
            timeout = MAX_SELECT_DELAY
            while True:
                status = self.sfp_event.check_sfp_status(port_dict, timeout)
                if port_dict:
                    break
        else:
            # At boot up and in "INIT" state call from xrcvd will have timeout
            # value return true without change after timeout and will
            # transition to "SYSTEM_READY"
            status = self.sfp_event.check_sfp_status(port_dict, timeout)

        if status:
            return True, {'sfp': port_dict}
        return True, {'sfp': {}}

    def get_num_psus(self):
        """
        Retrieves the num of the psus
        Returns:
            int: The num of the psus
        """
        return PSU_NUM

    def get_name(self):
        """
        Retrieves the name of the chassis
        Returns:
            string: The name of the chassis
        """
        return self._eeprom.modelstr()

    def get_presence(self):
        """
        Retrieves the presence of the chassis
        Returns:
            bool: True if chassis is present, False if not
        """
        return True

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.part_number_str()

    def get_serial(self):
        """
        Retrieves the serial number of the chassis
        Returns:
            string: Serial number of chassis
        """
        return self._eeprom.serial_number_str()

    def get_status(self):
        """
        Retrieves the operational status of the chassis
        Returns:
            bool: A boolean value, True if chassis is operating properly
            False if not
        """
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self._eeprom.base_mac_addr()

    def get_service_tag(self):
        """
        Retrieves the Service Tag of the chassis
        Returns:
            string: Service Tag of chassis
        """
        return self._eeprom.service_tag_str()

    def get_revision(self):
        """
        Retrieves the hardware revision of the chassis

        Returns:
            string: Revision value of chassis
        """
        result = read_sysfs_file(CPLD1_DIR + "pcb_ver")
        return result[4:]

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the
        chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their
            corresponding values.
        """
        return self._eeprom.system_eeprom_info()

    def get_thermal_manager(self):
        """
        Get thermal manager

        Returns:
            ThermalManager
        """
        from .thermal_manager import ThermalManager
        return ThermalManager

    def initizalize_system_led(self):
        """
        Initizalize system led

        Returns:
        bool: True if it is successful.
        """
        return True

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """

        result = read_sysfs_file(CPUPLD_DIR + "reset_cause")

        if (int(result, 16) & 0x20) >> 5 == 1:
            return (self.REBOOT_CAUSE_WATCHDOG, "TCO_WDT")

        if (int(result, 16) & 0x10) >> 4 == 1:
            return (self.REBOOT_CAUSE_HARDWARE_BUTTON, "Push button")

        if (int(result, 16) & 0x08) >> 3 == 1:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Cold Reset")

        if (int(result, 16) & 0x04) >> 2 == 1:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Manu Reset")

        if (int(result, 16) & 0x01) == 1:
            return (self.REBOOT_CAUSE_WATCHDOG, "CPU_WDT")

        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device

        Note:
            We overload this method to ensure that watchdog is only initialized
            when it is referenced. Currently, only one daemon can open the
            watchdog. To initialize watchdog in the constructor causes multiple
            daemon try opening watchdog when loading and constructing a chassis
            object and fail. By doing so we can eliminate that risk.
        """
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import WatchdogImplBase
                watchdog_device_path = "/dev/watchdog0"
                self._watchdog = WatchdogImplBase(watchdog_device_path)
        except ImportError as e:
            sonic_logger.log_warning(f"Fail to load watchdog {repr(e)}")

        return self._watchdog

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent
        cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0',
        then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if
            cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def set_status_led(self, color):
        """
        Sets the state of the system LED

        Args:
            color: A string representing the color with which to set the
                   system LED

        Returns:
            bool: True if system LED state is set successfully, False if not
        """
        try:
            reversed_led_support = list(reversed(self.system_led_supported_color))
            idx = reversed_led_support.index(color)
            if idx == 0:
                write_sysfs_file(CPLD1_DIR + 'sys_led', '0')
            else:
                write_sysfs_file(CPLD1_DIR + 'sys_led', str(2**(idx-1)))
            return True
        except ValueError:
            return False

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        result = read_sysfs_file(CPLD1_DIR + 'sys_led')
        result = int(result, 16)
        if result == 0:
            return 'off'

        for shift in list(range(0, 4)):
            if result & (0x8 >> shift):
                return self.system_led_supported_color[shift]
        return 'N/A'