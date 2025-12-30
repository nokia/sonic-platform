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
from sonic_platform_base.module_base import ModuleBase
import grpc
from platform_ndk import platform_ndk_pb2
from platform_ndk import platform_ndk_pb2_grpc
from datetime import datetime

NOKIA_MIDPLANE_SUBNET = "10.6."
NOKIA_UNIX_SOCKET_PREFIX = "unix://"
NOKIA_SONIC_UNIX_SOCKET_FOLDER = "/var/run/redis/"
NOKIA_DEVMGR_SONIC_UNIX_SOCKET_NAME = "devmgr_sonic_server"
NOKIA_DEVMGR_SONIC_SRVR_PORT = "50065"
NOKIA_DEVMGR_SONIC_SRVR_IP = "127.0.0.1"
NOKIA_DEVMGR_MONITOR_UPDATE_PERIOD_SECS = 1
NOKIA_CHANNEL_FILE_PATH = "/tmp/nokia_grpc_server"

NOKIA_DEVMGR_UNIX_SOCKET_PATH = NOKIA_UNIX_SOCKET_PREFIX + \
                                NOKIA_SONIC_UNIX_SOCKET_FOLDER + \
                                NOKIA_DEVMGR_SONIC_UNIX_SOCKET_NAME + ":" + \
                                NOKIA_DEVMGR_SONIC_SRVR_PORT

NOKIA_MIDPLANE_SONIC_SRVR_PORT = "60070"
NOKIA_QFPGA_SONIC_SRVR_PORT = "50067"
NOKIA_MIDPLANE_ETHMGR_SOCKET_PATH = "0.0.0.0:" + \
                                   NOKIA_MIDPLANE_SONIC_SRVR_PORT
NOKIA_QFPGA_SOCKET_PATH = "0.0.0.0:" + \
                                   NOKIA_QFPGA_SONIC_SRVR_PORT
NOKIA_FP_START_PORTID = 0
NOKIA_FP_END_PORTID = 35
NOKIA_MAX_PORTS_PER_ASIC = 12
NOKIA_CPM_SLOT_NUMBER = 0
NOKIA_INVALID_SLOT_NUMBER = -1
NOKIA_MAX_IMM_SLOTS = 8
NOKIA_INVALID_IP = '0.0.0.0'
NOKIA_INVALID_FIRMWARE_VERSION = '0.0'
NOKIA_INVALID_STRING = 'N/A'

NOKIA_ALLOW_FAN_OVERRIDE_FILE = '/etc/sonic/nokia/allow_fan_override'
NOKIA_TEMP_LOW_CRITICAL_THRESHOLD_MULTIPLIER = 0.1
NOKIA_TEMP_HIGH_CRITICAL_THRESHOLD_MULTIPLIER = 1.1
NOKIA_INVALID_TEMP = -128.0
NOKIA_SONIC_PMON_MAX_TEMP_THRESHOLD = 100.0
NOKIA_SONIC_PMON_INIT_TEMP = 30.0

NOKIA_GRPC_CHASSIS_SERVICE = 'Chassis-Service'
NOKIA_GRPC_PSU_SERVICE = 'PSU-Service'
NOKIA_GRPC_FAN_SERVICE = 'Fan-Service'
NOKIA_GRPC_THERMAL_SERVICE = 'Thermal-Service'
NOKIA_GRPC_XCVR_SERVICE = 'Xcvr-Service'
NOKIA_GRPC_LED_SERVICE = 'Led-Service'
NOKIA_GRPC_FIRMWARE_SERVICE = 'Firmware-Service'
NOKIA_GRPC_UTIL_SERVICE = 'Util-Service'
NOKIA_GRPC_EEPROM_SERVICE = 'Eeprom-Service'
NOKIA_GRPC_MIDPLANE_SERVICE = 'Midplane-Service'
NOKIA_GRPC_QFPGA_SERVICE = 'Qfpga-Service'

HW_SLOT_TO_EXTERNAL_SLOT_MAPPING = {
    0: "A",
    1: "1",
    2: "2",
    3: "3",
    4: "4",
    5: "5",
    6: "6",
    7: "7",
    8: "8",
    15: "1",
    16: "2",
    17: "3",
    18: "4",
    19: "5",
    20: "6",
    21: "7",
    22: "8",
    23: "B"
}
    


my_chassis_type = platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_INVALID


def channel_setup(service):
    if service == NOKIA_GRPC_MIDPLANE_SERVICE:
       server_path = NOKIA_MIDPLANE_ETHMGR_SOCKET_PATH
    elif service == NOKIA_GRPC_QFPGA_SERVICE:
       server_path = NOKIA_QFPGA_SOCKET_PATH
    else:
       server_path = NOKIA_DEVMGR_UNIX_SOCKET_PATH

    if os.path.exists(NOKIA_CHANNEL_FILE_PATH):
        server_path = (open(NOKIA_CHANNEL_FILE_PATH, 'r').readline().rstrip())
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
    elif service == NOKIA_GRPC_MIDPLANE_SERVICE:
        _stub = platform_ndk_pb2_grpc.MidplanePlatformNdkServiceStub(_channel)
    elif service == NOKIA_GRPC_QFPGA_SERVICE:
        _stub = platform_ndk_pb2_grpc.QfpgaPlatformNdkServiceStub(_channel)

    return _channel, _stub

