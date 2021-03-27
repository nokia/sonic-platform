# Name: nokia_common.py, version: 1.0
#
# Description: Module contains the common definitions for Nokia
# specific platform monitoring code.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

import os

from sonic_platform_base.device_base import DeviceBase
import grpc
from platform_ndk import platform_ndk_pb2
from platform_ndk import platform_ndk_pb2_grpc

NOKIA_UNIX_SOCKET_PREFIX = "unix://"
NOKIA_SONIC_UNIX_SOCKET_FOLDER = "/var/run/redis/"
NOKIA_DEVMGR_SONIC_UNIX_SOCKET_NAME = "devmgr_sonic_server"
NOKIA_DEVMGR_SONIC_SRVR_PORT = "50065"
NOKIA_DEVMGR_SONIC_SRVR_IP = "127.0.0.1"
NOKIA_DEVMGR_MONITOR_UPDATE_PERIOD_SECS = 1
NOKIA_CHANNEL_FILE_PATH = "/tmp/nokia_grpc_server"

NOKIA_DEVMGR_UNIX_SOCKET_PATH = NOKIA_UNIX_SOCKET_PREFIX+\
        NOKIA_SONIC_UNIX_SOCKET_FOLDER+\
        NOKIA_DEVMGR_SONIC_UNIX_SOCKET_NAME+":"+\
        NOKIA_DEVMGR_SONIC_SRVR_PORT

NOKIA_FP_START_PORTID = 0
NOKIA_FP_END_PORTID = 35
NOKIA_MAX_PORTS_PER_ASIC = 12
NOKIA_CPM_SLOT_NUMBER = 16
NOKIA_INVALID_SLOT_NUMBER = -1
NOKIA_INVALID_IP = '0.0.0.0'
NOKIA_INVALID_FIRMWARE_VERSION = '0.0'
NOKIA_INVALID_STRING = 'N/A'

NOKIA_ALLOW_FAN_OVERRIDE_FILE = '/etc/sonic/nokia/allow_fan_override'
NOKIA_TEMP_LOW_CRITICAL_THRESHOLD_MULTIPLIER = 0.1
NOKIA_TEMP_HIGH_CRITICAL_THRESHOLD_MULTIPLIER = 1.1

NOKIA_GRPC_CHASSIS_SERVICE = 'Chassis-Service'
NOKIA_GRPC_PSU_SERVICE = 'PSU-Service'
NOKIA_GRPC_FAN_SERVICE = 'Fan-Service'
NOKIA_GRPC_THERMAL_SERVICE = 'Thermal-Service'
NOKIA_GRPC_XCVR_SERVICE = 'Xcvr-Service'
NOKIA_GRPC_LED_SERVICE = 'Led-Service'
NOKIA_GRPC_FIRMWARE_SERVICE = 'Firmware-Service'
NOKIA_GRPC_UTIL_SERVICE = 'Util-Service'
NOKIA_GRPC_EEPROM_SERVICE = 'Eeprom-Service'


def channel_setup(service):
    server_path = NOKIA_DEVMGR_UNIX_SOCKET_PATH
    if os.path.exists(NOKIA_CHANNEL_FILE_PATH):
        server_path=(open(NOKIA_CHANNEL_FILE_PATH, 'r').readline().rstrip())
    _channel = grpc.insecure_channel(server_path)
    _stub = None

    _channel_ready = grpc.channel_ready_future(_channel)
    try:
        _channel_ready.result(timeout=0.5)
    except grpc.FutureTimeoutError:
        _channel = None
        return _channel, _stub

    if service == NOKIA_GRPC_CHASSIS_SERVICE:
        _stub = platform_ndk_pb2_grpc.ChassisPlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_PSU_SERVICE:
        _stub = platform_ndk_pb2_grpc.PsuPlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_FAN_SERVICE:
        _stub = platform_ndk_pb2_grpc.FanPlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_THERMAL_SERVICE:
        _stub = platform_ndk_pb2_grpc.ThermalPlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_LED_SERVICE:
        _stub = platform_ndk_pb2_grpc.LedPlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_XCVR_SERVICE:
        _stub = platform_ndk_pb2_grpc.XcvrPlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_FIRMWARE_SERVICE:
        _stub = platform_ndk_pb2_grpc.FirmwarePlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_UTIL_SERVICE:
        _stub = platform_ndk_pb2_grpc.UtilPlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_EEPROM_SERVICE:
        _stub = platform_ndk_pb2_grpc.EepromPlatformNdkServiceStub(_channel)

    return _channel, _stub


