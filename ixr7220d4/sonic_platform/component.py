"""
    NOKIA IXR7220-D4

    Module contains an implementation of SONiC Platform Base API and
    provides the Components' (e.g., BIOS, CPLD, etc.) available in
    the platform
"""

try:
    import os
    import subprocess
    import ntpath
    import time
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

SYSFS_PATH = ["/sys/class/dmi/id/bios_version",
              "/sys/bus/i2c/devices/1-0065/cpld_ver",
              "/sys/bus/i2c/devices/10-0060/cpld_ver",
              "/sys/bus/i2c/devices/10-0062/cpld_ver",
              "/sys/bus/i2c/devices/10-0064/cpld_ver",
              "/sys/bus/i2c/devices/14-0066/fan_version"]

class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "Basic Input/Output System"],
        ["CPUPLD", "Used for managing CPU board "],
        ["SWPLD1", "Used for managing QSFP28, QSFPDD, and System LEDs "],
        ["SWPLD2", "Used for managing QSFP28 19-28 and QSFPDD 29-36 port status, module presence and loopback mode "],
        ["SWPLD3", "Used for managing QSFP28 1-18 port status, module presence and loopback mode "],
        ["FANPLD", "Used for managing FAN board "]]

    CPLD_UPDATE_COMMAND = ['./cpldupd_d4', '-u', '', '']
    BIOS_UPDATE_COMMAND = ['./afulnx_64', '', '/P', '/B', '/N', '/K', '/ME', '/RLC:F']

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.sysfs_file = SYSFS_PATH[self.index]


    def _get_cpld_version(self):
        return read_sysfs_file(self.sysfs_file)

    def get_name(self):
        """
        Retrieves the name of the component

        Returns:
            A string containing the name of the component
        """
        return self.name

    def get_model(self):
        """
        Retrieves the part number of the component
        Returns:
            string: Part number of component
        """
        return 'NA'

    def get_serial(self):
        """
        Retrieves the serial number of the component
        Returns:
            string: Serial number of component
        """
        return 'NA'

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
            integer: The 1-based relative physical position in parent
            device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether component is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return False

    def get_description(self):
        """
        Retrieves the description of the component

        Returns:
            A string containing the description of the component
        """
        return self.description

    def get_firmware_version(self):
        """
        Retrieves the firmware version of the component

        Returns:
            A string containing the firmware version of the component
        """
        return self._get_cpld_version()

    def install_firmware(self, image_path):
        """
        Installs firmware to the component

        Args:
            image_path: A string, path to firmware image

        Returns:
            A boolean, True if install was successful, False if not
        """
        image_name = ntpath.basename(image_path)
        print(f"IXR-7220-D4 - Install image {image_name} for component {self.get_name()}")

        # check whether the image file exists
        os.chdir("/tmp")
        if not os.path.isfile(image_name):
            print(f"ERROR: the image {image_name} doesn't exist in /tmp")
            return False

        # check whether the upgrade tool exists
        if self.name == "BIOS":
            if not os.path.isfile('/tmp/afulnx_64'):
                print("ERROR: the BIOS upgrade tool /tmp/afulnx_64 doesn't exist ")
                return False
            self.BIOS_UPDATE_COMMAND[1] = image_name
            UPDATE_CMD = self.BIOS_UPDATE_COMMAND
        else:
            if not os.path.isfile('/tmp/cpldupd_d4'):
                print("ERROR: the CPLD upgrade tool /tmp/cpldupd_d4 doesn't exist ")
                return False
            if self.get_name() == "FANPLD":
                write_sysfs_file("/sys/bus/i2c/devices/1-0065/jtag_sel", str(0))
                write_sysfs_file("/sys/bus/i2c/devices/10-0060/jtag_sw_sel", str(1))
                write_sysfs_file("/sys/bus/i2c/devices/10-0060/jtag_sw_oe", str(0))
                self.CPLD_UPDATE_COMMAND[2] = "FAN_CPLD"
            elif self.get_name() == "CPUPLD":
                self.CPLD_UPDATE_COMMAND[2] = "CPU_CPLD"
            elif self.get_name() in ("SWPLD1", "SWPLD2", "SWPLD3"):
                write_sysfs_file("/sys/bus/i2c/devices/1-0065/jtag_sel", str(0))
                write_sysfs_file("/sys/bus/i2c/devices/10-0060/jtag_sw_sel", str(0))
                write_sysfs_file("/sys/bus/i2c/devices/10-0060/jtag_sw_oe", str(0))
                self.CPLD_UPDATE_COMMAND[2] = "MAIN_CPLD"
            else:
                print(f"ERROR: component with {self.get_name()} name was not found")
                return False
            self.CPLD_UPDATE_COMMAND[3] = image_name
            UPDATE_CMD = self.CPLD_UPDATE_COMMAND

        try:
            subprocess.run(UPDATE_CMD, stderr=subprocess.STDOUT)
            print(f"\n{self.get_name()} upgraded!\n")
            print("!!!The system will power cycle in 10 sec!!!")
            time.sleep(10)
            write_sysfs_file("/sys/bus/i2c/devices/10-0060/psu2_power_ok", str(1))
            write_sysfs_file("/sys/bus/i2c/devices/10-0060/psu1_power_ok", str(1))
        except subprocess.CalledProcessError as e:
            print(f"ERROR: Failed to upgrade {self.get_name()}: rc={e.returncode}")

        if self.get_name() in ("FANPLD", "SWPLD1", "SWPLD2", "SWPLD3"):
            write_sysfs_file("/sys/bus/i2c/devices/10-0060/jtag_sw_sel", str(0))
            write_sysfs_file("/sys/bus/i2c/devices/10-0060/jtag_sw_oe", str(1))
            write_sysfs_file("/sys/bus/i2c/devices/1-0065/jtag_sel", str(1))

        return True

    def update_firmware(self, _image_path):
        """
        Updates firmware of the component

        This API performs firmware update:
        it assumes firmware installation and loading in a single call.
        In case platform component requires some extra steps (apart from calling Low Level Utility)
        to load the installed firmware (e.g, reboot, power cycle, etc.)
        - this will be done automatically by API

        Args:
            image_path: A string, path to firmware image

        Raises:
            RuntimeError: update failed
        """
        return False

    def get_available_firmware_version(self, _image_path):
        """
        Retrieves the available firmware version of the component

        Note: the firmware version will be read from image

        Args:
            image_path: A string, path to firmware image

        Returns:
            A string containing the available firmware version of the component
        """
        return "N/A"
