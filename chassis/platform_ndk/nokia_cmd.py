# Name: nokia_cmd.py, version: 1.0
#
# Description: Module contains the test show, set and ut commands
# for interfacing with platform code
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

import argparse
import json
from google.protobuf.json_format import MessageToJson
import subprocess
import time
import os

from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
from sonic_py_common.logger import Logger

logger=Logger("nokia_cmd")

eeprom_default_dict = {
    "0x21": "Product Name",
    "0x22": "Part Number",
    "0x23": "Serial Number",
    "0x24": "Base MAC Address",
    "0x25": "Manufacture Date",
    "0x26": "Device Version",
    "0x27": "Label Revision",
    "0x28": "Platform Name",
    "0x29": "ONIE Version",
    "0x2A": "MAC Addresses",
    "0x2B": "Manufacturer",
    "0x2C": "Manufacture Country",
    "0x2D": "Vendor Name",
    "0x2E": "Diag Version",
    "0x2F": "Service Tag",
    "0xFC": "Undefined",
    "0xFD": "Vendor Extension",
    "0xFE": "CRC-32"
}

PROJECT_NAME = 'nokia_ixr7250'
verbose = False
DEBUG = False
args = []
format_type = ''

sfm_asic_dict = {
    1: [0,1],
    2: [2,3],
    3: [4,5],
    4: [6,7],
    5: [8,9],
    6: [10,11],
    7: [12,13],
    8: [14,15]
}

sfm_hw_slot_mapping = {
    1: 15,
    2: 16,
    3: 17,
    4: 18,
    5: 19,
    6: 20,
    7: 21,
    8: 22
}

qfpga_cli_port_name_dict = {
    "Port0": "cpm-A",
    "Port1": "cpm-B",
    "Port2": "qfpgap0/eth1-midplane",
    "Port3": "qfpgap1/eth0",
}

qfpga_ndk_port_name_dict = {
    "ALL_PORTS": "ALL_PORTS",
    "cpm-A": "cpma",
    "cpm-B": "cpmb",
    "qfpgap0": "eth1",
    "eth1-midplane": "eth1",
    "qfpgap1": "eth0",
    "eth0": "eth0",
}

def pretty_time_delta(seconds):
    sign_string = '-' if seconds < 0 else ''
    seconds = abs(int(seconds))
    days, seconds = divmod(seconds, 86400)
    hours, seconds = divmod(seconds, 3600)
    minutes, seconds = divmod(seconds, 60)
    if days > 0:
        return '%s %dd %dh %dm %ds' % (sign_string, days, hours, minutes, seconds)
    elif hours > 0:
        return '%s %dh %dm %ds' % (sign_string, hours, minutes, seconds)
    elif minutes > 0:
        return '%s %dm %ds' % (sign_string, minutes, seconds)
    else:
        return '%s %ds' % (sign_string, seconds)


def print_table(field, item_list):
    i = 0
    header = ''
    while i < len(field):
        header += "| "+field[i]
        i += 1
    header += " | "
    print('-' * (len(header) - 1))
    print(header)
    print('-' * (len(header) - 1))

    i = 0
    while i < len(item_list):
        item = item_list[i]
        row = ''
        j = 0
        while j < len(item):
            row += "| "+item[j]+' '*(len(field[j]) - len(item[j]))
            j += 1
        row += " |"
        print(row)
        i += 1
    print('-' * (len(header) - 1))
    print('\n')


def show_led():
    port_start = 0
    port_end = 35
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
    if not channel or not stub:
        return
    led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_PORT
    port_idx = platform_ndk_pb2.ReqLedIndexPb(start_idx=port_start,
                                              end_idx=port_end)
    response = stub.ShowLed(platform_ndk_pb2.ReqLedInfoPb(led_type=led_type, led_idx=port_idx))
    nokia_common.channel_shutdown(channel)

    if len(response.led_show.show_info) == 0:
        print('Port/LED Output not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('Interface     ')
    field.append('Status                    ')


    i = 0
    item_list = []
    while i < len(response.led_show.show_info):
        led_device = response.led_show.show_info[i]
        item_list.append([led_device.led_name,
                         (led_device.led_status.rstrip())])
        i += 1

    print('FRONT-PANEL INTERFACE STATUS TABLE')
    print_table(field, item_list)
    return


def show_psu():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
    if not channel or not stub:
        return

    arg1 = platform_ndk_pb2.ReqPsuInfoPb(psu_idx=0)
    ret, response = nokia_common.try_grpc(stub.ShowPsuInfo, arg1)
    nokia_common.channel_shutdown(channel)

    if ret is False:
        print('PSU request unsuccessful on this card')
        return

    if len(response.psu_info.psu_device) == 0:
        print('PSU Output not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('Index   ')
    field.append('Status       ')

    i = 0
    item_list = []
    while i < len(response.psu_info.psu_device):
        psu_device = response.psu_info.psu_device[i]
        item = []
        item.append(str(i + 1))
        if psu_device.psu_presence:
            if psu_device.psu_status:
                item.append('UP')
            else:
                item.append('DOWN')
        else:
            item.append('NOT-PRESENT')
        item_list.append(item)
        i += 1

    print('PSU TABLE')
    print_table(field, item_list)
    return


def show_psu_detail():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_PSU_SERVICE)
    if not channel or not stub:
        return

    response = stub.ShowPsuInfo(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=0))
    if len(response.psu_info.psu_device) == 0:
        nokia_common.channel_shutdown(channel)
        print('PSU Output not available on this card')
        return

    field = []
    field.append('Index   ')
    field.append('Current      ')
    field.append('Voltage      ')
    field.append('Power        ')
    field.append('Temperature  ')

    i = 0
    item_list = []
    while i < len(response.psu_info.psu_device):
        psu_device = response.psu_info.psu_device[i]
        if psu_device.psu_presence:
            item = []
            item.append(str(i + 1))
            p_response = stub.GetPsuOutputCurrent(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=i+1))
            item.append(str(round(p_response.output_current, 2)))
            p_response = stub.GetPsuOutputVoltage(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=i+1))
            item.append(str(round(p_response.output_voltage, 2)))
            p_response = stub.GetPsuOutputPower(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=i+1))
            item.append(str(p_response.output_power))
            p_response = stub.GetPsuTemperature(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=i+1))
            item.append(str(p_response.ambient_temp))
            item_list.append(item)
        i += 1

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
        json_list = []
        for item in item_list:
            dict_item = {f.rstrip(): v for f, v in zip(field, item)}
            json_list.append(dict_item)

        print(json.dumps(json_list, indent=4))
        return

    print('PSU TABLE DETAILS')
    print_table(field, item_list)
    return


