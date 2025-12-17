"""
    NOKIA IXR7220 H5-64D

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

GPIO_GET_LINEHANDLE_IOCTL = 0xC16CB403
GPIOHANDLE_SET_LINE_VALUES_IOCTL = 0xC040B409
GPIOHANDLE_REQUEST_OUTPUT = 0x02

class gpiohandle_request(ctypes.Structure):
    _fields_ = [
        ('lineoffsets', ctypes.c_uint32 * 64),
        ('flags', ctypes.c_uint32),
        ('default_values', ctypes.c_uint8 * 64),
        ('consumer_label', ctypes.c_char * 32),
        ('lines', ctypes.c_uint32),
        ('fd', ctypes.c_int),
    ]

class gpiohandle_data(ctypes.Structure):
    _fields_ = [
        ('values', ctypes.c_uint8 * 64),
    ]

COMPONENT_NUM = 5
BIOS_VERSION_PATH = "/sys/class/dmi/id/bios_version"
SYSFS_DIR = [" ",
             "/sys/bus/i2c/devices/4-0040/",
             "/sys/kernel/sys_fpga/",
             "/sys/bus/i2c/devices/21-0041/",
             "/sys/bus/i2c/devices/21-0045/"]

class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "Basic Input/Output System"],
        ["CPUPLD", "Used for managing CPU board "],
        ["SysFPGA", "Used for managing BCM chip, SFPs, PSUs and LEDs "],
        ["SWPLD2", "Used for managing PORT 1-16, 33-48"],
        ["SWPLD3", "Used for managing PORT 17-32, 49-64, SFP+"] ]

    CPLD_UPDATE_COMMAND = ['./vme_h5', '', '']
    FPGA_UPDATE_COMMAND = ['./fpga_upg_tool', '-c', '1', '-p', '', '']
    BIOS_UPDATE_COMMAND = ['./afulnx_64', '', '/P', '/B', '/N']

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

        if self.name == "CPUPLD":
            # check whether the cpld upgrade tool exists
            if not os.path.isfile('/tmp/vme_h5'):
                print("ERROR: the cpld upgrade tool /tmp/vme_h5 doesn't exist ")
                return False
            write_sysfs_file("/sys/class/gpio/export", str(627))
            write_sysfs_file("/sys/class/gpio/export", str(604))
            write_sysfs_file("/sys/class/gpio/gpio627/direction", "out")
            write_sysfs_file("/sys/class/gpio/gpio604/direction", "out")
            write_sysfs_file("/sys/class/gpio/gpio627/value", str(1))
            write_sysfs_file("/sys/class/gpio/gpio604/value", str(1))
            self.CPLD_UPDATE_COMMAND[1] = 'jtag0'
            self.CPLD_UPDATE_COMMAND[2] = image_name
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            self.gpio_set("/dev/gpiochip0", 105, 1)
            self.CPLD_UPDATE_COMMAND[2] = 'h5_cpupld_refresh.vme'
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            self.gpio_set("/dev/gpiochip0", 105, 0)
            write_sysfs_file("/sys/class/gpio/gpio604/value", str(0))
            write_sysfs_file("/sys/class/gpio/gpio627/value", str(0))
            write_sysfs_file("/sys/class/gpio/unexport", str(604))
            write_sysfs_file("/sys/class/gpio/unexport", str(627))
            print("\nCPUPLD firmware upgraded!\n")
            return True

        elif self.name == "SWPLD2":
            # check whether the cpld upgrade tool exists
            if not os.path.isfile('/tmp/vme_h5'):
                print("ERROR: the cpld upgrade tool /tmp/vme_h5 doesn't exist ")
                return False
            write_sysfs_file("/sys/class/gpio/export", str(769))
            write_sysfs_file("/sys/class/gpio/gpio769/value", str(1))
            self.CPLD_UPDATE_COMMAND[1] = 'jtag1'
            self.CPLD_UPDATE_COMMAND[2] = image_name
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            self.CPLD_UPDATE_COMMAND[2] = 'h5_swpld2_refresh.vme'
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            write_sysfs_file("/sys/class/gpio/gpio769/value", str(0))
            write_sysfs_file("/sys/class/gpio/unexport", str(769))
            print("\nSWPLD2 firmware upgraded!\n")
            return True
    
        elif self.name == "SWPLD3":
            # check whether the cpld upgrade tool exists
            if not os.path.isfile('/tmp/vme_h5'):
                print("ERROR: the cpld upgrade tool /tmp/vme_h5 doesn't exist ")
                return False
            write_sysfs_file("/sys/class/gpio/export", str(769))
            write_sysfs_file("/sys/class/gpio/gpio769/value", str(1))
            self.CPLD_UPDATE_COMMAND[1] = 'jtag1'
            self.CPLD_UPDATE_COMMAND[2] = image_name
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            self.CPLD_UPDATE_COMMAND[2] = 'h5_swpld3_refresh.vme'
            try:
                subprocess.run(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade CPLD: rc={e.returncode}")
            write_sysfs_file("/sys/class/gpio/gpio769/value", str(0))
            write_sysfs_file("/sys/class/gpio/unexport", str(769))
            print("\nSWPLD3 firmware upgraded!\n")
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
            self.FPGA_UPDATE_COMMAND[5] = 'h5_sysfpga_g.bit'
            try:
                subprocess.run(self.FPGA_UPDATE_COMMAND, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as e:
                print(f"ERROR: Failed to upgrade SysFPGA: rc={e.returncode}")
            print("\nSysFPGA firmware upgraded!\n")
            print("!!!The system will reboot in 10 sec!!!")
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

    def gpio_set(self, gpio_device, line, value):
        request = gpiohandle_request()
        request.lineoffsets[0] = line
        request.flags = GPIOHANDLE_REQUEST_OUTPUT
        request.consumer_label = "gpiochip_handler".encode()
        request.lines = 1
        try:
            chip_fd = os.open(gpio_device, os.O_RDONLY)
        except OSError as e:
            print(f"ERROR: {e.errno}, Opening GPIO chip: " + e.strerror)

        try:
            fcntl.ioctl(chip_fd, GPIO_GET_LINEHANDLE_IOCTL, request)
        except (OSError, IOError) as e:
            print(f"ERROR: {e.errno}, Opening output line handle: " + e.strerror)
        os.close(chip_fd)
        data = gpiohandle_data()
        data.values[0] = value
        try:
            fcntl.ioctl(request.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, data)
        except (OSError, IOError) as e:
            print(f"ERROR: {e.errno}, Setting line value: " + e.strerror)
        os.close(request.fd)

    def _power_cycle(self):
        os.system('sync')
        os.system('sync')
        time.sleep(3)
        write_sysfs_file("/sys/kernel/sys_fpga/sys_pwr", str(1))
