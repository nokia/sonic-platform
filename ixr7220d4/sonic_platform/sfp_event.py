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
QSFP_PORT_NUM = 32
PORT_START    = 1
PORT_END      = 36

CPLD1_DIR = "/sys/bus/i2c/devices/10-0060/"
CPLD2_DIR = "/sys/bus/i2c/devices/10-0062/"
CPLD3_DIR = "/sys/bus/i2c/devices/10-0064/"
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

    def _get_transceiver_status(self):
        reg_value = []

        for p in range(PORT_START, PORT_START + PORT_END):
            sfp_prs = read_sysfs_file(CPLD1_DIR + f"qsfp{p}_mod_prsnt")
            reg_value.append(sfp_prs)
            if p < 19:
                write_sysfs_file(CPLD3_DIR + f"qsfp{p}_prs", sfp_prs)
            else:
                write_sysfs_file(CPLD2_DIR + f"qsfp{p}_prs", sfp_prs)

        port_status = [not bool(int(x)) for x in reg_value]
        # True: present, False: absent
        for port in range (PORT_START, PORT_START + PORT_END):
            if not port_status[port-1]: # if absent skip ...
                continue

            reset_status = read_sysfs_file(CPLD1_DIR+f"qsfp{port}_reset")
            if reset_status == '1':
                sonic_logger.log_info(f"Port #{port} is reseting...")
                port_status[port-1] = False
                write_sysfs_file(CPLD1_DIR+f"qsfp{port}_reset", '2')
            elif reset_status == '2':
                sonic_logger.log_info(f"Port #{port} is still reseting... ")
                port_status[port-1] = False
            elif reset_status == '3':
                sonic_logger.log_info(f"Port #%{port} reset done... ")
                port_status[port-1] = True
                write_sysfs_file(CPLD1_DIR+f"qsfp{port}_reset", '0')

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
                            port_change[i+1] = '1' #ins
                        else:
                            port_change[i+1] = '0' #rem

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
