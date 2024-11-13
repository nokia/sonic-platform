"""
    listen for the SFP change event and return to chassis.
"""
try:
    import os
    import time
    import subprocess
    from sonic_py_common import logger
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

# system level event/error
EVENT_ON_ALL_SFP = '-1'
SYSTEM_NOT_READY = 'system_not_ready'
SYSTEM_READY = 'system_become_ready'
SYSTEM_FAIL = 'system_fail'

# SFP PORT numbers
QSFP_PORT_NUM = 64
PORT_START = 1
PORT_END = 66

REG_DIR = "/sys/devices/platform/sys_fpga/"
DRIVER_PATH = "/sys/bus/platform/drivers/pcie_{}_fpga_device/"
PORT_NAME = "pcie_{}_fpga_device.{}"
EEPROM_PATH = "/sys/devices/platform/pcie_{}_fpga_device.{}/eeprom"

SYSLOG_IDENTIFIER = "sfp_event"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class SfpEvent:
    ''' Listen to plugin/plugout cable events '''

    def __init__(self):
        self.handle = None
        self.modprs_list = []

    def initialize(self):
        """
        Initialize SFP
        """
        # Get Transceiver status
        time.sleep(5)
        self.modprs_list = self._get_transceiver_status()
        sonic_logger.log_info(f"Initial SFP presence={str(self.modprs_list)}")
        if self.modprs_list[PORT_END-2]:
            write_sysfs_file(REG_DIR+"module_tx_disable_65", '0')
        if self.modprs_list[PORT_END-1]:
            write_sysfs_file(REG_DIR+"module_tx_disable_66", '0')
        for i in range (5):
            rdx = (i + 1) * 16
            if rdx > PORT_END:
                rdx = PORT_END
            pres_group = self.modprs_list[i*16:rdx]
            pres_group.reverse()
            bin_str = ''.join(['0' if x else '1' for x in pres_group])
            hex_str = hex(int(bin_str, 2))
            write_sysfs_file(REG_DIR+f"port_g{i+1}_led_pres", hex_str)

    def deinitialize(self):
        """
        Deinitialize SFP
        """

    def _get_transceiver_status(self):
        port_status = []
        reg_value = read_sysfs_file(REG_DIR + "module_present_all")
        bin_str = f'{int(reg_value, 16):068b}'
        bin_str = bin_str[68:1:-1]
        bool_list = [not bool(int(bit)) for bit in bin_str]
        port_status.extend(bool_list)

        for port in range (PORT_START, PORT_START + PORT_END):
            if port <= QSFP_PORT_NUM:
                if port_status[port-1]:
                    reset_status = read_sysfs_file(REG_DIR+f"qsfp{port}_reset")
                    if reset_status == '1':                        
                        port_status[port-1] = False
                        write_sysfs_file(REG_DIR+f"qsfp{port}_reset", '2')
                    elif reset_status == '2':                        
                        port_status[port-1] = False
                    elif reset_status == '3':                        
                        port_status[port-1] = True
                        write_sysfs_file(REG_DIR+f"qsfp{port}_reset", '0')

        return port_status

    def check_sfp_status(self, port_change, timeout):
        """
        check_sfp_status called from get_change_event, this will return correct
            status of all SFP ports if there is a change in any of them
        """
        start_time = time.time()
        forever = False

        if timeout == 0:
            forever = True
        elif timeout > 0:
            timeout = timeout / float(1000)  # Convert to secs
        else:
            return False, {}
        end_time = start_time + timeout

        if start_time > end_time:
            return False, {}  # Time wrap or possibly incorrect timeout

        while timeout >= 0:
            # Check for OIR events and return updated port_change
            port_status = self._get_transceiver_status()
            if port_status != self.modprs_list:
                for i in range(PORT_END):
                    if port_status[i] != self.modprs_list[i]:
                        if port_status[i]:
                            port_change[i+1] = '1'                            
                            
                            if i < QSFP_PORT_NUM // 2:
                                prefix = "udb"
                                port_index = i
                            else:
                                prefix = "ldb"
                                port_index = i - QSFP_PORT_NUM // 2
                            eeprom_file = EEPROM_PATH.format(prefix, port_index)
                            if os.path.getsize(eeprom_file) == 256:
                                port_name = PORT_NAME.format(prefix, port_index)
                                subprocess.run(f"echo {port_name} > {DRIVER_PATH.format(prefix)}"+"unbind", shell=True)
                                subprocess.run(f"echo {port_name} > {DRIVER_PATH.format(prefix)}"+"bind", shell=True)
                            if os.path.getsize(eeprom_file) == 256:
                                port_change[i+1] = '0'
                                port_status[i] = False
                                write_sysfs_file(REG_DIR+f"port{i+1}_led_pres", '1')
                            else:
                                write_sysfs_file(REG_DIR+f"port{i+1}_led_pres", '0')

                        else:
                            port_change[i+1] = '0'
                            write_sysfs_file(REG_DIR+f"port{i+1}_led_pres", '1')
                        
                        if (i >= QSFP_PORT_NUM):                            
                            if port_status[i]:
                                write_sysfs_file(REG_DIR+f"module_tx_disable_{i+1}", '0')
                            else:
                                write_sysfs_file(REG_DIR+f"module_tx_disable_{i+1}", '1')                            

                # Update reg value
                self.modprs_list = port_status
                return True, port_change

            if forever:
                time.sleep(1)
            else:
                timeout = end_time - time.time()
                if timeout >= 1:
                    time.sleep(1)  # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, {}
        return False, {}