def midplane_channel_setup(service, hw_slot):
    midplane_ip = NOKIA_MIDPLANE_SUBNET + str(hw_slot) + '.100' + ':'
    if service == NOKIA_GRPC_MIDPLANE_SERVICE:
      #Midplane/Ethmgr is supported only in CPM
      return None, None
    elif service == NOKIA_GRPC_QFPGA_SERVICE:
       server_path = midplane_ip + NOKIA_QFPGA_SONIC_SRVR_PORT
    else:
        server_path = midplane_ip + NOKIA_DEVMGR_SONIC_SRVR_PORT

    if os.path.exists(NOKIA_CHANNEL_FILE_PATH):
        server_path = (open(NOKIA_CHANNEL_FILE_PATH, 'r').readline().rstrip())
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
    elif service == NOKIA_GRPC_QFPGA_SERVICE:
        _stub = platform_ndk_pb2_grpc.QfpgaPlatformNdkServiceStub(_channel)
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
        return False
    ret, response = try_grpc(stub.GetMySlot, platform_ndk_pb2.ReqModuleInfoPb())
    channel_shutdown(channel)

    if ret is False:
        return False

    return (response.my_slot == NOKIA_CPM_SLOT_NUMBER)


def _get_cpm_slot():
    return NOKIA_CPM_SLOT_NUMBER


def _get_my_slot():
    channel, stub = channel_setup(NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return NOKIA_INVALID_SLOT_NUMBER
    ret, response = try_grpc(stub.GetMySlot, platform_ndk_pb2.ReqModuleInfoPb())
    channel_shutdown(channel)

    if ret is False:
        return NOKIA_INVALID_SLOT_NUMBER

    return response.my_slot

def hw_slot_to_external_slot(slot):
    if slot in HW_SLOT_TO_EXTERNAL_SLOT_MAPPING:
        return HW_SLOT_TO_EXTERNAL_SLOT_MAPPING[slot]
    else:
        return str(slot)

def get_chassis_type():
    global my_chassis_type
    channel, stub = channel_setup(NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return my_chassis_type
    ret, response = try_grpc(stub.GetChassisType, platform_ndk_pb2.ReqModuleInfoPb())
    channel_shutdown(channel)

    if ret is False:
        return my_chassis_type

    my_chassis_type = response.chassis_type
    return my_chassis_type


def is_chassis_modular():
    chassis_type = get_chassis_type()
    if ((chassis_type == platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_IXR6) or
        (chassis_type == platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_IXR10) or
        (chassis_type == platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_IXR6E) or
        (chassis_type == platform_ndk_pb2.HwChassisType.HW_CHASSIS_TYPE_IXR10E)):
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


def hw_module_status_name(status_type):
    status_ = ModuleBase.MODULE_STATUS_EMPTY

    if status_type == platform_ndk_pb2.HwModuleStatus.HW_MODULE_STATUS_EMPTY:
        status_ = ModuleBase.MODULE_STATUS_EMPTY
    elif status_type == platform_ndk_pb2.HwModuleStatus.HW_MODULE_STATUS_OFFLINE:
        status_ = ModuleBase.MODULE_STATUS_OFFLINE
    elif status_type == platform_ndk_pb2.HwModuleStatus.HW_MODULE_STATUS_POWERED_DOWN:
        status_ = ModuleBase.MODULE_STATUS_POWERED_DOWN
    elif status_type == platform_ndk_pb2.HwModuleStatus.HW_MODULE_STATUS_PRESENT:
        status_ = ModuleBase.MODULE_STATUS_PRESENT
    elif status_type == platform_ndk_pb2.HwModuleStatus.HW_MODULE_STATUS_FAULT:
        status_ = ModuleBase.MODULE_STATUS_FAULT
    elif status_type == platform_ndk_pb2.HwModuleStatus.HW_MODULE_STATUS_ONLINE:
        status_ = ModuleBase.MODULE_STATUS_ONLINE

    return status_

def _cpm_reboot_IMMs(imm_slot):
    channel, stub = channel_setup(NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return False
    response = stub.RebootSlot(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=imm_slot))
    channel_shutdown(channel)
    return True

def _reboot_IMMs_via_midplane(imm_slot, reboot_type_=None):
    time_now = datetime.now()
    if reboot_type_ == "PMON_API":
        user = "PMON_API"
    else:
        try:
            user = os.getlogin()
        except Exception:
            user = "Unknown"

    reboot_reason_ = ("User issued 'reboot from Supervisor' command [User: " + user);
    channel, stub = midplane_channel_setup(NOKIA_GRPC_CHASSIS_SERVICE, imm_slot)
    if not channel or not stub:
        return False
    reboot_cause = platform_ndk_pb2.reboot_cause(reboot_type=reboot_type_, reboot_reason=reboot_reason_)
    response = stub.RebootSlot(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=imm_slot,reboot_type_reason=reboot_cause))
    if response.response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
        print('Rebooting IMM {} via midplane failed'.format(imm_slot))
        rv = False
    else:
        print('Rebooting - IMM {} - slot reboot requested via midplane'.format(imm_slot))
        rv = True
    channel_shutdown(channel)
    return rv

def _force_reboot_IMMs_via_midplane(imm_slot, reboot_type_=None):
    # Use this reboot_reason to trigger the NDK /opt/srlinux/bin/reboot_platform.sh
    time_now = datetime.now()
    if reboot_type_ == "PMON_API":
        user = "PMON_API"
    else:
        try:
            user = os.getlogin()
        except Exception:
            user = "Unknown"

    reboot_reason_ = "Unable to reach CPM. Supervisor forces reboot IMM"
    channel, stub = midplane_channel_setup(NOKIA_GRPC_CHASSIS_SERVICE, imm_slot)
    if not channel or not stub:
        return False
    reboot_cause = platform_ndk_pb2.reboot_cause(reboot_type=reboot_type_, reboot_reason=reboot_reason_)
    response = stub.RebootSlot(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=imm_slot,reboot_type_reason=reboot_cause))
    if response.response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
        print('Force Rebooting IMM {} via midplane failed'.format(imm_slot))
        rv = False
    else:
        print('Force Rebooting - IMM {} - slot reboot requested via midplane'.format(imm_slot))
        rv = True
    channel_shutdown(channel)
    return rv

