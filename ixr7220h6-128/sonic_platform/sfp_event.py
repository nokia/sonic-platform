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
PORT_END = 130

SYSFS_DIR = "/sys/bus/i2c/devices/{}/"
PORTPLD_ADDR = ["152-0076", "153-0076", "148-0074", "149-0075", "150-0073", "151-0073"]

_BOOL_LOOKUP_LSB = [tuple((i >> s) & 1 for s in range(8)) for i in range(256)]
_BOOL_TABLE_A = [((i >> 0) & 1, (i >> 1) & 1, (i >> 4) & 1, (i >> 5) & 1)
                 for i in range(256)]
_BOOL_TABLE_B = [((i >> 2) & 1, (i >> 3) & 1, (i >> 6) & 1, (i >> 7) & 1)
                 for i in range(256)]

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
            write_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[3])+"port_33_tx_en", '0')
        if self.modprs_list[PORT_END-1]:
            write_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[3])+"port_34_tx_en", '0')

    def deinitialize(self):
        if self.handle is None:
            return

    def _get_transceiver_status(self):
        lookup = _BOOL_LOOKUP_LSB
        port_status = []
        reg_value = []
                
        for i in range(6):
            m = read_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[i]) + "modprs_reg1")
            n = read_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[i]) + "modprs_reg2")
            reg_value.extend([m,n])
            if i == 2 or i == 3:
                m = read_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[i]) + "modprs_reg3")
                n = read_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[i]) + "modprs_reg4")
                reg_value.extend([m,n])
                
        port_status.extend([bit for h in reg_value[:4] for bit in lookup[int(h, 16)]])
        port_status.extend(self._reorder(reg_value[4:12]))
        port_status.extend([bit for h in reg_value[-4:] for bit in lookup[int(h, 16)]])

        for i in range (33, 35):
            status = read_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[3])+f"port_{i}_prs")
            if status == '0':
                port_status.append(0)
            else:
                port_status.append(1)

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
                        if port_status[i] == 0:
                            port_change[i+1] = '1'
                        else:
                            port_change[i+1] = '0'

                        if (i == PORT_END -2) or (i == PORT_END -1):
                            if port_status[i] == 0:
                                write_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[3])+f"port_{i-95}_tx_en", '0')
                            else:
                                write_sysfs_file(SYSFS_DIR.format(PORTPLD_ADDR[3])+f"port_{i-95}_tx_en", '1')

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
    
    def _reorder(self, hex_list):
        t_a = _BOOL_TABLE_A
        t_b = _BOOL_TABLE_B
        res = []
        ext = res.extend
        data = [int(h, 16) for h in hex_list]
        for i in range(0, len(data), 8):
            chunk = data[i:i+8]
            for byte in chunk:
                ext(t_a[byte])
            for byte in chunk:
                ext(t_b[byte])
        return res