def show_fan():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FAN_SERVICE)
    if not channel or not stub:
        return

    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=0)
    response = stub.ShowFanInfo(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))

    nokia_common.channel_shutdown(channel)

    if len(response.fan_show.fan_device) == 0:
        print('Fan output not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('        Type         ')
    field.append('        Value                        ')

    i = 0
    item_list = []
    while i < len(response.fan_show.fan_device):
        fan_device = response.fan_show.fan_device[i]
        item = []
        item.append('Fan Tray - '+str(i))
        item.append(fan_device.fan_state)
        item_list.append(item)
        i += 1

    item_list.append(['Fan Algorithm Disable', 'True' if response.fan_show.fan_algorithm_disable else 'False'])
    item_list.append(['Current Fan Speed', str(response.fan_show.current_fan_speed)])
    item_list.append(['Max Fan Speed', str(response.fan_show.max_fan_speed)])
    item_list.append(['Force Max Fan Speed', 'True' if response.fan_show.is_force_max_speed else 'False'])

    print('FAN TABLE')
    print_table(field, item_list)
    return


def show_fan_detail():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FAN_SERVICE)
    if not channel or not stub:
        return

    # Get Num-fans
    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=0)
    response = stub.GetFanNum(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))

    fan_msg = response.fan_nums
    num_fans = fan_msg.num_fantrays

    if num_fans == 0:
        nokia_common.channel_shutdown(channel)
        print('Fan detail output not available on this card')
        return

    field = []
    field.append('Index   ')
    field.append('Presence')
    field.append('Status  ')
    field.append('Target Speed')
    field.append('Speed   ')
    field.append('SerialNo      ')
    field.append('PartNo        ')
    field.append('Led-Color     ')

    item_list = []
    # For each fan check presence
    for index in range(0, num_fans):
        req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=index)

        # Presence
        response = stub.GetFanPresence(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
        presence_msg = response.fan_presence
        presence = presence_msg.fantray_presence

        # Status
        status = False
        response = stub.GetFanStatus(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
        status_msg = response.fan_status
        # Empty, Init, SpinUp, Online are possible states
        if status_msg.fantray_status == 'Online':
            status = True

        # Target Speed
        response = stub.GetFanTargetSpeed(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
        speed_msg = response.fan_speed_target
        target_speed = speed_msg.fantray_speed

        # Speed
        response = stub.GetFanActualSpeed(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
        speed_msg = response.fan_speed_actual
        speed = speed_msg.fantray_speed

        # PartNo
        response = stub.GetFanTrayPartNo(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
        fantray_partno = response.fan_eeprom.fantray_edata

        # SerialNo
        response = stub.GetFanTraySerialNo(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
        fantray_serialno = response.fan_eeprom.fantray_edata

        # LED
        response = stub.GetFanLedStatus(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
        color = nokia_common.led_info_to_color(response.led_info)

        item = []
        item.append(str(index))
        item.append(str(presence))
        item.append(str(status))
        item.append(str(target_speed))
        item.append(str(speed))
        item.append(fantray_serialno)
        item.append(fantray_partno)
        item.append(color)
        item_list.append(item)

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
        json_list = []
        for item in item_list:
            dict_item = {f.rstrip(): v for f, v in zip(field, item)}
            json_list.append(dict_item)

        print(json.dumps(json_list, indent=4))
        return
    print('FAN DETAIL TABLE')
    print_table(field, item_list)
    return


def show_temp():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
    if not channel or not stub:
        return

    response = stub.ShowThermalInfo(platform_ndk_pb2.ReqTempParamsPb())

    nokia_common.channel_shutdown(channel)
    
    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('Name    ')
    field.append('Local   ')
    field.append('Remote   ')
    field.append('Description        ')

    i = 0
    item_list = []
    while i < len(response.temp_show.temp_device):
        temp_device = response.temp_show.temp_device[i]
        item = []
        item.append(temp_device.sensor_name)
        item.append(str(temp_device.local))
        item.append(str(temp_device.remote))
        item.append(temp_device.device_desc)
        item_list.append(item)
        i += 1

    print('TEMPERATURE DEVICES TABLE')
    print_table(field, item_list)

    field = []
    field.append('Hw Slot')
    field.append('Current Temp')
    field.append('Min Temp')
    field.append('Max Temp')
    field.append('Margin')

    i = 0
    item_list = []
    while i < len(response.temp_show.temp_summary):
        temp_summary = response.temp_show.temp_summary[i]
        item = []
        item.append(str(temp_summary.slot_num))
        item.append(str(temp_summary.current_temp))
        item.append(str(temp_summary.min_temp))
        item.append(str(temp_summary.max_temp))
        item.append(str(temp_summary.margin))
        item_list.append(item)
        i += 1

    print('TEMPERATURE SUMMARY TABLE')
    print_table(field, item_list)

    return


def show_platform():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return

    response = stub.GetChassisStatus(platform_ndk_pb2.ReqModuleInfoPb())

    if format_type == 'json-format':
        nokia_common.channel_shutdown(channel)
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('Slot   ')
    field.append('Name                              ')
    field.append('Status    ')
    field.append('Midplane-IP       ')

    i = 0
    item_list = []
    while i < len(response.hw_status_info.device_status):
        hw_device = response.hw_status_info.device_status[i]
        item = []
        item.append(str(hw_device.slot_num))
        item.append(hw_device.hw_name)
        item.append(nokia_common.hw_module_status_name(hw_device.status))
        m_response = stub.GetMidplaneIP(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=hw_device.slot_num))
        item.append(m_response.midplane_ip)
        item_list.append(item)
        i += 1

    nokia_common.channel_shutdown(channel)

    print('PLATFORM INFO TABLE')
    print_table(field, item_list)
    return


def show_firmware():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FIRMWARE_SERVICE)
    if not channel or not stub:
        return

    response = stub.HwFirmwareGetComponents(platform_ndk_pb2.ReqHwFirmwareInfoPb())

    nokia_common.channel_shutdown(channel)

    if len(response.firmware_info.component) == 0:
        print('Firmware Info not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('Component                 ')
    field.append('Name        ')
    field.append('Description                                     ')

    i = 0
    item_list = []
    while i < len(response.firmware_info.component):
        component = response.firmware_info.component[i]
        item = []
        item.append(platform_ndk_pb2.HwFirmwareDeviceType.Name(component.dev_type))
        item.append(component.dev_name)
        item.append(component.dev_desc)
        item_list.append(item)
        i += 1

    print('FIRMWARE INFO TABLE')
    print_table(field, item_list)
    return


def show_sfm_summary():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    req_etype = platform_ndk_pb2.ReqSfmOpsType.SFM_OPS_SHOW_SUMMARY
    response = stub.ReqSfmInfo(platform_ndk_pb2.ReqSfmInfoPb(type=req_etype))

    nokia_common.channel_shutdown(channel)

    if len(response.sfm_summary.sfm_info) == 0:
        print('SFM summary not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('SFM-Num')
    field.append('Hw-slot')
    field.append('Admin Down')
    field.append('Initialized')
    field.append('Num Ramons')
    field.append('Presence')
    field.append('Error')
    field.append('Instance')
    field.append('Failed')

    i = 0
    item_list = []
    while i < len(response.sfm_summary.sfm_info):
        sfm_info = response.sfm_summary.sfm_info[i]
        item = []
        item.append(str(sfm_info.sfm_num))
        item.append(str(sfm_info.hw_slot))
        item.append(str(sfm_info.admin_down))
        item.append(str(sfm_info.is_initialized))
        item.append(str(sfm_info.num_ramons))
        item.append(str(sfm_info.presence))
        item.append(str(sfm_info.error))
        item.append(str(sfm_info.instance))
        item.append(str(sfm_info.failed))
        item_list.append(item)
        i += 1

    print('SFM SUMMARY TABLE')
    print_table(field, item_list)
    return

def show_sfm_eeprom():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    req_etype = platform_ndk_pb2.ReqSfmOpsType.SFM_OPS_SHOW_EEPROM
    response = stub.ReqSfmInfo(platform_ndk_pb2.ReqSfmInfoPb(type=req_etype))

    nokia_common.channel_shutdown(channel)

    if len(response.sfm_eeprom.eeprom_info) == 0:
        print('SFM Eeprom not available')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response,including_default_value_fields=True)
        print(json_response)
        return

    field = []
    field.append('SFM-Num')
    field.append('Hwsku')
    field.append('Num Asics')
    field.append('     Name     ')
    field.append('Assembly Num')
    field.append('Deviation')
    field.append(' Mfg Date ')
    field.append('  Serial Num  ')
    field.append('  Part Num    ')
    field.append('Platform')
    field.append('Hw Directive') 

    i = 0
    item_list = []
    while i < len(response.sfm_eeprom.eeprom_info):
        eeprom_info = response.sfm_eeprom.eeprom_info[i]
        item = []
        item.append(str(eeprom_info.sfm_num))
        item.append(str(eeprom_info.hwsku))
        item.append(str(eeprom_info.num_asics))
        item.append(str(eeprom_info.name))
        item.append(str(eeprom_info.eeprom_assembly_num))
        item.append(str(eeprom_info.eeprom_deviation))
        item.append(str(eeprom_info.eeprom_date))
        item.append(str(eeprom_info.eeprom_serial))
        item.append(str(eeprom_info.eeprom_part))
        item.append(str(eeprom_info.eeprom_platform))
        item.append(str(eeprom_info.eeprom_hw_directive))
        item_list.append(item)
        i += 1

    print('SFM EEPROM TABLE')
    print_table(field, item_list)
    return

def show_sfm_imm_links(imm_slot):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    req_etype = platform_ndk_pb2.ReqSfmOpsType.SFM_OPS_SHOW_IMMLINKS
    response = stub.ReqSfmInfo(platform_ndk_pb2.ReqSfmInfoPb(type=req_etype, imm_slot=imm_slot))

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('IMM-Link    ')
    field.append('SFM-Link    ')
    field.append('Link Status')
    field.append('Isolate')
    field.append('Module-Id')

    i = 0
    item_list = []
    while i < len(response.sfm_imm_info.link_info):
        link_info = response.sfm_imm_info.link_info[i]
        item = []
        imm_link = 'IMM ' + str(response.sfm_imm_info.imm_slot) +\
                   '.' + str(link_info.imm_unit) + '.' + str(link_info.imm_link)
        sfm_link = 'SFM ' + str(response.sfm_imm_info.sfm_slot) +\
                   '.' + str(link_info.sfm_unit) + '.' + str(link_info.sfm_link)

        item.append(imm_link)
        item.append(sfm_link)
        if link_info.link_status_up:
            item.append('UP')
        else:
            item.append('DOWN')
        item.append(str(link_info.link_is_isolate))
        item.append('M'+str(link_info.link_module_id))
        item_list.append(item)
        i += 1

    print('SFM IMM LINK TABLE')
    print_table(field, item_list)

    print('Num links UP: '+str(response.sfm_imm_info.up_links))
    print('Num links total: '+str(response.sfm_imm_info.total_links))
    return


def show_system_led():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
    if not channel or not stub:
        return

    led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_ALL
    response = stub.ShowLed(platform_ndk_pb2.ReqLedInfoPb(led_type=led_type))

    nokia_common.channel_shutdown(channel)

    if len(response.led_show.show_info) == 0:
        print('LED Output not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('Name                  ')
    field.append('Status                    ')

    i = 0
    item_list = []
    while i < len(response.led_show.show_info):
        led_device = response.led_show.show_info[i]
        item_list.append([led_device.led_name,
                         (led_device.led_status.rstrip())])
        i += 1

    print('SYSTEM LED STATUS TABLE')
    print_table(field, item_list)
    return


def show_syseeprom():
    import sonic_platform.platform
    chassis = sonic_platform.platform.Platform().get_chassis()
    eeprom = chassis.get_system_eeprom_info()

    field = []
    field.append('      Field Name      ')
    field.append(' Type ')
    field.append('        Value         ')
    item_list = []
    for key in eeprom:
        item = []
        name = eeprom_default_dict.get(key, "Undefined")
        item.append(name)
        item.append(key)
        item.append(eeprom[key])
        item_list.append(item)

    print("SYSTEM EEPROM TABLE")
    if format_type == 'json-format':
        json_list = []
        json_field = []
        json_field.append('Field Name')
        json_field.append('Type')
        json_field.append('Value')
        for item in item_list:
            dict_item = {f.rstrip(): v for f, v in zip(json_field, item)}
            json_list.append(dict_item)
        print(json.dumps(json_list, indent=4))
    else:
        print_table(field, item_list)

def show_chassis():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return

    response = stub.GetChassisProperties(platform_ndk_pb2.ReqModuleInfoPb())

    nokia_common.channel_shutdown(channel)

    if response.response_status.status_code == platform_ndk_pb2.ResponseCode.NDK_ERR_INVALID_REQ:
        print('{}'.format(response.response_status.error_msg))
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('ModuleType                ')
    field.append('Num-Modules       ')
    field.append('Valid-Slots           ')

    i = 0
    item_list = []
    while i < len(response.chassis_property.hw_property):
        hw_property = response.chassis_property.hw_property[i]
        item = []
        item.append(platform_ndk_pb2.HwModuleType.Name(hw_property.module_type))
        item.append(str(hw_property.max_num))
        slot_list = []
        for j in range(len(hw_property.slot)):
            slot_list.append(str(hw_property.slot[j]))
        item.append(', '.join(slot_list))
        item_list.append(item)
        i += 1

    print('CHASSIS INFO TABLE')
    print_table(field, item_list)

    return


def show_power():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return

    response = stub.GetModuleMaxPower(platform_ndk_pb2.ReqModuleInfoPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('ModuleType                ')
    field.append('Power      ')

    i = 0
    item_list = []
    while i < len(response.power_info.module_power):
        module_info = response.power_info.module_power[i]
        item = []
        item.append(platform_ndk_pb2.HwModuleType.Name(module_info.module_type))
        item.append(str(module_info.module_maxpower))
        item_list.append(item)
        i += 1

    print('CHASSIS POWER TABLE')
    print_table(field, item_list)


def show_midplane_status(hw_slot):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return

    response = stub.IsMidplaneReachable(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=hw_slot))

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    print('Reachability to slot {} is {}'.format(hw_slot, response.midplane_status))
    return


def show_logging():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    response = stub.ShowLogAll(platform_ndk_pb2.ReqLogSettingPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('Module     ')
    field.append('DefaultLevel      ')
    field.append('CurrentLevel      ')

    i = 0
    item_list = []
    while i < len(response.log_show.show_info):
        show_info = response.log_show.show_info[i]
        item = []
        item.append(str(show_info.module))
        item.append(str(show_info.level_default))
        item.append(str(show_info.level_current))
        item_list.append(item)
        i += 1

    print('LOGGING INFORMATION')
    print_table(field, item_list)
    return


def show_ndk_eeprom():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_EEPROM_SERVICE)
    if not channel or not stub:
        return

    response1 = stub.GetCardProductName(platform_ndk_pb2.ReqEepromInfoPb())
    response2 = stub.GetCardSerialNumber(platform_ndk_pb2.ReqEepromInfoPb())
    response3 = stub.GetCardPartNumber(platform_ndk_pb2.ReqEepromInfoPb())
    response4 = stub.GetCardBaseMac(platform_ndk_pb2.ReqEepromInfoPb())
    response5 = stub.GetCardMacCount(platform_ndk_pb2.ReqEepromInfoPb())

    field = []
    field.append('Field                     ')
    field.append('Value                     ')

    item_list = []
    item_list.append(['Card ProductName', str(response1.card_product_name)])
    item_list.append(['Card SerialNum', str(response2.card_serial_num)])
    item_list.append(['Card PartNum', str(response3.card_part_num)])
    item_list.append(['Card BaseMac', str(response4.card_base_mac)])
    item_list.append(['Card MacCount', str(response5.card_mac_count)])
    
    print('CARD-EEPROM INFORMATION')
    if format_type == 'json-format':
        dict_item = {}
        for item in item_list:
            dict_item[item[0]] = item[1]
            # dict_item = {f.rstrip(): v for f, v in zip(field, item)}
            #json_list.append(dict_item)
        print(json.dumps(dict_item, indent=4))
    else:        
        print_table(field, item_list)

    response = stub.GetChassisEeprom(platform_ndk_pb2.ReqEepromInfoPb())

    nokia_common.channel_shutdown(channel)

    if response.response_status.status_code == platform_ndk_pb2.ResponseCode.NDK_ERR_INVALID_REQ:
        print('{}'.format(response.response_status.error_msg))
        return

    item_list = []
    chassis_eeprom = response.chassis_eeprom

    item_list.append(['Chassis Type', str(platform_ndk_pb2.HwChassisType.Name(chassis_eeprom.chassis_type))])
    item_list.append(['Chassis SerialNum', str(chassis_eeprom.chassis_serial_num)])
    item_list.append(['Chassis PartNum', str(chassis_eeprom.chassis_part_num)])
    item_list.append(['Chassis CleiNum', str(chassis_eeprom.chassis_clei_num)])
    item_list.append(['Chassis MfgDate', str(chassis_eeprom.chassis_mfg_date)])
    item_list.append(['Chassis BaseMac', str(chassis_eeprom.chassis_base_mac)])
    item_list.append(['Chassis MacCount', str(chassis_eeprom.chassis_mac_count)])

    print("")
    print('CHASSIS-EEPROM INFORMATION')
    if format_type == 'json-format':
        dict_item = {}
        for item in item_list:
            dict_item[item[0]] = item[1]
        print(json.dumps(dict_item, indent=4))
    else:
        print_table(field, item_list)

def show_ndk_version():
    ndk_version_file = "/etc/opt/srlinux/ndk-version"
    print("NDK Version:")
    if os.path.isfile(ndk_version_file):
        with open(ndk_version_file) as f:
            for line in f.readlines():
                print("{}".format(line))
    else:
        print("File {} not found".format(ndk_version_file))
    print("NDK Build Info:")
    # Process state
    process = subprocess.Popen(['/opt/srlinux/bin/sr_platform_ndk_cli', '-c', 'Cli::GetVersionJson'],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    outstr = stdout.decode('ascii')
    print(outstr)

def show_ndk_status():
    import datetime

    # Get platform from machine.conf
    is_cpm = False
    platform = 'unknown'
    with open('/host/machine.conf') as f:
        for line in f.readlines():
            k, v = line.rstrip("\n").split("=")
            if (k == 'onie_platform'):
                platform = v
                break

    if ((platform == 'x86_64-nokia_ixr7250_cpm-r0') or (platform == 'x86_64-nokia_ixr7250e_sup-r0')):
        is_cpm = True

    if is_cpm:
        proc_list = ['nokia-sr-device-mgr', 'nokia-eth-mgr', 'nokia-watchdog']

    else:
        proc_list = ['nokia-sr-device-mgr', 'nokia-ndk-qfpga-mgr', 'nokia-watchdog', 'nokia-phy-mgr']
    item_list = []
    for proc_item in proc_list:
        item = []

        # Process state
        process = subprocess.Popen(['systemctl', 'show', proc_item],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        outstr = stdout.decode('ascii')
        test = dict(item.split("=", 1) for item in outstr.splitlines())
        if test['LoadState'] == 'not-found':
            continue
        item.append(proc_item)
        item.append(test['ActiveState'])
        item.append(test['MainPID'])
        item.append(test['NRestarts'])

        # Timestamp
        if test['ActiveState'] == 'active':
            process = subprocess.Popen(['systemctl', 'show', proc_item, '--property=ActiveEnterTimestamp'],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        else:
            process = subprocess.Popen(['systemctl', 'show', proc_item, '--property=InactiveEnterTimestamp'],
                                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        outstr = stdout.decode('ascii')
        f, v = outstr.rstrip("UTC \n").split(' ', 1)
        start_time_utc = datetime.datetime.strptime(v, '%Y-%m-%d %H:%M:%S')
        current_time_utc = datetime.datetime.utcnow()
        uptime = (current_time_utc - start_time_utc)
        item.append(pretty_time_delta(uptime.seconds))

        # Add it to the item list
        item_list.append(item)

    field = []
    field.append('Service                 ')
    field.append('State       ')
    field.append('PID     ')
    field.append('RestartCount')
    field.append('Uptime/Exittime   ')

    if format_type == 'json-format':
        json_list = []
        for item in item_list:
            dict_item = {f.rstrip(): v for f, v in zip(field, item)}
            json_list.append(dict_item)

        print(json.dumps(json_list, indent=4))
        return

    print('PLATFORM NDK STATUS INFORMATION')
    print_table(field, item_list)


def show_fabric_pcieinfo(hw_slot):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_CHASSIS_SERVICE)
    if not channel or not stub:
        return

    response = stub.GetFabricPcieInfo(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=hw_slot))

    nokia_common.channel_shutdown(channel)
    if response.response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS :
        print('{}'.format(response.response_status.error_msg))
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response, True)
        print(json_response)
        return

    field = []
    field.append('ASIC IDX    ')
    field.append('ASIC PCIE ID  ')
    item_list = []
    i = 0
    while i < len(response.pcie_info.asic_entry):
        asic_info = response.pcie_info.asic_entry[i]
        item = []
        item.append(str(asic_info.asic_idx))
        item.append(str(asic_info.asic_pcie_id))
        item_list.append(item)
        i += 1

    print('FABRIC PCIE INFORMATION')
    print_table(field, item_list)
    return


def request_devmgr_admintech(filepath):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    stub.ReqAdminTech(platform_ndk_pb2.ReqAdminTechPb(admintech_path=filepath))
    nokia_common.channel_shutdown(channel)

def request_ndk_admintech():
    process = subprocess.call(['/usr/local/bin/nokia_ndk_techsupport'])


def set_temp_offset(offset):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
    if not channel or not stub:
        return

    stub.SetThermalOffset(platform_ndk_pb2.ReqTempParamsPb(temp_offset=offset))
    nokia_common.channel_shutdown(channel)

def set_fan_algo_disable(disable):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FAN_SERVICE)
    if not channel or not stub:
        return

    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=1)
    req_fan_algo = platform_ndk_pb2.SetFanTrayAlgorithmPb(fantray_algo_disable=disable)
    stub.DisableFanAlgorithm(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx, fan_algo=req_fan_algo))
    nokia_common.channel_shutdown(channel)

def set_fan_speed(speed):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FAN_SERVICE)
    if not channel or not stub:
        return

    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=1)
    stub.SetFanTargetSpeed(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx,
                                                            fantray_speed=speed))
    nokia_common.channel_shutdown(channel)

def set_fantray_led(index, color):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_FAN_SERVICE)
    if not channel or not stub:
        return

    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=index)
    _led_info = nokia_common.led_color_to_info(color)
    stub.SetFanLedStatus(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx, led_info=_led_info))
    nokia_common.channel_shutdown(channel)

