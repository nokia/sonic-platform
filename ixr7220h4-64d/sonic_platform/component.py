########################################################################
# NOKIA IXR7220 H4-64D
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Components' (e.g., BIOS, CPLD, FPGA, etc.) available in
# the platform
#
########################################################################

try:
    import sys
    import os
    import time
    import subprocess
    import ntpath
    import struct    
    from mmap import *
    from sonic_platform_base.component_base import ComponentBase
    from sonic_py_common.general import getstatusoutput_noshell, getstatusoutput_noshell_pipe
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

if sys.version_info[0] < 3:
    import commands as cmd
else:
    import subprocess as cmd

COMPONENT_NUM = 13
SYSFS_PATH = ["/sys/class/dmi/id/bios_version",
             "/sys/bus/platform/drivers/sys_fpga/sys_fpga/smb_version",
             "/sys/bus/platform/drivers/sys_fpga/sys_fpga/udb_version",
             "/sys/bus/platform/drivers/sys_fpga/sys_fpga/ldb_version",
             "/sys/bus/i2c/devices/6-0060/code_ver",
             "/sys/devices/platform/sys_fpga/udb_cpld1_ver",
             "/sys/devices/platform/sys_fpga/udb_cpld2_ver",
             "/sys/devices/platform/sys_fpga/ldb_cpld1_ver",
             "/sys/devices/platform/sys_fpga/ldb_cpld2_ver",
             "/sys/bus/i2c/devices/25-0033/code_ver",
             "/sys/bus/i2c/devices/36-0060/code_ver",
             "/sys/bus/i2c/devices/44-0060/code_ver",
             "/sys/bus/i2c/devices/51-0035/code_ver"]

class Component(ComponentBase):
    """Nokia platform-specific Component class"""

    CHASSIS_COMPONENTS = [
        ["BIOS", "Basic Input/Output System"],
        ["SMB FPGA", "Used for managing syetem board"],
        ["UDB FPGA", "Used for managing upper QSFPs 1-32"],
        ["LDB FPGA", "Used for managing lower QSFPs 33-64 and SFP+"],
        ["SMB CPLD", "Used for managing system board"],
        ["UDB CPLD1", "Used for managing upper QSFPs 1-32"],
        ["UDB CPLD2", "Used for managing upper QSFPs 1-32"],
        ["LDB CPLD1", "Used for managing lower QSFPs 33-64 and SFP+"],
        ["LDB CPLD2", "Used for managing lower QSFPs 33-64 and SFP+"],
        ["FAN CPLD", "Used for managing fans"],
        ["PDB_L CPLD", "Used for managing left PSU"],
        ["PDB_R CPLD", "Used for managing right PSU"],
        ["SCM CPLD", "Used for managing misc system"]]
    
    CPLD_UPDATE_COMMAND = ['./vme', '']

    def __init__(self, component_index):
        self.index = component_index
        self.name = self.CHASSIS_COMPONENTS[self.index][0]
        self.description = self.CHASSIS_COMPONENTS[self.index][1]
        self.sysfs_path = SYSFS_PATH[self.index]

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
    
    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
                fd.close()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _write_sysfs_file(self, sysfs_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(value)
                fd.close()
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if (int(self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if (int(self._read_sysfs_file(sysfs_file)) != value ):
                rv = 'ERR'

        return rv

    def _get_cpld_version(self):
        return self._read_sysfs_file(self.sysfs_path)

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
        print(" IXR-7220-H4-64D - install cpld {}".format(image_name))

        # check whether the image file exists
        if not os.path.isfile(image_path):
            print("ERROR: the cpld image {} doesn't exist ".format(image_path))
            return False

        # check whether the cpld exe exists
        if not os.path.isfile('/tmp/vme'):
            print("ERROR: the cpld exe {} doesn't exist ".format('/tmp/vme'))
            return False

        self.CPLD_UPDATE_COMMAND[1] = image_name

        success_flag = False
 
        try:   
            subprocess.check_call(self.CPLD_UPDATE_COMMAND, stderr=subprocess.STDOUT)

            success_flag = True
        except subprocess.CalledProcessError as e:
            print("ERROR: Failed to upgrade CPLD: rc={}".format(e.returncode))

        if success_flag:
            print("INFO: Refresh or power cycle is required to finish CPLD installation")

        return success_flag
    
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