def channel_shutdown(_channel):
    _channel.close()


def try_grpc(callback, *args, **kwargs):
    """
    Handy function to invoke the callback and catch NotImplementedError
    :param callback: Callback to be invoked
    :param args: Arguments to be passed to callback
    :param kwargs: Default return value if exception occur
    :return: Default return value if exception occur else return value of the callback
    """
    return_val = True
    try:
        resp = callback(*args)
        if resp is None:
            resp_status = platform_ndk_pb2.ResponseStatus(error_msg='No response available')
            resp = platform_ndk_pb2.DefaultResponse(response_status=resp_status)
            return_val = False
    except grpc.RpcError as e:
        status_code = platform_ndk_pb2.ResponseCode.NDK_ERR_FAILURE
        err_msg = 'Grpc error code '+str(e.code())
        resp_status = platform_ndk_pb2.ResponseStatus(status_code=status_code,
                error_msg=err_msg)
        resp = platform_ndk_pb2.DefaultResponse(response_status=resp_status)
        return_val = False

    return return_val, resp


def is_cpm():
    channel, stub = channel_setup(NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return NOKIA_INVALID_SLOT_NUMBER
    response = stub.GetMySlot(platform_ndk_pb2.ReqModuleInfoPb())
    channel_shutdown(channel)
    return (response.my_slot == NOKIA_CPM_SLOT_NUMBER)


def _get_cpm_slot():
    return NOKIA_CPM_SLOT_NUMBER


def _get_my_slot():
    channel, stub = channel_setup(NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return NOKIA_INVALID_SLOT_NUMBER
    response = stub.GetMySlot(platform_ndk_pb2.ReqModuleInfoPb())
    channel_shutdown(channel)
    return response.my_slot


def get_chassis_type():
    channel, stub = channel_setup(NOKIA_GRPC_CHASSIS_SERVICE)
    response = stub.GetChassisType(platform_ndk_pb2.ReqModuleInfoPb())
    channel_shutdown(channel)
    return response.chassis_type


def is_chassis_modular():
    chassis_type = get_chassis_type()
    if ((chassis_type == platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_IXR6) or
        (chassis_type == platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_IXR10)):
        return True
    return False


def led_color_to_info(led_color):
    _state = platform_ndk_pb2.LedStateType.LED_STATE_ON
    _color = platform_ndk_pb2.LedColorType.LED_COLOR_AMBER

    if led_color == DeviceBase.STATUS_LED_COLOR_OFF:
        _state = platform_ndk_pb2.LedStateType.LED_STATE_OFF
    elif led_color == DeviceBase.STATUS_LED_COLOR_GREEN:
        _color = platform_ndk_pb2.LedColorType.LED_COLOR_GREEN
    elif led_color == DeviceBase.STATUS_LED_COLOR_AMBER:
        _color = platform_ndk_pb2.LedColorType.LED_COLOR_AMBER
    elif led_color == DeviceBase.STATUS_LED_COLOR_RED:
        _color = platform_ndk_pb2.LedColorType.LED_COLOR_RED

    _led_info = platform_ndk_pb2.LedSetInfoPb(led_color=_color, led_state=_state)
    return _led_info


def led_info_to_color(led_info):
    _led_color = led_info.led_color
    _led_state = led_info.led_state
    _color = DeviceBase.STATUS_LED_COLOR_OFF

    if _led_state == platform_ndk_pb2.LedStateType.LED_STATE_OFF:
        _color = DeviceBase.STATUS_LED_COLOR_OFF
    elif _led_color == platform_ndk_pb2.LedColorType.LED_COLOR_GREEN:
        _color = DeviceBase.STATUS_LED_COLOR_GREEN
    elif _led_color == platform_ndk_pb2.LedColorType.LED_COLOR_AMBER:
        _color = DeviceBase.STATUS_LED_COLOR_AMBER
    elif _led_color == platform_ndk_pb2.LedColorType.LED_COLOR_RED:
        _color = DeviceBase.STATUS_LED_COLOR_RED

    return _color