def set_led(type_str, color):
    if type_str == 'port':
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_PORT
    elif type_str == 'fantray':
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_FANTRAY
    elif type_str == 'sfm':
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_SFM
    elif type_str == 'board':
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_BOARD_STATUS
    elif type_str == 'master-fan':
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_MASTER_FAN_STATUS
    elif type_str == 'master-psu':
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_MASTER_PSU_STATUS
    elif type_str == 'master-sfm':
        led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_MASTER_SFM_STATUS
    else:
        print('Unsupported led-type')
        return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_LED_SERVICE)
    if not channel or not stub:
        return

    _led_info = nokia_common.led_color_to_info(color)
    stub.SetLed(platform_ndk_pb2.ReqLedInfoPb(led_type=led_type, led_info=_led_info))
    nokia_common.channel_shutdown(channel)

def set_log_level_module(level, module):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    if (module == 'all'):
        _log_type = platform_ndk_pb2.ReqLogType.REQ_LOG_SET_ALL
        _log_info = platform_ndk_pb2.ReqLogInfoPb(level_current=level, module=module)
        stub.ReqLogSetAll(platform_ndk_pb2.ReqLogSettingPb(log_type=_log_type, log_info=_log_info))
    else:
        _log_type = platform_ndk_pb2.ReqLogType.REQ_LOG_SET_MODULE
        _log_info = platform_ndk_pb2.ReqLogInfoPb(level_current=level, module=module)
        stub.ReqLogSetModule(platform_ndk_pb2.ReqLogSettingPb(log_type=_log_type, log_info=_log_info))
    nokia_common.channel_shutdown(channel)

