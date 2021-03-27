# Name: sfp.py, version: 1.0
#
# Description: Module contains the definitions of SFP change event
# related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

from __future__ import print_function
import os
import time
import subprocess
from swsscommon import swsscommon
from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
from sonic_platform.sfp import Sfp
from sonic_py_common.logger import Logger
from sonic_py_common import daemon_base


# from sfp.Sfp import SfpHasBeenTransitioned

logger = Logger("sfp_event")

MAX_NOKIA_SFP_EVENT_SLEEP_TIME = 5
TRANSCEIVER_INFO_TABLE = 'TRANSCEIVER_INFO'
SFPEVENT_TOTAL_NUM_TESTS = 4

# @todo-nokia Remove once the below APIs are present in util_base.py as part of PDDF
PLATFORM_ROOT_DOCKER = '/usr/share/sonic/platform'
SONIC_CFGGEN_PATH = '/usr/local/bin/sonic-cfggen'
HWSKU_KEY = 'DEVICE_METADATA.localhost.hwsku'
PLATFORM_KEY = 'DEVICE_METADATA.localhost.platform'


class sfp_event:
    ''' Listen to plugin/plugout cable events '''

    def __init__(self, num_ports, stub):
        self.num_ports = num_ports
        self.port_begin = 1
        self.port_end = num_ports

        self.stub = stub
        self.test_num = 1
        self.test_port_num = 8
        self.test_port_status = True

        try:
            proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-H', '-v', PLATFORM_KEY],
                                    stdout=subprocess.PIPE,
                                    shell=False,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            self.platform = stdout.rstrip(b'\n')

            proc = subprocess.Popen([SONIC_CFGGEN_PATH, '-d', '-v', HWSKU_KEY],
                                    stdout=subprocess.PIPE,
                                    shell=False,
                                    stderr=subprocess.STDOUT)
            stdout = proc.communicate()[0]
            proc.wait()
            self.hwsku = stdout.rstrip(b'\n')
        except OSError as e:
            raise OSError("Failed to detect platform: %s" % (str(e)))

        logger.log_info("Init sfp-event - num_ports {}, begin {} and end {} : platform {} hwsku {}".format(
            num_ports, self.port_begin, self.port_end, self.platform, self.hwsku))

    def initialize(self):
        self.port_status_list = []

        # Get Transceiver status
        self.port_status_list = self._get_transceiver_status()
        self.debug_print_port_list(self.port_status_list)

    def deinitialize(self):
        if self.handle is None:
            return

    def debug_print_port_list(self, port_list):
        if not port_list:
            return

        i = 0
        port_str = ''
        for i in range(self.num_ports):
            port_idx = (self.port_begin + i)
            port_val = port_list[i]
            port_str = port_str+'{'+str(port_idx)+','+str(port_val)+'}, '
        logger.log_info("SFP-event with presence list = {}".format(port_str))

    # Returns platform and hwsku
    def get_platform_and_hwsku(self):
        return (self.platform, self.hwsku)

    # Returns path to platform and hwsku
    def get_path_to_platform_and_hwsku(self):
        # Get platform and hwsku
        (platform, hwsku) = self.get_platform_and_hwsku()

        # Load platform module from source
        platform_path = PLATFORM_ROOT_DOCKER
        hwsku_path = '/'.join([str(platform_path), str(hwsku)])

        return (platform_path, hwsku_path)

    def test_check_port_status(self, port, status):
        state_db = daemon_base.db_unix_connect(swsscommon.STATE_DB)
        int_tbl = swsscommon.Table(state_db, TRANSCEIVER_INFO_TABLE)
        port_name = 'Ethernet'+str(port)
        # port_name_str = 'Ethernet'+str(self.port_begin+port-1)
        status, cur_fvs = int_tbl.get(port_name)
        # logger.log_error("SFP-event test port {} fvs{}".format(port_name_str, cur_fvs))

    def test_port_add(self, port_list, port):
        port_list[port-1] = True

    def test_port_del(self, port_list, port):
        port_list[port-1] = False

    def test_run_suite(self, test_num, port_list, port_num):
        # logger.log_error("SFP-event test-suite test-case {}".format(test_num))
        if test_num == 1:
            status = self.test_port_add(port_list, port_num)
        elif test_num == 2:
            status = self.test_check_port_status(port_num, True)
        elif test_num == 3:
            status = self.test_port_del(port_list, port_num)
        elif test_num == 4:
            status = self.test_check_port_status(port_num, False)

        if status is False:
            logger.log_error("SFP-event test-suite failed in test-case{}".format(test_num))

    def test_enable(self):
        (platform, hwsku_path) = self.get_path_to_platform_and_hwsku()
        test_enable_file_path = "/".join([platform, "test/test_enable"])
        if not os.path.isfile(test_enable_file_path):
            return False

        return True

    def _get_transceiver_status(self):
        # logger.log_error("Transceiver multi-status")

        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_GET_MPORT_STATUS
        port_list = []

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return port_list
        ret, response = nokia_common.try_grpc(
            stub.GetSfpPresence,
            platform_ndk_pb2.ReqSfpOpsPb(type=op_type,
                                         hw_port_id_begin=self.port_begin,
                                         hw_port_id_end=self.port_end))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return port_list
        # logger.log_error("Transceiver multi-status-done")

        port_list = []
        mport_msg = response.sfp_mstatus
        i = 0
        num_ports = len(mport_msg.port_status)
        for i in range(num_ports):
            port_status = mport_msg.port_status[i]
            port_list.append(port_status.status)

        # logger.log_error("Transceiver multi-status response with len{}".format(num_ports))
        return port_list

    def check_sfp_status(self, port_change, timeout):
        """
         check_sfp_status called from get_change_event,
         this will return correct status of all QSFP ports if there is a change in any of them
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

        if (start_time > end_time):
            return False, {}  # Time wrap or possibly incorrect timeout

        while (timeout >= 0):
            if (self.test_enable()):
                port_list = []
                if (self.test_num == 1) or (self.test_num == (SFPEVENT_TOTAL_NUM_TESTS + 1)):
                    self.test_port_list = self._get_transceiver_status()
                    self.test_num = 1

                self.test_run_suite(self.test_num, self.test_port_list, self.test_port_num)
                port_list = list(self.test_port_list)
                self.test_num += 1
            else:
                self.test_num = 1
                port_list = self._get_transceiver_status()

            # self.debug_print_port_list(self.port_status_list)
            if (port_list != self.port_status_list):
                for i in range(self.num_ports):
                    if port_list[i] != self.port_status_list[i]:
                        # Port 8 will be index 7 in port_list
                        if port_list[i] is True:
                            port_change[i+1] = '1'
                        else:
                            port_change[i+1] = '0'

                        logger.log_info(
                            "sfp_event.check_sfp_status: port{} status changed to {} ".format(i, port_change[i+1]))
                        Sfp.SfpHasBeenTransitioned(i, port_change[i+1])

                # Save the current port status list
                self.port_status_list = port_list
                return True, port_change

            if forever:
                time.sleep(MAX_NOKIA_SFP_EVENT_SLEEP_TIME)
            else:
                timeout = end_time - time.time()
                logger.log_error("Check sfp status timeout {}".format(timeout))
                if timeout >= 1:
                    time.sleep(1)  # We poll at 1 second granularity
                else:
                    if timeout > 0:
                        time.sleep(timeout)
                    return True, {}
        return False, {}
