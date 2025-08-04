""""
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
PORT_END = 66

SWPLD2_DIR = "/sys/bus/i2c/devices/89-0064/"
SWPLD3_DIR = "/sys/bus/i2c/devices/90-0065/"

SYSLOG_IDENTIFIER = "sfp_event"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)

class SfpEvent:
    ''' Listen to plugin/plugout cable events '''

    def __init__(self):
        self.handle = None
        self.modprs_list = []

    def initialize(self):
        # Get Transceiver status
        time.sleep(5)
        self.modprs_list = self._get_transceiver_status()
        if self.modprs_list[PORT_END-2]:
            write_sysfs_file(SWPLD2_DIR+"port_65_tx_en", '0')
        if self.modprs_list[PORT_END-1]:
            write_sysfs_file(SWPLD2_DIR+"port_66_tx_en", '0')

    def deinitialize(self):
        if self.handle is None:
            return

    def _get_transceiver_status(self):
        port_status = []
        reg_value = []
        reg_value.append(read_sysfs_file(SWPLD2_DIR + "modprs_reg1"))
        reg_value.append(read_sysfs_file(SWPLD2_DIR + "modprs_reg2"))
        reg_value.append(read_sysfs_file(SWPLD3_DIR + "modprs_reg1"))
        reg_value.append(read_sysfs_file(SWPLD3_DIR + "modprs_reg2"))
        reg_value.append(read_sysfs_file(SWPLD2_DIR + "modprs_reg3"))
        reg_value.append(read_sysfs_file(SWPLD2_DIR + "modprs_reg4"))
        reg_value.append(read_sysfs_file(SWPLD3_DIR + "modprs_reg3"))
        reg_value.append(read_sysfs_file(SWPLD3_DIR + "modprs_reg4"))
        for i in range(8):
            bin_str = f'{int(reg_value[i], 16):08b}'
            bin_str = bin_str[::-1]
            bool_list = [not bool(int(bit)) for bit in bin_str]
            port_status.extend(bool_list)

        for port in range (PORT_END-1, PORT_END+1):
            status = read_sysfs_file(SWPLD2_DIR+f"port_{port}_prs")
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

        while (timeout >= 0):
            # Check for OIR events and return updated port_change
            port_status = self._get_transceiver_status()
            if port_status != self.modprs_list:
                for i in range(PORT_END):
                    if port_status[i] != self.modprs_list[i]:
                        if port_status[i] == True:
                            port_change[i+1] = '1'
                        else:
                            port_change[i+1] = '0'

                        if (i == PORT_END -2) or (i == PORT_END -1):
                            if port_status[i]:
                                write_sysfs_file(SWPLD2_DIR+f"port_{i+1}_tx_en", '0')
                            else:
                                write_sysfs_file(SWPLD2_DIR+f"port_{i+1}_tx_en", '1')

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