def set_reboot_linecard(slot):
    logger.log_warning("nokia_cmd reboots linecard slot {}".format(slot))
    nokia_common._reboot_imm(slot)

def set_restart_lc_system_service(slot, service_name):
    print("nokia_cmd restart system service '{}' on linecard slot {}".format(service_name, slot))
    logger.log_warning("nokia_cmd restart system service '{}' on linecard slot {}".format(service_name, slot))
    nokia_common._restart_lc_system_service(slot, service_name)

def set_log_restore_default():
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_UTIL_SERVICE)
    if not channel or not stub:
        return

    _log_type = platform_ndk_pb2.ReqLogType.REQ_LOG_RESET_ALL
    stub.ReqLogResetAll(platform_ndk_pb2.ReqLogSettingPb(log_type=_log_type))
    nokia_common.channel_shutdown(channel)

def set_asic_temp(name, temp, threshold):
    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
    if not channel or not stub:
        return

    asic_temp_entry = platform_ndk_pb2.AsicTempPb.AsicTempDevicePb(name=name, current_temp=temp,
                                                                   threshold=threshold)
    asic_devices = []
    asic_devices.append(asic_temp_entry)
    asic_temp_all = platform_ndk_pb2.AsicTempPb(temp_device=asic_devices)
    stub.SetThermalAsicInfo(platform_ndk_pb2.ReqTempParamsPb(asic_temp=asic_temp_all))
    nokia_common.channel_shutdown(channel)

def modify_startup_debug(key_str, new_stringval):
    startup_debug="/etc/opt/srlinux/startup_debug.json"
    tmp_startup_debug="/tmp/startup_debug.json"
    data = None

    file=os.readlink(startup_debug)
    with open(startup_debug, "r") as stream:
        data = json.load(stream)
    if data:
        for _, item in data.items():
            for info in item:
                if info['key'] == key_str:
                    info['stringval'] = new_stringval

        with open(tmp_startup_debug,"w+") as f:
            f.write(json.dumps(data, sort_keys=True, indent=4))
            f.write('\n')

        process = subprocess.Popen(["sudo", "mv", tmp_startup_debug, file])
        process.wait()
        return True

    return False


def set_ndk_monitor_action(action):
    startup_debug="/etc/opt/srlinux/startup_debug.json"
    tmp_startup_debug="/tmp/startup_debug.json"
    action_str = action
    if action == 'default':
        if nokia_common.is_cpm() == 1:
            action_str = 'warn'
        else:
            action_str = 'reboot'

    if modify_startup_debug("monitor_action", action_str):
        # display to and allow users to view it
        with open(startup_debug, "r") as stream:
            new_data = json.load(stream)
            print(json.dumps(new_data, indent=4))
        print("====== Modification is done. Reboot to take effect ======")

def set_ndk_log_level(level):
    startup_debug="/etc/opt/srlinux/startup_debug.json"
    tmp_startup_debug="/tmp/startup_debug.json"
    level_str = level
    if level == 'default':
        level_str = "debug"
    if modify_startup_debug("sonic_log_level", level_str):
        # display to allow users to view it
        with open(startup_debug, "r") as stream:
            new_data = json.load(stream)
            print(json.dumps(new_data, indent=4))
        print("====== Modification is done. Reboot to take effect ======")

def show_asic_temperature():
    if nokia_common.is_cpm() == 1:
      print('Command is not supported in Supervisor card')
      return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
    if not channel or not stub:
       return

    response = stub.GetThermalAsicInfo(platform_ndk_pb2.ReqTempParamsPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response.response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response.response_status.error_msg)
       return

    field = []
    field.append('Asic   ')
    field.append('Name    ')
    field.append('Current Temperature')
    field.append('Threshold')
    item_list = []
    dram_list = []
    i = 0
    while i < len(response.asic_temp_devices.temp_device):
       asic_temp = response.asic_temp_devices.temp_device[i]
       ns,name = asic_temp.name.split('_')
       item = []
       item.append(ns)
       item.append(name)
       item.append(str(asic_temp.current_temp))
       item.append(str(asic_temp.threshold))
       if 'dram' in name:
          dram_list.append(item)
       else:
          item_list.append(item)
       i += 1
    item_list.sort()
    print('  ASIC TEMPERATURE')
    print_table(field, item_list)
    print('  ASIC DRAM TEMPERATURE')
    print_table(field, dram_list)
    return