def _reboot_IMMs(reboot_type=None):
    for imm_slot in range(1, NOKIA_MAX_IMM_SLOTS+1):        
        ret = _reboot_IMMs_via_midplane(imm_slot, reboot_type)
        if ret == False:
            _cpm_reboot_IMMs(imm_slot)
    return True

def _reboot_imm(slot):
    if slot < 1 or slot > NOKIA_MAX_IMM_SLOTS:
        return
    ret = _reboot_IMMs_via_midplane(slot)
    if ret == False:
        ret = _cpm_reboot_IMMs(slot)
    return ret

def _restart_lc_system_service(imm_slot, service_file):
    if imm_slot < 1 or imm_slot > NOKIA_MAX_IMM_SLOTS:
        return
    channel, stub = midplane_channel_setup(NOKIA_GRPC_CHASSIS_SERVICE, imm_slot)
    if not channel or not stub:
        return False
    response = stub.RestartSystemService(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=imm_slot, service_name=service_file))
    if response.response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
        print('Failed to restart Linecard Slot {} system service \"{}\"'.format(imm_slot, service_file))
        rv = False
    else:
        print('Restart Linecard Slot {} system service \"{}\" success'.format(imm_slot, service_file))
        rv = True
    channel_shutdown(channel)
    return rv

def _force_reboot_imm(slot):
    if slot < 1 or slot > NOKIA_MAX_IMM_SLOTS:
        return
    ret = _force_reboot_IMMs_via_midplane(slot)
    if ret == False:
        ret = _cpm_reboot_IMMs(slot)
    return ret

def _power_onoff_SFM(hw_slot, powerOn):
    channel, stub = channel_setup(NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return False
    if powerOn is True:
        power_status_ = platform_ndk_pb2.SetHwModulePowerStatus(power_status=platform_ndk_pb2.HWModulePowerStatus.HW_MODULE_POWER_ON)
    else:
        power_status_ = platform_ndk_pb2.SetHwModulePowerStatus(power_status=platform_ndk_pb2.HWModulePowerStatus.HW_MODULE_POWER_OFF)
    response = stub.PowerOnOffSlot(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=hw_slot, set_power_status=power_status_))

    channel_shutdown(channel)
    return True

def _get_sfm_eeprom_info_list():
    """
    Get all sfm eeprom info into a list.
    """
    channel, stub = channel_setup(NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    req_etype = platform_ndk_pb2.ReqSfmOpsType.SFM_OPS_SHOW_EEPROM
    response = stub.ReqSfmInfo(platform_ndk_pb2.ReqSfmInfoPb(type=req_etype))
    channel_shutdown(channel)
    return response.sfm_eeprom.eeprom_info
    
def _tx_disable_all_sfps():
    op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_NORMAL
    
    channel, stub = channel_setup(NOKIA_GRPC_XCVR_SERVICE)
    if not channel or not stub:
        return False
    ret, response = try_grpc(stub.ReqSfpTxDisable,
                             platform_ndk_pb2.ReqSfpOpsPb(type=op_type,
                                                          hw_port_id_begin=0xffff,
                                                          val=False))
    channel_shutdown(channel)

    if ret is False:
        print("Fail to send to disable all SFPs tx")
        return False
    status = response.sfp_status.status
    if status == 0:
        return True
    else:
        return False
        
