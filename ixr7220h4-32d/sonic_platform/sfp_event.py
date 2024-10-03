"""
    listen for the SFP change event and return to chassis.
"""
try:
    import time
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
PORT_START = 1
PORT_END = 33
QSFP_PORT_NUM = 32
QSFP_IN_SWPLD = 16

SWPLD2_DIR = "/sys/bus/i2c/devices/14-0034/"
SWPLD3_DIR = "/sys/bus/i2c/devices/14-0035/"

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
        if self.modprs_list[PORT_END-1]:
            write_sysfs_file(SWPLD3_DIR+"sfp0_tx_en", '0')

    def _get_transceiver_status(self):
        port_status = []
        reg_value = []
        reg_value.append(read_sysfs_file(SWPLD2_DIR + "modprs_reg1"))
        reg_value.append(read_sysfs_file(SWPLD2_DIR + "modprs_reg2"))
        reg_value.append(read_sysfs_file(SWPLD3_DIR + "modprs_reg1"))
        reg_value.append(read_sysfs_file(SWPLD3_DIR + "modprs_reg2"))
        for i in range(4):
            bin_str = f'{int(reg_value[i], 16):08b}'
            bool_list = [not bool(int(bit)) for bit in bin_str]
            port_status.extend(bool_list)

        for port in range (PORT_START, PORT_START + PORT_END):
            if port <= QSFP_PORT_NUM:
                if port_status[port-1]:
                    if port <= QSFP_IN_SWPLD:
                        swpld_path = SWPLD2_DIR
                    else:
                        swpld_path = SWPLD3_DIR
                    reset_status = read_sysfs_file(swpld_path+f"qsfp{port}_reset")
                    if reset_status == '1':
                        sonic_logger.log_info(f"Port #{port} is reseting...")
                        port_status[port-1] = False
                        write_sysfs_file(swpld_path+f"qsfp{port}_reset", '2')
                    elif reset_status == '2':
                        sonic_logger.log_info(f"Port #{port} is still reseting... ")
                        port_status[port-1] = False
                    elif reset_status == '3':
                        sonic_logger.log_info(f"Port #%{port} reset done... ")
                        port_status[port-1] = True
                        write_sysfs_file(swpld_path+f"qsfp{port}_reset", '0')

            else:
                status = read_sysfs_file(SWPLD3_DIR+f"sfp{port - QSFP_PORT_NUM - 1}_prs")
                if status == '0':
                    port_status.append(True)
                else:
                    port_status.append(False)

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
                        else:
                            port_change[i+1] = '0'

                        if i == PORT_END -1:
                            if port_status[i]:
                                write_sysfs_file(SWPLD3_DIR+f"sfp{i-QSFP_PORT_NUM}_tx_en", '0')
                            else:
                                write_sysfs_file(SWPLD3_DIR+f"sfp{i-QSFP_PORT_NUM}_tx_en", '1')

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
