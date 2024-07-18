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

# Port numbers for SFP List Initialization
PORT_START = 1
QSFP_PORT_NUM = 64
PORT_END = 66

UDB_NAME = "/sys/devices/platform/pcie_udb_fpga_device.{}/eeprom"
LDB_NAME = "/sys/devices/platform/pcie_ldb_fpga_device.{}/eeprom"
SCM_PATH = "/sys/bus/i2c/devices/51-0035/"
SYS_LED_PATH = "/sys/devices/platform/sys_fpga/led_sys"
MAX_SELECT_DELAY = 10

# Device counts
FAN_DRAWERS = 4
FANS_PER_DRAWER = 2
PSU_NUM = 2
THERMAL_NUM = 11
COMPONENT_NUM = 13

SYSLOG_IDENTIFIER = "chassis"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class Chassis(ChassisBase):
    """
    Nokia platform-specific Chassis class
        customized for the 7220 H4-64D platform.
    """
    STATUS_LED_COLOR_BLUE = 'blue'
    STATUS_LED_COLOR_GREEN_BLINK = 'green_blink'

    def __init__(self):
        ChassisBase.__init__(self)
        self.system_led_supported_color = ['off', 'amber', 'green', 'blue', 'green_blink']

        # Verify optoe driver QSFP-DD eeprom devices were enumerated and exist
        # then create the sfp nodes
        for index in range(PORT_START, PORT_START + QSFP_PORT_NUM):
            port_i2c_map = index
            if index <= QSFP_PORT_NUM // 2:
                port_eeprom_path = UDB_NAME.format(index - 1)
            else:
                port_eeprom_path = LDB_NAME.format(index - QSFP_PORT_NUM // 2 - 1)
            if not os.path.exists(port_eeprom_path):
                sonic_logger.log_info(f"path {port_eeprom_path} didnt exist")
            sfp_node = Sfp(index, 'QSFPDD', port_eeprom_path, port_i2c_map)
            self._sfp_list.append(sfp_node)

        port_eeprom_path = LDB_NAME.format(32)
        if not os.path.exists(port_eeprom_path):
            sonic_logger.log_info(f"path {port_eeprom_path} didnt exist")
        sfp_node = Sfp(QSFP_PORT_NUM + 1, 'SFP+', port_eeprom_path, 65)
        self._sfp_list.append(sfp_node)

        port_eeprom_path = LDB_NAME.format(33)
        if not os.path.exists(port_eeprom_path):
            sonic_logger.log_info(f"path {port_eeprom_path} didnt exist")
        sfp_node = Sfp(QSFP_PORT_NUM + 2, 'SFP+', port_eeprom_path, 66)
        self._sfp_list.append(sfp_node)
        self.sfp_event_initialized = False
        # Instantiate system eeprom object
        self._eeprom = Eeprom(False, 0, False, 0)

        self._watchdog = None
        self.sfp_event = None
        self.max_select_event_returned = None

        # Construct lists fans, power supplies, thermals & components
        for i in range(THERMAL_NUM):
            thermal = Thermal(i)
            self._thermal_list.append(thermal)

        drawer_num = FAN_DRAWERS
        fan_num_per_drawer = FANS_PER_DRAWER
        drawer_ctor = RealDrawer
        for drawer_index in range(drawer_num):
            drawer = drawer_ctor(drawer_index)
            self._fan_drawer_list.append(drawer)
            for fan_index in range(fan_num_per_drawer):
                fan = Fan(fan_index, drawer_index)
                drawer._fan_list.append(fan)
                self._fan_list.append(fan)

        for i in range(PSU_NUM):
            psu = Psu(i)
            self._psu_list.append(psu)

        for i in range(COMPONENT_NUM):
            component = Component(i)
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
            # xrcvd will call this monitor loop in the "SYSTEM_READY" state
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
        #Revision is always 0 for 7220-IXR-H4-64D
        return str(0)

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
        result = read_sysfs_file(SCM_PATH + "reset_cause")

        if (int(result, 16) & 0x8) >> 3 == 0:
            return (self.REBOOT_CAUSE_WATCHDOG, "COMe_WDT")
        if (int(result, 16) & 0x4) >> 2 == 0:
            return (self.REBOOT_CAUSE_WATCHDOG, "TCO_WDT")
        if (int(result, 16) & 0x2) >> 1 == 0:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "SW Reset")
        if (int(result, 16) & 0x1) == 0:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "COMe Reset")

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
        color_to_value = {
            'off': '0',
            'green': '8',
            'amber': '1',
            'green_blink': '2',
            'blue': '4'
        }

        if color not in self.system_led_supported_color:
            return False

        value = color_to_value.get(color)
        if value is None:
            return False

        write_sysfs_file(SYS_LED_PATH, value)
        return True

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        result = read_sysfs_file(SYS_LED_PATH)

        if (int(result, 10) & 0x8) == 0x8:
            return self.STATUS_LED_COLOR_GREEN
        if (int(result, 10) & 0x4) == 0x4:
            return self.STATUS_LED_COLOR_BLUE
        if (int(result, 10) & 0x2) == 0x2:
            return self.STATUS_LED_COLOR_GREEN_BLINK
        if (int(result, 10) & 0x1) == 0x1:
            return self.STATUS_LED_COLOR_AMBER
        return self.STATUS_LED_COLOR_OFF
