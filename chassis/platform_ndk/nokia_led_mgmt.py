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
from sonic_platform_base.device_base import DeviceBase
try:
    from sonic_led import led_control_base
except ImportError as e:
    raise ImportError('%s - required module not found' % str(e))

SYSLOG_IDENTIFIER = "led-mgmt"
logger = Logger(SYSLOG_IDENTIFIER)


class LedControlCommon(led_control_base.LedControlBase):
    def __init__(self):
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
            import re
            parse_nums = re.findall(r'\d+', port)
            port_id = int(parse_nums[0])

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
