# Name: nokia_led_mgmt.py, version: 1.0
#
# Description: Module contains the Nokia IXR7250 specific implementation
# of the LedControlCommon class
#
# Copyright (c) 2019, Nokia
# All rights reserved.

from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
from sonic_py_common.logger import Logger
from sonic_py_common import multi_asic, device_info
from sonic_platform_base.device_base import DeviceBase
from sonic_platform_base.sonic_sfp.sfputilhelper import SfpUtilHelper
from swsscommon import swsscommon
try:
    from sonic_led import led_control_base
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

SYSLOG_IDENTIFIER = "led-mgmt"
logger = Logger(SYSLOG_IDENTIFIER)


class LedControlCommon(led_control_base.LedControlBase):
    def __init__(self):
        self.platform_sfputil = SfpUtilHelper()
        if multi_asic.is_multi_asic():
            # For multi ASIC platforms we pass DIR of port_config_file_path and the number of asics
            (platform_path, hwsku_path) = device_info.get_paths_to_platform_and_hwsku_dirs()

            if not swsscommon.SonicDBConfig.isGlobalInit():
                swsscommon.SonicDBConfig.load_sonic_global_db_config()
            # Load platform module from source
            self.platform_sfputil.read_all_porttab_mappings(hwsku_path, multi_asic.get_num_asics())
        else:
            # For single ASIC platforms we pass port_config_file_path and the asic_inst as 0
            port_config_file_path = device_info.get_path_to_port_config_file()
            
            if not swsscommon.SonicDBConfig.isInit():
                swsscommon.SonicDBConfig.load_sonic_db_config()
            self.platform_sfputil.read_porttab_mappings(port_config_file_path, 0)

        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_PORT
        led_info = nokia_common.led_color_to_info(DeviceBase.STATUS_LED_COLOR_OFF)
        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
        if not channel or not stub:
            return
        port_idx = platform_ndk_pb2.ReqLedIndexPb(
                start_idx=nokia_common.NOKIA_FP_START_PORTID,
                end_idx=nokia_common.NOKIA_FP_END_PORTID)
        ret, response = nokia_common.try_grpc(stub.SetLed,
                platform_ndk_pb2.ReqLedInfoPb(led_type=led_type,
                                              led_idx=port_idx, led_info=led_info))
        nokia_common.channel_shutdown(channel)

    def port_link_state_change(self, port, state):
        intf_prefix = 'Ethernet'
        if port.startswith(intf_prefix) is False:
            return
        else:
            if self.platform_sfputil.is_logical_port(port):
                port_id_list = self.platform_sfputil.get_logical_to_physical(port)
                port_id = port_id_list[0] - 1
            else:
                return

        if state == 'up':
            port_up = 1
        elif state == 'down':
            port_up = 0
        else:
            return

        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_PORT
        if port_up:
            led_info = nokia_common.led_color_to_info(DeviceBase.STATUS_LED_COLOR_GREEN)
        else:
            led_info = nokia_common.led_color_to_info(DeviceBase.STATUS_LED_COLOR_OFF)

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
        if not channel or not stub:
            return
        port_idx = platform_ndk_pb2.ReqLedIndexPb(start_idx=port_id, end_idx=port_id)
        ret, response = nokia_common.try_grpc(stub.SetLed,
                platform_ndk_pb2.ReqLedInfoPb(led_type=led_type,
                                              led_idx=port_idx, led_info=led_info))
        nokia_common.channel_shutdown(channel)


def getLedControl():
    return LedControlCommon