def show_midplane_port_counters(port):
    global format_type
    if nokia_common.is_cpm() == 0:
       print('Command is supported only in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_MIDPLANE_SERVICE)
    if not channel or not stub:
        return

    req_type = platform_ndk_pb2.ethMgrPortCounterRequest.ETH_MGR_GET_COUNTERS
    response = stub.ReqMidplanePortCounters(platform_ndk_pb2.ReqMidplanePortCountersInfoPb(_req_type=req_type, _port_name=port))

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    print('  PORT STATISTICS  ')
    i = 0
    while i < len(response._port_counters._stats_entry):
        counters = response._port_counters._stats_entry[i]
        print('================================================')
        print("Statistics for {}:".format(counters._port))
        print('================================================')
        stats_len = 0
        while stats_len < len(counters._stats):
          print(counters._stats[stats_len])
          stats_len += 1
        i += 1
    print('================================================')
    return

def show_midplane_port_status(port):
    global format_type
    if nokia_common.is_cpm() == 0:
       print('Command is supported only in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_MIDPLANE_SERVICE)
    if not channel or not stub:
        return

    response = stub.ReqMidplanePortStatus(platform_ndk_pb2.ReqMidplanePortStatusInfoPb(_port_name=port))

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    field = []
    field.append('Slot/Port   ')
    field.append('Admin Status')
    field.append('Oper Status')
    field.append('Speed ')
    field.append('Duplex     ')
    field.append('Linkscan  ')
    field.append('AutoNeg  ')
    field.append('STP state')
    field.append('Intf mode')
    field.append('MTU ')
    item_list = []
    i = 0
    while i < len(response._port_status._port_status):
        port_status = response._port_status._port_status[i]
        item = []
        item.append(port_status._port_name)
        item.append(port_status._admin_status)
        item.append(port_status._oper_status)
        item.append(str(port_status._speed))
        item.append(port_status._duplex)
        item.append(port_status._linkscan)
        item.append(port_status._autoneg)
        item.append(port_status._stp_state)
        item.append(port_status._intf_mode)
        item.append(str(port_status._mtu))
        item_list.append(item)
        i += 1

    print('   PORT STATUS')
    print_table(field, item_list)
    return

def show_midplane_vlan_table(vlan):
    global format_type
    if nokia_common.is_cpm() == 0:
       print('Command is supported only in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_MIDPLANE_SERVICE)
    if not channel or not stub:
        return

    elif vlan == 'ALL':
        vlan_id = 0
    else:
       vlan_id = int(vlan)

    response = stub.ReqMidplaneVlanTable(platform_ndk_pb2.ReqMidplaneVlanTablePb(_vlan_id=vlan_id))
    
    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    field = []
    field.append('VLAN   ')
    field.append('     Tagged Ports                       ')
    field.append('Untagged Ports')
    item_list = []
    i = 0
    while i < len(response._vlan_table._vlan_entry):
        vlan_entry = response._vlan_table._vlan_entry[i]
        item = []
        item.append(str(vlan_entry._vlan_id))
        mem_len = 0
        mem_str = ""
        while mem_len < len(vlan_entry._members):
            mem_str += vlan_entry._members[mem_len]
            if mem_len < (len(vlan_entry._members)-1):
                mem_str += ","
            mem_len += 1
        item.append(mem_str)
        untag_len = 0
        untag_str = ""
        while untag_len < len(vlan_entry._untagged):
            untag_str += vlan_entry._untagged[untag_len]
            if untag_len < (len(vlan_entry._untagged)-1):
                untag_str += ","
            untag_len += 1
        item.append(untag_str)
        item_list.append(item)
        i += 1

    print('  VLAN TABLE')
    print_table(field, item_list)
    return

def show_midplane_mac_table(port):
    global format_type
    if nokia_common.is_cpm() == 0:
       print('Command is supported only in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_MIDPLANE_SERVICE)
    if not channel or not stub:
        return

    response = stub.ReqMidplaneMacTable(platform_ndk_pb2.ReqMidplaneMacTablePb(_port_name=port))

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    field = []
    field.append('   Mac Address   ')
    field.append('Vlan ')
    field.append(' Slot  ')
    field.append(' Type  ')
    item_list = []
    i = 0
    while i < len(response._mac_table._mac_entry):
        mac_entry = response._mac_table._mac_entry[i]
        item = []
        item.append(mac_entry._mac_address)
        item.append(str(mac_entry._vlan))
        item.append(mac_entry._port)
        item.append(mac_entry._type)
        item_list.append(item)
        i += 1

    print('   MAC TABLE')
    print_table(field, item_list)
    return

def show_midplane_link_status_table():
    global format_type
    if nokia_common.is_cpm() == 0:
       print('Command is supported only in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_MIDPLANE_SERVICE)
    if not channel or not stub:
        return

    response = stub.ReqMidplaneLinkStatusTracker(platform_ndk_pb2.ReqMidplanePortStatusInfoPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    field = []
    field.append('  PORT   ')
    field.append(' UP ')
    field.append(' DOWN')
    item_list = []
    i = 0
    while i < len(response._link_status._link_status_entry):
        link_status = response._link_status._link_status_entry[i]
        item = []
        item.append(link_status._port)
        item.append(str(link_status._up))
        item.append(str(link_status._down))
        item_list.append(item)
        i += 1

    print('   LINK STATUS FLAP')
    print_table(field, item_list)
    return

def clear_midplane_port_counters():
    if nokia_common.is_cpm() == 0:
       print('Command is supported only in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_MIDPLANE_SERVICE)
    if not channel or not stub:
        return

    req_type = platform_ndk_pb2.ethMgrPortCounterRequest.ETH_MGR_CLEAR_COUNTERS
    response = stub.ReqMidplanePortCounters(platform_ndk_pb2.ReqMidplanePortCountersInfoPb(_req_type=req_type))

    nokia_common.channel_shutdown(channel)

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print('Clearing Port counters failed')

def show_qfpga_port_status():
    global format_type
    if nokia_common.is_cpm() == 1:
       print('Command is not supported in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_QFPGA_SERVICE)
    if not channel or not stub:
        return

    response = stub.ReqQfpgaPortSummary(platform_ndk_pb2.ReqQfpgaInfoPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    field = []
    field.append('  Port  ')
    field.append(' Description          ')
    field.append('OperStatus ')
    field.append(' MTU  ')
    field.append(' Speed  ')
    field.append(' Vlan ')
    item_list = []
    i = 0

    while i < len(response._port_summary._port):
       port = "Port"+str(i)
       port_sum = response._port_summary._port[port]
       item = []
       item.append(port)
       item.append(qfpga_cli_port_name_dict[port])
       item.append(port_sum._oper_status)
       item.append(str(port_sum._mtu))
       item.append(str(port_sum._speed))
       j = 0
       vlan = ""
       vlan_len = len(port_sum._vlan)
       while j < vlan_len:
         if vlan_len != 1 and j != (vlan_len-1):
           vlan = vlan + str(port_sum._vlan[j]) + ","
         else:
           vlan = vlan + str(port_sum._vlan[j])
         j += 1
       item.append(str(vlan))
       item_list.append(item)
       i += 1
    print('   PORT SUMMARY')
    print_table(field, item_list)
    return

def show_qfpga_port_statistics(port):
    global format_type
    if nokia_common.is_cpm() == 1:
       print('Command is not supported in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_QFPGA_SERVICE)
    if not channel or not stub:
        return

    port_name = qfpga_ndk_port_name_dict.get(port, "Undefined")
    if port_name == 'Undefined':
       print('port-desc must be one of: cpm-A, cpm-B, qfpgap0/eth1-midplane, qfpgap1/eth0')
       return

    response = stub.ReqQfpgaPortStatsSummary(platform_ndk_pb2.ReqQfpgaInfoPb(_port = port_name))

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    print('       PORT STATISTICS  ')
    i = 0
    print('==============================================')
    while i < len(response._port_stats._port):
       port = "Port"+str(i)
       counters = response._port_stats._port[port]
       if counters._port != port:
           i += 1
           continue
       port_name = port+'/'+qfpga_cli_port_name_dict[port]
       print('Port : ', port_name)
       print('==============================================')
       print('RxOctetsGood                 : ', str(counters._rx_octets_good))
       print('TxOctetsGood                 : ', str(counters._tx_octets_good))
       print('RxPacketsGoodPause           : ', str(counters._rx_pkts_good_pause))
       print('RxPacketsGoodUnicast         : ', str(counters._rx_pkts_good_unicast))
       print('RxPacketsGoodMulticast       : ', str(counters._rx_pkts_good_multicast))
       print('RxPacketsGoodBroadcast       : ', str(counters._rx_pkts_good_broadcast))
       print('RxPacketsGoodOverlong        : ', str(counters._rx_pkts_good_overlong))
       print('RxPacketsFailOverlong        : ', str(counters._rx_pkts_fail_overlong))
       print('TxPacketsGoodPause           : ', str(counters._tx_pkts_good_pause))
       print('TxPacketsGoodUnicast         : ', str(counters._tx_pkts_good_unicast))
       print('TxPacketsGoodMulticast       : ', str(counters._tx_pkts_good_multicast))
       print('TxPacketsGoodBroadcast       : ', str(counters._tx_pkts_good_broadcast))
       print('TxPacketsGoodInitDefer       : ', str(counters._tx_pkts_good_init_defer))
       print('TxPacketsGoodInitExcessDefer : ', str(counters._tx_pkts_good_init_excess_defer))
       print('TxPacketsGood0Collisions     : ', str(counters._tx_pkts_good_0_collisions))
       print('TxPacketsGood1Collisions     : ', str(counters._tx_pkts_good_1_collision))
       print('TxPacketsGood2To7Collisions  : ', str(counters._tx_pkts_good_2_to_7_collisions))
       print('TxPacketsGood8To15Collisions : ', str(counters._tx_pkts_good_8_to_15_collisions))
       print('TxPacketsFailExcessCollisions: ', str(counters._tx_pkts_fail_excess_collisions))
       print('TxPacketsFailLateCollisions  : ', str(counters._tx_pkts_fail_late_collisions))
       print('TxPacketsFailFcsError        : ', str(counters._tx_pkts_fail_fcs_error))
       print('PacketsGood64Octets          : ', str(counters._pkts_good_64_octets))
       print('PacketsGood65To127Octets     : ', str(counters._pkts_good_65to127_octets))
       print('PacketsGood128To255Octets    : ', str(counters._pkts_good_128to255_octets))
       print('PacketsGood256To511Octets    : ', str(counters._pkts_good_256to511_octets))
       print('PacketsGood512To1023Octets   : ', str(counters._pkts_good_512to1023_octets))
       print('PacketsGood1024To1518Octets  : ', str(counters._pkts_good_1024to1518_octets))
       print('PacketsGood1519PlusOctets    : ', str(counters._pkts_good_1519plus_octets))
       print('RxFragments                  : ', str(counters._rx_fragments))
       print('SymbolOrDribbleError         : ', str(counters._symbol_or_dribble_error))
       print('FcsError                     : ', str(counters._fcs_error))
       print('RunTimeError                 : ', str(counters._runt_error))
       print('==============================================')
       i += 1
    print('==============================================')
    return

def show_qfpga_error_counters():
    global format_type
    if nokia_common.is_cpm() == 1:
       print('Command is not supported in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_QFPGA_SERVICE)
    if not channel or not stub:
        return

    response = stub.ReqQfpgaErrorCounters(platform_ndk_pb2.ReqQfpgaInfoPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    counters = response._error_counters
    print('==============================================')
    print('      Qfpga  Error Counters')
    print('==============================================')
    print('FCS Error                 : ', str(counters._fcs_error))
    print('TTL pkt Error             : ', str(counters._ttl_pkts_error))
    print('Network LIF Error         : ', str(counters._network_lif_error))
    print('Runt Pkt Error            : ', str(counters._runt_pkts_error))
    print('Unknown IP version Error  : ', str(counters._unknown_ip_version_error))
    print('IP HdrLen Error           : ', str(counters._ip_hdr_len_error))
    print('IP Checksum Error         : ', str(counters._ip_checksum_error))
    print('LACP Error                : ', str(counters._lacp_disable_error))
    print('LLDP Error                : ', str(counters._lldp_disable_error))
    print('Dot1x Disable Error       : ', str(counters._dot1x_disable_error))
    print('EFM Disable Error         : ', str(counters._efm_disable_error))
    print('Unknown UC Drop           : ', str(counters._unknown_unicast_error))
    print('Unknown MC Drop           : ', str(counters._unknown_multicast_error))
    print('Broadcast Drop            : ', str(counters._broadcast_drop))
    print('MAC Move Disable Drop     : ', str(counters._mac_move_disable_error))
    print('Extra Tag Drop            : ', str(counters._extra_tag_drop))
    print('Invalid CFM Level Drop    : ', str(counters._invalid_cfm_level_drop))
    print('Src Knockout Drop         : ', str(counters._src_knockout_drop))
    print('MTU Exceeded Drop         : ', str(counters._mtu_exceeded_drop))
    print('AIS Disable Drop          : ', str(counters._cfm_ais_disable_drop))
    print('APS Disable Drop          : ', str(counters._cfm_aps_disable_drop))
    print('RAPS Disable Drop         : ', str(counters._cfm_raps_disable_drop))
    print('ETH-LM Disable Drop       : ', str(counters._cfm_lm_disable_drop))
    print('ETH-DM Disable Drop       : ', str(counters._cfm_dm_disable_drop))
    print('Oper Shutdown Drop        : ', str(counters._oper_shutdown_drop))
    print('Ingress Protection Drop   : ', str(counters._ingress_protection_drop))
    print('Parser Drop               : ', str(counters._parser_drop))
    print('STP Block                 : ', str(counters._stp_block))
    print('Unknown SAP Drop          : ', str(counters._unknown_sap_drop))
    print('EFM Loopback Drop         : ', str(counters._efm_loopback_drop))
    print('Src Miss Drop             : ', str(counters._src_miss_drop))
    print('Egress STP Block          : ', str(counters._egress_stp_block))
    print('Egress Oper Shut Drop     : ', str(counters._egress_oper_shut_drop))
    print('Spork Send Error          : ', str(counters._spork_send_error))
    print('Policer RED Drop          : ', str(counters._policer_red_drop))
    print('Ingress TM Drop           : ', str(counters._ingress_tm_drop))
    print('Multicast FIFO Drop FC BE : ', str(counters._mcast_fifo_drop_fc_be))
    print('Multicast FIFO Drop FC L2 : ', str(counters._mcast_fifo_drop_fc_l2))
    print('Multicast FIFO Drop FC AF : ', str(counters._mcast_fifo_drop_fc_af))
    print('Multicast FIFO Drop FC L1 : ', str(counters._mcast_fifo_drop_fc_l1))
    print('Multicast FIFO Drop FC H2 : ', str(counters._mcast_fifo_drop_fc_h2))
    print('Multicast FIFO Drop FC EF : ', str(counters._mcast_fifo_drop_fc_ef))
    print('Multicast FIFO Drop FC H1 : ', str(counters._mcast_fifo_drop_fc_h1))
    print('Multicast FIFO Drop FC NC : ', str(counters._mcast_fifo_drop_fc_nc))
    print('Unknown Label Drop        : ', str(counters._unknown_label_drop))
    print('MPLS Drop                 : ', str(counters._mpls_drop))
    print('Egress port oper shut     : ', str(counters._egress_port_oper_shut))
    print('Egress tunnel oper shut   : ', str(counters._egress_tunnel_oper_shut))
    print('Egress lif oper shut      : ', str(counters._egress_lif_oper_shut))
    print('ISIS ERROR                : ', str(counters._isis_errors))
    print('synce Disable             : ', str(counters._synce_disable))
    print('RPF Check                 : ', str(counters._rpf_check_drops))
    print('Rx Mtu Exceeded           : ', str(counters._rx_mtu_exceeded))
    print('SMAC Error                : ', str(counters._smac_errors))
    print('==============================================')
    return


def show_qfpga_vlan_counters():
    global format_type
    if nokia_common.is_cpm() == 1:
       print('Command is not supported in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_QFPGA_SERVICE)
    if not channel or not stub:
        return

    response = stub.ReqQfpgaVlanCounters(platform_ndk_pb2.ReqQfpgaInfoPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    print('==============================================')
    print('       IMM TO CPM STATISTICS')
    print('==============================================')
    field = []
    field.append('  EPIPE ')
    field.append(' VLAN ')
    field.append(' SAP INGRESS')
    field.append(' SAP EGRESS')
    field.append(' POLICER DROP')
    field.append(' QUEUE DROP')
    item_list = []
    i = 0
    while i < len(response._vlan_counters._imm_to_cpm_stats):
       epipe = response._vlan_counters._imm_to_cpm_stats[i]
       item = []
       item.append(str(epipe._epipe))
       item.append(str(epipe._vlan))
       item.append(str(epipe._sap_ingress))
       item.append(str(epipe._sap_egress))
       item.append(str(epipe._policer_drop))
       item.append(str(epipe._queue_drop))
       item_list.append(item)
       i += 1
    print_table(field, item_list)
    print('==============================================')
    print('       CPM TO IMM STATISTICS')
    print('==============================================')
    field = []
    field.append('  EPIPE ')
    field.append(' VLAN ')
    field.append(' SAP INGRESS')
    field.append(' SAP EGRESS')
    field.append(' POLICER DROP')
    field.append(' QUEUE DROP')
    item_list1 = []
    i = 0
    while i < len(response._vlan_counters._cpm_to_imm_stats):
       epipe = response._vlan_counters._cpm_to_imm_stats[i]
       item = []
       item.append(str(epipe._epipe))
       item.append(str(epipe._vlan))
       item.append(str(epipe._sap_ingress))
       item.append(str(epipe._sap_egress))
       item.append(str(epipe._policer_drop))
       item.append(str(epipe._queue_drop))
       item_list1.append(item)
       i += 1
    print_table(field, item_list1)
    return

def show_qfpga_epipe_config():
    global format_type
    if nokia_common.is_cpm() == 1:
       print('Command is not supported in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_QFPGA_SERVICE)
    if not channel or not stub:
        return

    response = stub.ReqQfpgaEpipeConfig(platform_ndk_pb2.ReqQfpgaInfoPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    print('==============================================')
    print('       EPIPE CONFIGURATION')
    print('==============================================')
    field = []
    field.append('  EPIPE ')
    field.append(' VLAN ')
    field.append(' PRIORITY')
    field.append(' WEIGHT')
    field.append(' SAP1_ID')
    field.append(' SAP1_PORT_NUM')
    field.append(' SAP1_QUEUE_ID')
    field.append(' SAP1_POLICER_ID')
    field.append(' SAP2_ID')
    field.append(' SAP2_PORT_NUM')
    field.append(' SAP2_QUEUE_ID')
    field.append(' SAP2_POLICER_ID')
    item_list = []
    i = 0
    while i < len(response._epipe_config._epipe_info):
       epipe = response._epipe_config._epipe_info[i]
       item = []
       item.append(str(epipe._epipe_id))
       item.append(str(epipe._vlan_id))
       item.append(str(epipe._priority))
       item.append(str(epipe._weight))
       item.append(str(epipe._sap_1._sap_id))
       item.append(str(epipe._sap_1._port_num))
       item.append(str(epipe._sap_1._queue_id))
       item.append(str(epipe._sap_1._policer_id))
       item.append(str(epipe._sap_2._sap_id))
       item.append(str(epipe._sap_2._port_num))
       item.append(str(epipe._sap_2._queue_id))
       item.append(str(epipe._sap_2._policer_id))
       item_list.append(item)
       i += 1
    print_table(field, item_list)
    return

def show_qfpga_version():
    global format_type
    if nokia_common.is_cpm() == 1:
       print('Command is not supported in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_QFPGA_SERVICE)
    if not channel or not stub:
       return

    response = stub.ReqQfpgaVersion(platform_ndk_pb2.ReqQfpgaInfoPb())

    nokia_common.channel_shutdown(channel)

    if format_type == 'json-format':
      json_response = MessageToJson(response)
      print(json_response)
      return

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print(response._response_status.error_msg)
       return

    version = response._version_info._version
    revision = response._version_info._revision
    print('The E2E_4X1G FPGA (qfpga) version is {}, revision is {}'.format(str(version),str( revision)))
    return

def clear_qfpga_stats():
    global format_type
    if nokia_common.is_cpm() == 1:
       print('Command is not supported in Supervisor card')
       return

    channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_QFPGA_SERVICE)
    if not channel or not stub:
       return

    response = stub.ReqQfpgaClearStats(platform_ndk_pb2.ReqQfpgaInfoPb())

    nokia_common.channel_shutdown(channel)

    if response._response_status.status_code != platform_ndk_pb2.ResponseCode.NDK_SUCCESS:
       print('Clearing Qfpga Stats failed')
    return

def set_shutdown_sfm(num):
    if num not in sfm_hw_slot_mapping:
        print("Invalid SFM number {}. Valid range is (1..8)".format(num))
        return
    logger.log_warning("nokia_cmd shutdown SFM-{}".format(num))    
    asic_list = sfm_asic_dict[num]
    for asic in asic_list:
        print("Shutdown swss@{} and syncd@{} services".format(asic, asic))
        # Process state
        process = subprocess.Popen(['sudo', 'systemctl', 'stop', 'swss@{}.service'.format(asic)],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        outstr = stdout.decode('ascii')
    # wait for service is down
    time.sleep(2)

    print("Power off SFM-{} module ...".format(num))
    hw_slot = sfm_hw_slot_mapping[num]
    nokia_common._power_onoff_SFM(hw_slot,False)

def set_startup_sfm(num):
    if num not in sfm_hw_slot_mapping:
        print("Invalid SFM number {}. Valid range is (1..8)".format(num))
        return
    logger.log_warning("nokia_cmd startup SFM-{}".format(num))     
    print("Power up SFM-{} module ...".format(num))
    hw_slot = sfm_hw_slot_mapping[num]
    nokia_common._power_onoff_SFM(hw_slot,True)
    # wait SFM HW init done.
    time.sleep(15)

    asic_list = sfm_asic_dict[num]
    for asic in asic_list:
        print("Start the swss@{} and syncd@{} ...".format(asic, asic))
        # Process state
        process = subprocess.Popen(['sudo', 'systemctl', 'start', 'swss@{}.service'.format(asic)],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        outstr = stdout.decode('ascii')
    return

def main():
    global format_type

    base_parser = argparse.ArgumentParser()
    subparsers = base_parser.add_subparsers(help='sub-commands', dest="cmd")

    # Show Commands
    show_parser = subparsers.add_parser('show', help='show help')
    showsubparsers = show_parser.add_subparsers(help='show cmd options', dest="showcmd")

    # show platform
    show_platform_parser = showsubparsers.add_parser('platform', help='show platform')
    show_platform_parser.add_argument('json-format', nargs='?', help='show platform <json-format>')

    # show psu
    show_psu_parser = showsubparsers.add_parser('psus', help='show psus info')
    show_psu_parser.add_argument('json-format', nargs='?', help='show psus <json-format>')

    # show led
    show_led_parser = showsubparsers.add_parser('fp-status', help='show front-panel-leds')
    show_led_parser.add_argument('json-format', nargs='?', help='show front-panel-leds <json-format>')

    # show sensors
    show_sensor_parser = showsubparsers.add_parser('sensors', help='show sensors info')
    show_sensor_parser.add_argument('json-format', nargs='?', help='show sensors <json-format>')

    # show firmware
    show_firmware_parser = showsubparsers.add_parser('firmware', help='show firmware info')
    show_firmware_parser.add_argument('json-format', nargs='?', help='show firmware <json-format>')

    # show firmware
    show_sysleds_parser = showsubparsers.add_parser('system-leds', help='show system-leds info')
    show_sysleds_parser.add_argument('json-format', nargs='?', help='show system-leds <json-format>')

    # show syseeprom
    show_syseeprom_parser = showsubparsers.add_parser('syseeprom', help='show syseeprom info')
    show_syseeprom_parser.add_argument('json-format', nargs='?', help='show syseeprom <json-format>')

    # show chassis
    show_chassis_parser = showsubparsers.add_parser('chassis', help='show chassis info')
    show_chassis_parser.add_argument('json-format', nargs='?', help='show chassis <json-format>')

    # show power
    show_power_parser = showsubparsers.add_parser('power', help='show power info')
    show_power_parser.add_argument('json-format', nargs='?', help='show power <json-format>')

    # show midplane
    show_midplane_parser = showsubparsers.add_parser('midplane', help='show midplane')
    show_midplane_sub_parser = show_midplane_parser.add_subparsers(help='show midplane options', dest="midplanecmd")
    # show midplane status
    show_midplane_status_parser = show_midplane_sub_parser.add_parser('status', help='show midplane status')
    show_midplane_status_parser.add_argument('hw-slot', nargs='?', help='slot number integer')
    show_midplane_status_parser.add_argument('json-format', nargs='?', help='json-format')
    # show midplane port-counters
    show_midplane_port_ctr_parser = show_midplane_sub_parser.add_parser('port-counters', help='show midplane port counters')
    show_midplane_port_ctr_parser.add_argument('--hw-slot', nargs='?', default='ALL_PORTS', help='slot name lc1,lc2,lc3,lc4,lc5,lc6,lc7,lc8,cpm-xe0,cpm-xe1')
    show_midplane_port_ctr_parser.add_argument('json-format', nargs='?', help='json-format')
    # show midplane port-status
    show_midplane_port_status_parser = show_midplane_sub_parser.add_parser('port-status', help='show midplane port status')
    show_midplane_port_status_parser.add_argument('--hw-slot', nargs='?', default='ALL_PORTS', help='slot name lc1,lc2,lc3,lc4,lc5,lc6,lc7,lc8,cpm-xe0,cpm-xe1')
    show_midplane_port_status_parser.add_argument('json-format', nargs='?', help='json-format')
    # show midplane vlan-table
    show_midplane_vlan_table_parser = show_midplane_sub_parser.add_parser('vlan-table', help='show midplane vlan table')
    show_midplane_vlan_table_parser.add_argument('--vlan-id', nargs='?', default='ALL', help='vlan id integer')
    show_midplane_vlan_table_parser.add_argument('json-format', nargs='?', help='json-format')
    # show midplane mac-table
    show_midplane_mac_table_parser = show_midplane_sub_parser.add_parser('mac-table', help='show midplane mac table')
    show_midplane_mac_table_parser.add_argument('--hw-slot', nargs='?', default='ALL_PORTS', help='slot name lc1,lc2,lc3,lc4,lc5,lc6,lc7,lc8,cpm-xe0,cpm-xe1')
    show_midplane_mac_table_parser.add_argument('json-format', nargs='?', help='json-format')
    # show midplane link-status-flap
    show_midplane_link_status_parser = show_midplane_sub_parser.add_parser('link-status-flap', help='show link status flapping')
    show_midplane_link_status_parser.add_argument('json-format', nargs='?', help='json-format')

    # show logging
    show_logging_parser = showsubparsers.add_parser('logging', help='show logging info')
    show_logging_parser.add_argument('json-format', nargs='?', help='show logging <json-format>')

    # show ndk-eeprom
    show_ndkeeprom_parser = showsubparsers.add_parser('ndk-eeprom', help='show ndk-eeprom info')
    show_ndkeeprom_parser.add_argument('json-format', nargs='?', help='show ndk-eeprom <json-format>')

    # show ndk-status
    show_ndkstatus_parser = showsubparsers.add_parser('ndk-status', help='show ndk-status info')
    show_ndkstatus_parser.add_argument('json-format', nargs='?', help='show ndk-status <json-format>')

    # show ndk-version
    show_ndkversion_parser = showsubparsers.add_parser('ndk-version', help='show ndk-version info')

    # show sfm-summary
    show_sfmsum_parser = showsubparsers.add_parser('sfm-summary', help='show sfm-summary info')
    show_sfmsum_parser.add_argument('json-format', nargs='?', help='show sfm-summary <json-format>')

    # show fabric-pcieinfo
    show_fpcie_parser = showsubparsers.add_parser('fabric-pcie', help='show fabric-pcie')
    show_fpcie_parser.add_argument('hw-slot', metavar='hw-slot', type=int, help='slot number integer. "nokia_cmd show sfm-summary" shows valid slot numbers')
    show_fpcie_parser.add_argument('json-format', nargs='?', help='show fabric-pcie <json-format>')
    
    # show sfm-eeprom
    show_sfmeeprom_parser = showsubparsers.add_parser('sfm-eeprom', help='show sfm-eeprom info')
    show_sfmeeprom_parser.add_argument('json-format', nargs='?', help='show sfm-eeprom <json-format>')

    # show asic-temperature
    show_asictemp_parser = showsubparsers.add_parser('asic-temperature', help='show asic-temperature info')
    show_asictemp_parser.add_argument('json-format', nargs='?', help='show asic-temperature <json-format>')

    # show qfpga
    show_qfpga_parser = showsubparsers.add_parser('qfpga', help='show qfpga')
    show_qfpga_sub_parser = show_qfpga_parser.add_subparsers(help='show qfpga options', dest="qfpgacmd")
    # show qfpga port-status
    show_qfpga_port_status_parser = show_qfpga_sub_parser.add_parser('port-status', help='show qfpga port status')
    show_qfpga_port_status_parser.add_argument('json-format', nargs='?', help='json-format')
    # show qfpga port-statistics
    show_qfpga_port_stats_parser = show_qfpga_sub_parser.add_parser('port-statistics', help='show qfpga port statistics')
    show_qfpga_port_stats_parser.add_argument('--port-desc', nargs='?', default='ALL_PORTS', help='port-name: cpm-A, cpm-B, qfpgap0/eth1-midplane, qfpgap1/eth0')
    show_qfpga_port_stats_parser.add_argument('json-format', nargs='?', help='json-format')
    # show qfpga error-counters
    show_qfpga_error_counter_parser = show_qfpga_sub_parser.add_parser('error-counters', help='show qfpga error counters')
    show_qfpga_error_counter_parser.add_argument('json-format', nargs='?', help='json-format')
    # show qfpga vlan-counters
    show_qfpga_vlan_counters_parser = show_qfpga_sub_parser.add_parser('vlan-counters', help='show qfpga vlan counters')
    show_qfpga_vlan_counters_parser.add_argument('json-format', nargs='?', help='json-format')
    # show qfpga epipe-config
    show_qfpga_epipe_config_parser = show_qfpga_sub_parser.add_parser('epipe-config', help='show qfpga epipe configuration')
    show_qfpga_epipe_config_parser.add_argument('json-format', nargs='?', help='json-format')
    # show qfpga version
    show_qfpga_version_parser = show_qfpga_sub_parser.add_parser('version', help='show qfpga version')
    show_qfpga_version_parser.add_argument('json-format', nargs='?', help='json-format')

    # Set Commands
    set_parser = subparsers.add_parser('set', help='set help')
    setsubparsers = set_parser.add_subparsers(help='set cmd options', dest="setcmd")

    # Set temp-offset
    set_tempoff_parser = setsubparsers.add_parser('temp-offset', help='set tempoffset value')
    set_tempoff_parser.add_argument('offset', nargs='?', help='int from 0-100')

    # Set fan-algorithm
    set_fanalgo_parser = setsubparsers.add_parser('fan-algorithm', help='set fan-algo value')
    set_fanalgo_parser.add_argument('disable', nargs='?', help='1(disable) or 0(enable)')

    # Set fan-speed
    set_fanspeed_parser = setsubparsers.add_parser('fan-speed', help='set fan-speed value')
    set_fanspeed_parser.add_argument('speed', nargs='?', help='int from 0-100')

    # Set fanled
    set_fanled_parser = setsubparsers.add_parser('fantray-led', help='set fan-led')
    set_fanled_parser.add_argument('index', nargs='?', help='0-MAX')
    set_fanled_parser.add_argument('color', nargs='?', help='off, red, amber, green')

    # Set led
    set_deviceled_parser = setsubparsers.add_parser('led', help='set device led')
    set_deviceled_parser.add_argument('device', nargs='?',
                                      help='port/fantray/sfm/board/master-psu/master-fan/master-sfm')
    set_deviceled_parser.add_argument('color', nargs='?', help='off, red, amber, green')

    # Set loglvl
    set_loglvl_parser = setsubparsers.add_parser('log-level', help='set logging level')
    set_loglvl_parser.add_argument('level', nargs='?', help='Levels - \n' +
                                   '\t0 or EMERG\n' +
                                   '\t1 or ALERT\n' +
                                   '\t2 or CRIT\n' +
                                   '\t3 or ERR\n' +
                                   '\t4 or WARNING\n' +
                                   '\t5 or NOTICE\n' +
                                   '\t6 or INFO\n' +
                                   '\t7 or DEBUG\n' +
                                   '\t8 or TRACE\n')
    set_loglvl_parser.add_argument('module', nargs='?', help='Modules - \n' + '\tall\n' + '\tcommon\n')

    # Set log-restore
    set_logreset_parser = setsubparsers.add_parser('log-level-restore', help='reset logging level')

    # Set asic temp
    set_asictemp_parser = setsubparsers.add_parser('asic-temp', help='set asic temp-device')
    set_asictemp_parser.add_argument('name', nargs='?', help='Sensor name. Format: <asic_name>_<sensor_name> from the output of \'nokia_cmd show asic-temperature\'')
    set_asictemp_parser.add_argument('temp', nargs='?', help='Temperature value')
    set_asictemp_parser.add_argument('threshold', nargs='?', help='Threshold')

    # set stop-sfm
    set_shutdownsfm_parser = setsubparsers.add_parser('shutdown-sfm', help='shutdown a sfm and related asic services (swss and syncd)')
    set_shutdownsfm_parser.add_argument('sfm-num', metavar='sfm-num', type=int, help='SFM slot number starts from 1')
    
    # set start-sfm
    set_startupsfm_parser = setsubparsers.add_parser('startup-sfm', help='startup a sfm and related asic services (swss and syncd)')
    set_startupsfm_parser.add_argument('sfm-num', metavar='sfm-num', type=int, help='SFM slot number starts from 1')

    # set reboot-linecard
    set_reboot_linecard_parser = setsubparsers.add_parser('reboot-linecard', help='Reboot linecard from Supervisor')
    set_reboot_linecard_parser.add_argument('slot', type=int, help='Linecard slot number starts from 1 to 8')
    set_reboot_linecard_parser.add_argument('force', nargs='?', type=str, help='Continue without prompt for confirmation')

    # set restart_lc_system_service
    set_restart_lc_system_service_parser = setsubparsers.add_parser('restart-lc-system-service', help='Restart linecard system service from Supervisor')
    set_restart_lc_system_service_parser.add_argument('slot', type=int, help='Linecard slot number starts from 1 to 8')
    set_restart_lc_system_service_parser.add_argument('service', type=str, help='Service filename. Current support: interfaces-config.service')
    
    set_ndk_monitor_action_parser = setsubparsers.add_parser('ndk-monitor-action', help='Change the NDK monitor_action value (warn or reboot) in starup_debug.json file')
    set_ndk_monitor_action_parser.add_argument('action', nargs='?', help='Choices: warn, reboot or default')

    set_ndk_log_level_parser = setsubparsers.add_parser('ndk-log-level', help='Change the NDK sonic_log_level value in starup_debug.json file')
    set_ndk_log_level_parser.add_argument('level', nargs='?', help='Choices: trace. debug, info, notice, warning, error, critical or default')

    # Request Commands
    req_parser = subparsers.add_parser('request', help='Req help')
    reqsubparsers = req_parser.add_subparsers(help='Request command options', dest="reqcmd")

    # Devmgr admintech
    req_devmgr_admintech_parser = reqsubparsers.add_parser('devmgr-admintech',
                                                           help='collect devmgr admintech')
    req_devmgr_admintech_parser.add_argument('filepath', nargs='?', help='<filepath>')

    # Clear Commands
    clear_parser = subparsers.add_parser('clear', help='Clear help')
    clearsubparsers = clear_parser.add_subparsers(help='clear cmd options', dest="clearcmd")

    #clear midplane port-counters
    clear_midplane_port_counter_parser = clearsubparsers.add_parser('midplane',
                                                           help='clear midplane counters')
    clear_midplane_port_counter_parser.add_argument('port-counters', nargs='?', help='clear port counters')
    #clear qfpga stats
    clear_qfpga_stats_parser = clearsubparsers.add_parser('qfpga',
                                                           help='clear qfpga stats')
    clear_qfpga_stats_parser.add_argument('stats', nargs='?', help='clear stats')

    # An illustration of how access the arguments.
    args = base_parser.parse_args()
    d = vars(args)
    if args.cmd == 'show':
        if args.showcmd == 'platform':
            format_type = d['json-format']
            show_platform()
        elif args.showcmd == 'psus':
            format_type = d['json-format']
            show_psu()
            show_psu_detail()
        elif args.showcmd == 'fp-status':
            format_type = d['json-format']
            show_led()
        elif args.showcmd == 'sensors':
            format_type = d['json-format']
            show_temp()
            show_fan()
            show_fan_detail()
        elif args.showcmd == 'firmware':
            format_type = d['json-format']
            show_firmware()
        elif args.showcmd == 'system-leds':
            format_type = d['json-format']
            show_system_led()
        elif args.showcmd == 'syseeprom':
            format_type = d['json-format']
            show_syseeprom()
        elif args.showcmd == 'chassis':
            format_type = d['json-format']
            show_chassis()
        elif args.showcmd == 'power':
            format_type = d['json-format']
            show_power()
        elif args.showcmd == 'logging':
            format_type = d['json-format']
            show_logging()
        elif args.showcmd == 'syseeprom':
            format_type = d['json-format']
            show_syseeprom()
        elif args.showcmd == 'midplane':
            if 'json-format' in d:
                format_type = d['json-format']
            if args.midplanecmd == 'status':
              show_midplane_status(int(d['hw-slot']))
            elif args.midplanecmd == 'port-counters':
              show_midplane_port_counters(d['hw_slot'])
            elif args.midplanecmd == 'port-status':
              show_midplane_port_status(d['hw_slot'])
            elif args.midplanecmd == 'vlan-table':
              show_midplane_vlan_table(d['vlan_id'])
            elif args.midplanecmd == 'mac-table':
              show_midplane_mac_table(d['hw_slot'])
            elif args.midplanecmd == 'link-status-flap':
              show_midplane_link_status_table()
            else:
                show_midplane_parser.print_help()
        elif args.showcmd == 'ndk-eeprom':
            format_type = d['json-format']
            show_ndk_eeprom()
        elif args.showcmd == 'ndk-status':
            format_type = d['json-format']
            show_ndk_status()
        elif args.showcmd == 'ndk-version':
            show_ndk_version()
        elif args.showcmd == 'sfm-summary':
            format_type = d['json-format']
            show_sfm_summary()
        elif args.showcmd == 'fabric-pcie':
            format_type = d['json-format']
            show_fabric_pcieinfo(int(d['hw-slot']))
        elif args.showcmd == 'sfm-eeprom':
            format_type = d['json-format']
            show_sfm_eeprom()
        elif args.showcmd == 'asic-temperature':
            format_type = d['json-format']
            show_asic_temperature()
        elif args.showcmd == 'qfpga':
            if 'json-format' in d:
                format_type = d['json-format']
            if args.qfpgacmd == 'port-status':
                show_qfpga_port_status()
            elif args.qfpgacmd == 'port-statistics':
                show_qfpga_port_statistics(d['port_desc'])
            elif args.qfpgacmd == 'error-counters':
                show_qfpga_error_counters()
            elif args.qfpgacmd == 'vlan-counters':
                show_qfpga_vlan_counters()
            elif args.qfpgacmd == 'epipe-config':
                show_qfpga_epipe_config()
            elif args.qfpgacmd == 'version':
                show_qfpga_version()
            else:
                show_qfpga_parser.print_help()
        else:
            show_parser.print_help()
    elif args.cmd == 'set':
        if args.setcmd == 'temp-offset':
            set_temp_offset(int(d['offset']))
        elif args.setcmd == 'fan-algorithm':
            set_fan_algo_disable(int(d['disable']))
        elif args.setcmd == 'fan-speed':
            set_fan_speed(int(d['speed']))
        elif args.setcmd == 'fantray-led':
            led_list = ['off', 'red', 'amber', 'green']
            if d['color'] not in led_list:
                print('Unsupported color for fan-tray-led. Choose from {}'.format(led_list))
                return
            set_fantray_led(int(d['index']), d['color'])
        elif args.setcmd == 'led':
            dev_list = ['port', 'fantray', 'sfm', 'board', 'master-psu', 'master-fan', 'master-sfm']
            if d['device'] not in dev_list:
                print('Unsupported device for led. Choose from {}'.format(dev_list))
                set_deviceled_parser.print_help()
                return

            led_list = ['off', 'red', 'amber', 'green']
            if d['color'] not in led_list:
                print('Unsupported color for led. Choose from {}'.format(led_list))
                return

            set_led(d['device'], d['color'])
        elif args.setcmd == 'log-level':
            set_log_level_module(d['level'], d['module'])
        elif args.setcmd == 'log-level-restore':
            set_log_restore_default()
        elif args.setcmd == 'asic-temp':
            if d['name'] is None or d['temp'] is None or d['threshold'] is None:
                set_asictemp_parser.print_help()
                return
            set_asic_temp(d['name'], int(d['temp']), int(d['threshold']))
        elif args.setcmd == 'shutdown-sfm':
            if not nokia_common.is_cpm():
                print('Command is only supported on Supervisor card')
                return
            
            set_shutdown_sfm(d['sfm-num'])
        elif args.setcmd == 'startup-sfm':
            if not nokia_common.is_cpm():
                print('Command is only supported on Supervisor card')
                return
            set_startup_sfm(d['sfm-num'])
        elif args.setcmd == 'ndk-monitor-action':
            action_list = ['warn','reboot','default']
            if d['action'] not in action_list:
                set_ndk_monitor_action_parser.print_help()
            else:
                set_ndk_monitor_action(d['action'])
        elif args.setcmd == 'ndk-log-level':
            level_list = ['trace', 'debug', 'info', 'notice', 'warning', 'error', 'critical', 'default']
            if d['level'] not in level_list:
                set_ndk_log_level_parser.print_help()
            else:
                set_ndk_log_level(d['level'])
        elif args.setcmd == 'reboot-linecard':
            if not nokia_common.is_cpm():
                print('Command is only supported on Supervisor card')
                return
            slot = d['slot']
            if slot < 1 or slot > 8:
                print("Error: Invalid slot number. Linecard slot number starts from 1 to 8")
                return
            force = d['force']
            if force != 'force':
                ans = input("Reboot linecard slot {}. Continue [y/n]?:".format(slot))
                if ans.strip().upper() != "Y":
                    print("Operation abort!")
                    return

            set_reboot_linecard(slot)
        elif args.setcmd == 'restart-lc-system-service':
            supported_list = ["interfaces-config.service"]
            if not nokia_common.is_cpm():
                print('Command is only supported on Supervisor card')
                return
            slot = d['slot']
            if slot < 1 or slot > 8:
                print("Error: Invalid slot number. Linecard slot number starts from 1 to 8")
                return
            service_name = d['service']
            if service_name not in supported_list:
                print("Error: Service name \"{}\" is not in supported list".format(service_name))
                print("Supported list: " + ", ".join(supported_list))
                return
            set_restart_lc_system_service(slot, service_name)
        else:
            set_parser.print_help()
        
    elif args.cmd == 'request':
        if args.reqcmd == 'devmgr-admintech':
            if d['filepath'] is None:
                print("Missing parameter: file path is mandatory\n")
                req_devmgr_admintech_parser.print_help()
                return
            request_devmgr_admintech(d['filepath'])
        elif args.reqcmd == 'ndk-admintech':
            request_ndk_admintech()
        else:
             req_parser.print_help()
    elif args.cmd == 'clear':
        if args.clearcmd == 'midplane':
          clear_midplane_port_counters()
        if args.clearcmd == 'qfpga':
          clear_qfpga_stats()
    else:
        base_parser.print_help()

if __name__ == "__main__":
    main()
