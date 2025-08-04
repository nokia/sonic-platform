"""
    NOKIA IXR7220 H6-64

    Module contains an implementation of SONiC Platform Base API and
    provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
    the platform
"""

try:
    import os
    import subprocess
    import ntpath
    import time
    import glob
    from sonic_platform_base.component_base import ComponentBase
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSFS_DIR = ["/sys/class/dmi/id/",
             "/sys/bus/i2c/devices/1-0060/",
             "/sys/bus/i2c/devices/73-0061/",
             "/sys/bus/i2c/devices/89-0064/",
             "/sys/bus/i2c/devices/90-0065/",
             "/sys/bus/i2c/devices/79-0033/hwmon/hwmon*/",
             "/sys/bus/i2c/devices/80-0033/hwmon/hwmon*/"]

class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "Basic Input/Output System"],
        ["SYS_FPGA", "Used for managing CPU board"],
        ["SYS_CPLD", "Used for managing BCM chip, PSUs and LEDs"],
        ["PORT_CPLD0", "Used for managing PORT 1-16, 33-48, SFP28"],
        ["PORT_CPLD1", "Used for managing PORT 17-32, 49-64"],
        ["FCM0_CPLD", "Used for managing upper fan drawers"],
        ["FCM1_CPLD", "Used for managing lower fan drawers"] ]
    DEV_NAME = [" ", " ", "MAIN_CPLD", "MAIN_CPLD", "MAIN_CPLD", "FAN0_CPLD", "FAN1_CPLD"]
    TFR_NAME = [" ", " ", "h6_64_sys_cpld_tfr.vme", "h6_64_port_cpld0_tfr.vme", 
                "h6_64_port_cpld1_tfr.vme", "h6_64_fan_cpld_tfr.vme", "h6_64_fan_cpld_tfr.vme"]

    BIOS_UPDATE_COMMAND = ['./afulnx_64', '', '/B', '/P', '/N', '/K']
    FPGA_CHECK_COMMAND = ['./fpga_spi_flash.sh', '-rid']
    FPGA_UPDATE_COMMAND = ['./fpga_spi_flash.sh', '-upd', '', '-all']
    CPLD_UPDATE_COMMAND = ['./cpldupd', '-u', '', '']

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        if self.name == "FCM0_CPLD" or self.name == "FCM1_CPLD":
            hwmon_dir = glob.glob(SYSFS_DIR[self.index])
            self.sysfs_dir = hwmon_dir[0]
        else:
            self.sysfs_dir = SYSFS_DIR[self.index]
        self.dev_name = self.DEV_NAME[self.index]
        self.tfr_name = self.TFR_NAME[self.index]

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
            return read_sysfs_file(self.sysfs_dir + "bios_version")
        else:
            return read_sysfs_file(self.sysfs_dir + "version")

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
                return False
            print("\nBIOS update has ended\n")
        
        elif self.name == "SYS_FPGA":
            # check whether the fpga upgrade tool exists
            if not os.path.isfile('/tmp/fpga_spi_flash.sh'):
                print("ERROR: the fpga upgrade tool /tmp/fpga_spi_flash.sh doesn't exist ")
                return False
            if not os.path.isfile('/tmp/fpga_upd2'):
                print("ERROR: the fpga upgrade tool /tmp/fpga_upd2 doesn't exist ")
                return False
            try:
                result = subprocess.check_output(self.FPGA_CHECK_COMMAND)
                result = subprocess.check_output(self.FPGA_CHECK_COMMAND)
                text = result.decode('utf-8')
                print(text)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to check SYS_FPGA RDID: rc={e.returncode}")
            last = text.splitlines()
            if last[-1].strip() != "RDID: c2 20 18":
                print("FPGA RDID check failed!")
                return False
            self.FPGA_UPDATE_COMMAND[2] = image_name
            try:
                subprocess.run(self.FPGA_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade SYS_FPGA: rc={e.returncode}")
                return False
            print("\nSYS_FPGA firmware update has ended\n")
            print("!!!The system will power cycle in 10 sec!!!")
            time.sleep(7)
            self._power_cycle()

        else:
            # check whether the cpld upgrade tool exists
            if not os.path.isfile('/tmp/cpldupd'):
                print("ERROR: the cpld upgrade tool /tmp/cpldupd doesn't exist ")
                return False
            val = [" ", " ", "0x4", "0x2", "0x1", "0x10", "0x8"]
            write_sysfs_file("/sys/bus/i2c/devices/1-0060/hitless", val[self.index])
            self.CPLD_UPDATE_COMMAND[2] = self.dev_name
            self.CPLD_UPDATE_COMMAND[3] = image_name
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade {self.name}: rc={e.returncode}")
                return False
            self.CPLD_UPDATE_COMMAND[3] = self.tfr_name
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade {self.name}: rc={e.returncode}")
                return False
            write_sysfs_file("/sys/bus/i2c/devices/1-0060/hitless", "0x0")
            print(f"\n{self.name} firmware update has ended\n")
        
        return True

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

    def _power_cycle(self):
        os.system('sync')
        os.system('sync')
        time.sleep(3)
        for i in range(4):
            file_path = f"/sys/bus/i2c/devices/{i+94}-00{hex(0x58+i)[2:]}/psu_rst"
            if os.path.exists(file_path):
                write_sysfs_file(file_path, "Reset\n")