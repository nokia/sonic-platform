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
PORT_NUM = 64
PORT_END = 66
PORT_I2C_START = 5
MAX_SELECT_DELAY = 10

# Device counts
FAN_DRAWERS = 8
FANS_PER_DRAWER = 2
PSU_NUM = 4
THERMAL_NUM = 17
COMPONENT_NUM = 7

CPLD_DIR = "/sys/bus/i2c/devices/73-0061/"
SYSFPGA_DIR  = "/sys/bus/i2c/devices/1-0060/"

SYSLOG_IDENTIFIER = "chassis"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class Chassis(ChassisBase):
    """
    Nokia platform-specific Chassis class
        customized for the 7220 H6-64 platform.
    """
    def __init__(self):
        ChassisBase.__init__(self)
        self.system_led_supported_color = ['green', 'blue', 'green_blink',
                                            'amber', 'off']
        
        # Verify optoe driver OSFP eeprom devices were enumerated and exist
        # then create the sfp nodes
        eeprom_path = "/sys/bus/i2c/devices/{}-0050/eeprom"
        for index in range(PORT_START, PORT_START + PORT_END):
            port_i2c_map = PORT_I2C_START + index - 1
            port_eeprom_path = eeprom_path.format(port_i2c_map)
            if not os.path.exists(port_eeprom_path):
                sonic_logger.log_info(f"path {port_eeprom_path} didnt exist")
            if index <= PORT_NUM:
                sfp_node = Sfp(index, 'OSFP', port_eeprom_path, port_i2c_map)
            elif index > PORT_NUM and index <= PORT_END:
                sfp_node = Sfp(index, 'SFP28', port_eeprom_path, port_i2c_map)
            self._sfp_list.append(sfp_node)

        self.sfp_event_initialized = False

        # Instantiate system eeprom object
        self._eeprom = Eeprom(False, 0, False, 0)

        self._watchdog = None
        self.sfp_event = None
        self.max_select_event_returned = None

        # Construct lists fans, power supplies, thermals & components
        for i in range(THERMAL_NUM):
            thermal = Thermal(i, self._sfp_list)
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
        result = read_sysfs_file(SYSFPGA_DIR + "reset_cause")

        if (int(result, 16) & 0x10) >> 4 == 1:
            return (self.REBOOT_CAUSE_WATCHDOG, "CPU_WD")

        if (int(result, 16) & 0x01) == 1:
            return (self.REBOOT_CAUSE_WATCHDOG, "EC_WD")

        if (int(result, 16) & 0x02) >> 1 == 1:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "CPU Over Heat")

        if (int(result, 16) & 0x08) >> 3 == 1:
            return (self.REBOOT_CAUSE_HARDWARE_OTHER, "Power Cycle")
        
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
            'blue': '0x3',
            'green': '0x5',
            'amber': '0x6',
            'off': '0x7',
            'green_blink': '0xf'
        }

        if color not in self.system_led_supported_color:
            return False

        value = color_to_value.get(color)
        if value is None:
            return False

        write_sysfs_file(CPLD_DIR + 'led_sys', value)
        return True

    def get_status_led(self):
        """
        Gets the state of the system LED

        Returns:
            A string, one of the valid LED color strings which could be vendor
            specified.
        """
        result = read_sysfs_file(CPLD_DIR + 'led_sys')
        val = int(result, 16)
        if (val & 0x8) == 0x8:
            return self.system_led_supported_color[2]
        if val == 0x3:
            return self.system_led_supported_color[1]
        if val == 0x5:
            return self.system_led_supported_color[0]
        if val == 0x6:
            return self.system_led_supported_color[3]
        if val == 0x7:
            return self.system_led_supported_color[4]
        return 'N/A'
