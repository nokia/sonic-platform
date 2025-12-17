"""
    NOKIA IXR7220 H4-32D
   
    Module contains an implementation of SONiC Platform Base API and
    provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
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


MAX_H4_32D_COMPONENT = 5
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
FPGA_CODE_REV0 = "/sys/kernel/delta_fpga/sys-fpga-code-revision"
CPLD_DIR = [" ",
            "/sys/bus/i2c/devices/1-0031/",
            " ",
            "/sys/bus/i2c/devices/14-0034/",
            "/sys/bus/i2c/devices/14-0035/"]

class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "Basic Input/Output System"],
        ["CPUPLD", "Used for managing CPU board "],
        ["SysFPGA", "Used for managing BCM chip, SFPs, PSUs and LEDs "],
        ["SWPLD2", "Used for managing QSFP-DD 1-16 "],
        ["SWPLD3", "Used for managing QSFP-DD 17-32, SFP+ "] ]

    CPLD_UPDATE_COMMAND = ['./vme_h4_32d', '', '']
    FPGA_UPDATE_COMMAND = ['./fpga_upg_tool', '-c', '1', '-p', '', '']
    BIOS_UPDATE_COMMAND = ['./afulnx_64', '', '/P', '/B', '/N', '/K']

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.cpld_dir = CPLD_DIR[self.index]

    def _get_command_result(self, cmdline):
        try:
            with subprocess.Popen(cmdline.split(), stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT) as proc:
                _, _ = proc.communicate()
                stdout = proc.communicate()[0]
                proc.wait()
                result = stdout.rstrip('\n')
        except OSError:
            result = None

        return result

    def _get_cpld_version(self):
        if self.name == "SysFPGA":
            return read_sysfs_file(FPGA_CODE_REV0)
        if self.name == "BIOS":
            return read_sysfs_file(BIOS_VERSION_PATH)
        if self.index < MAX_H4_32D_COMPONENT:
            return read_sysfs_file(self.cpld_dir + "code_ver")
        return 'NA'

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
        
        if self.name == "CPUPLD":
            # check whether the cpld upgrade tool exists
            if not os.path.isfile('/tmp/vme_h4_32d'):
                print("ERROR: the cpld upgrade tool /tmp/vme_h4_32d doesn't exist ")
                return False
            write_sysfs_file("/sys/class/gpio/export", str(666))
            write_sysfs_file("/sys/class/gpio/gpio666/value", str(1))
            self.CPLD_UPDATE_COMMAND[1] = 'jtag0'
            self.CPLD_UPDATE_COMMAND[2] = image_name
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            write_sysfs_file("/sys/class/gpio/gpio666/value", str(0))
            write_sysfs_file("/sys/class/gpio/unexport", str(666))
            print("\nCPUPLD firmware update has ended\n")
            print("!!!System will reboot in 10 sec!!!")
            time.sleep(7)
            self._power_cycle()
            return True
        
        elif self.name == "SWPLD2" or self.name == "SWPLD3":
            # check whether the cpld upgrade tool exists
            if not os.path.isfile('/tmp/vme_h4_32d'):
                print("ERROR: the cpld upgrade tool /tmp/vme_h4_32d doesn't exist ")
                return False
            write_sysfs_file("/sys/class/gpio/export", str(667))
            write_sysfs_file("/sys/class/gpio/gpio667/value", str(1))
            self.CPLD_UPDATE_COMMAND[1] = 'jtag1'
            self.CPLD_UPDATE_COMMAND[2] = image_name
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            if self.name == "SWPLD2":
                self.CPLD_UPDATE_COMMAND[2] = 'h4_32d_swpld2_refresh.vme'
            else:
                self.CPLD_UPDATE_COMMAND[2] = 'h4_32d_swpld3_refresh.vme'
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            write_sysfs_file("/sys/class/gpio/gpio667/value", str(0))
            write_sysfs_file("/sys/class/gpio/unexport", str(667))
            print("\nSWPLD2/3 firmware update has ended\n")
            return True
            
        elif self.name == "SysFPGA":
            # check whether the fpga upgrade tool exists
            if not os.path.isfile('/tmp/fpga_upg_tool'):
                print("ERROR: the fpga upgrade tool /tmp/fpga_upg_tool doesn't exist ")
                return False
            self.FPGA_UPDATE_COMMAND[4] = '0'
            self.FPGA_UPDATE_COMMAND[5] = image_name
            try:
                subprocess.run(self.FPGA_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade SysFPGA: rc={e.returncode}")
            self.FPGA_UPDATE_COMMAND[4] = '1'
            self.FPGA_UPDATE_COMMAND[5] = 'h4_32d_sysfpga_g.bit' 
            try:
                subprocess.run(self.FPGA_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade SysFPGA: rc={e.returncode}")
            print("\nSysFPGA firmware update has ended\n")
            print("!!!System will reboot in 10 sec!!!")
            time.sleep(7)
            self._power_cycle()
            return True
            
        elif self.name == "BIOS":
            # check whether the BIOS upgrade tool exists
            if not os.path.isfile('/tmp/afulnx_64'):
                print("ERROR: the BIOS upgrade tool /tmp/afulnx_64 doesn't exist ")
                return False
            self.BIOS_UPDATE_COMMAND[1] = image_name
            try:
                subprocess.run(self.BIOS_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade BIOS: rc={e.returncode}")
            print("\nBIOS update has ended\n")
            return True

        return False

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
    
    def _power_cycle(self):
        os.system('sync')
        os.system('sync')
        time.sleep(3)
        write_sysfs_file("/sys/kernel/delta_fpga/sys-pwr", str(1))
