"""
    NOKIA 7250 IXR-X

    Module contains an implementation of SONiC Platform Base API and
    provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
    the platform
"""

try:
    import os
    import subprocess
    import ntpath
    import fcntl
    import ctypes
    import time
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

COMPONENT_NUM = 3
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
SYSFS_DIR = [" ",
             "/sys/bus/pci/devices/0000:01:00.0/",
             "/sys/bus/pci/devices/0000:05:00.0/"]

class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "Basic Input/Output System"],
        ["CpuCtlFpga", "Used for managing CPM"],
        ["IoCtlFpga", "Used for managing IMM"]]

    BIOS_UPDATE_COMMAND = ['./afulnx_64', '', '/P', '/B', '/N', '/K']
    FPGA_UPDATE_COMMAND = ['', '-c', '1', '-p', '', '']

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.sysfs_dir = SYSFS_DIR[self.index]

    def _get_command_result(self, cmdline):
        try:
            proc = subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            result = stdout.rstrip('\n')
        except OSError:
            result = None

        return result

    def _get_cpld_version(self):
        if self.name == "BIOS":
            return read_sysfs_file(BIOS_VERSION_PATH)
        else:
            return read_sysfs_file(self.sysfs_dir + "code_ver")

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

        # check whether the image file exists
        os.chdir("/tmp")
        if not os.path.isfile(image_name):
            print(f"ERROR: the image {image_name} doesn't exist in /tmp")
            return False

        if self.name == "BIOS":
            # check whether the BIOS upgrade tool exists
            if not os.path.isfile('/tmp/afulnx_64'):
                print("ERROR: the BIOS upgrade tool /tmp/afulnx_64 doesn't exist ")
                return False
            self.BIOS_UPDATE_COMMAND[1] = image_name
            try:
                subprocess.run(self.BIOS_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade BIOS: rc={e.returncode}")
            print("\nBIOS upgraded!\n")
            return True

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

