# Name: nokia_cmd.py, version: 1.0
#
# Description: Module contains the test show, set and ut commands
# for interfacing with platform code
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

import sys
import getopt
import logging
import json
from google.protobuf.json_format import MessageToJson

import grpc
from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
from platform_ndk import platform_ndk_pb2_grpc

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


def generate_grpc_channel():
    channel = None

    server_path = nokia_common.NOKIA_DEVMGR_UNIX_SOCKET_PATH
    channel = grpc.insecure_channel(server_path)
    channel_ready = grpc.channel_ready_future(channel)
    try:
        channel_ready.result(timeout=1)
    except grpc.FutureTimeoutError:
        print('GRPC channel could not be established')
        return None

    return channel


def show_led(stub, port_start, port_end):
    led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_PORT
    port_idx = platform_ndk_pb2.ReqLedIndexPb(start_idx=port_start,
                                              end_idx=port_end)
    response = stub.ShowLed(platform_ndk_pb2.ReqLedInfoPb(led_type=led_type, led_idx=port_idx))
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


def show_psu(stub):
    arg1 = platform_ndk_pb2.ReqPsuInfoPb(psu_idx=0)
    ret, response = nokia_common.try_grpc(stub.ShowPsuInfo, arg1)

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


def show_psu_detail(stub):
    response = stub.ShowPsuInfo(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=0))

    if len(response.psu_info.psu_device) == 0:
        print('PSU Output not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
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
            item.append(str(p_response.output_current))
            p_response = stub.GetPsuOutputVoltage(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=i+1))
            item.append(str(p_response.output_voltage))
            p_response = stub.GetPsuOutputPower(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=i+1))
            item.append(str(p_response.output_power))
            p_response = stub.GetPsuTemperature(platform_ndk_pb2.ReqPsuInfoPb(psu_idx=i+1))
            item.append(str(p_response.ambient_temp))
            item_list.append(item)
        i += 1

    print('PSU TABLE DETAILS')
    print_table(field, item_list)
    return


def show_fan(stub):
    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=0)
    response = stub.ShowFanInfo(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))
    if len(response.fan_show.fan_device) == 0:
        print('Fan output not available on this card')
        return

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    field = []
    field.append('        Type         ')
    field.append('        Value                    ')

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


def show_fan_detail(stub):
    # Get Num-fans
    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=0)
    response = stub.GetFanNum(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx))

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    fan_msg = response.fan_nums
    num_fans = fan_msg.num_fantrays

    if num_fans == 0:
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

    print('FAN DETAIL TABLE')
    print_table(field, item_list)
    return


def show_temp(stub):
    response = stub.ShowThermalInfo(platform_ndk_pb2.ReqTempParamsPb())

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


def show_platform(stub):
    response = stub.GetChassisStatus(platform_ndk_pb2.ReqModuleInfoPb())
    if format_type == 'json-format':
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
        item.append(hw_device.state)
        m_response = stub.GetMidplaneIP(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=hw_device.slot_num))
        item.append(m_response.midplane_ip)
        item_list.append(item)
        i += 1

    print('PLATFORM INFO TABLE')
    print_table(field, item_list)
    return


def show_firmware(stub):
    response = stub.HwFirmwareGetComponents(platform_ndk_pb2.ReqHwFirmwareInfoPb())

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


def show_sfm_summary(stub):
    req_etype = platform_ndk_pb2.ReqSfmOpsType.SFM_OPS_SHOW_SUMMARY
    response = stub.ReqSfmInfo(platform_ndk_pb2.ReqSfmInfoPb(type=req_etype))

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
        item_list.append(item)
        i += 1

    print('SFM SUMMARY TABLE')
    print_table(field, item_list)
    return


def show_sfm_imm_links(stub, imm_slot):
    req_etype = platform_ndk_pb2.ReqSfmOpsType.SFM_OPS_SHOW_IMMLINKS
    response = stub.ReqSfmInfo(platform_ndk_pb2.ReqSfmInfoPb(type=req_etype, imm_slot=imm_slot))

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


def show_system_led(stub):
    led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_ALL
    response = stub.ShowLed(platform_ndk_pb2.ReqLedInfoPb(led_type=led_type))
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


def show_syseeprom(chassis):
    eeprom = chassis.get_system_eeprom_info()
    if format_type == 'json-format':
        ret = json.dumps(eeprom, indent=True)
        print(ret)
        return

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
    print_table(field, item_list)
    return


def show_chassis(stub):
    response = stub.GetChassisProperties(platform_ndk_pb2.ReqModuleInfoPb())
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


def show_power(stub):
    response = stub.GetModuleMaxPower(platform_ndk_pb2.ReqModuleInfoPb())

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


def show_midplane_status(stub, hw_slot):
    response = stub.IsMidplaneReachable(platform_ndk_pb2.ReqModuleInfoPb(hw_slot=hw_slot))

    if format_type == 'json-format':
        json_response = MessageToJson(response)
        print(json_response)
        return

    print('Reachability to slot {} is {}'.format(hw_slot, response.midplane_status))
    return


def show_logging(stub):
    response = stub.ShowLogAll(platform_ndk_pb2.ReqLogSettingPb())

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


def show_ndk_eeprom(stub):
    response1 = stub.GetCardProductName(platform_ndk_pb2.ReqEepromInfoPb())
    response2 = stub.GetCardSerialNumber(platform_ndk_pb2.ReqEepromInfoPb())
    response3 = stub.GetCardPartNumber(platform_ndk_pb2.ReqEepromInfoPb())
    response4 = stub.GetCardBaseMac(platform_ndk_pb2.ReqEepromInfoPb())
    response5 = stub.GetCardMacCount(platform_ndk_pb2.ReqEepromInfoPb())

    if format_type == 'json-format':
        print('Json output not supported')
        return

    field = []
    field.append('Field                     ')
    field.append('Value                     ')

    item_list = []
    item_list.append(['Card ProductName       ', str(response1.card_product_name)])
    item_list.append(['Card SerialNum         ', str(response2.card_serial_num)])
    item_list.append(['Card PartNum           ', str(response3.card_part_num)])
    item_list.append(['Card BaseMac           ', str(response4.card_base_mac)])
    item_list.append(['Card MacCount          ', str(response5.card_mac_count)])
    print('CARD-EEPROM INFORMATION')
    print_table(field, item_list)

    response = stub.GetChassisEeprom(platform_ndk_pb2.ReqEepromInfoPb())
    item_list = []
    chassis_eeprom = response.chassis_eeprom
    item_list.append(['Chassis Type           ', str(platform_ndk_pb2.HwChassisType.Name(chassis_eeprom.chassis_type))])
    item_list.append(['Chassis SerialNum      ', str(chassis_eeprom.chassis_serial_num)])
    item_list.append(['Chassis PartNum        ', str(chassis_eeprom.chassis_part_num)])
    item_list.append(['Chassis CleiNum        ', str(chassis_eeprom.chassis_clei_num)])
    item_list.append(['Chassis MfgDate        ', str(chassis_eeprom.chassis_mfg_date)])
    item_list.append(['Chassis BaseMac        ', str(chassis_eeprom.chassis_base_mac)])
    item_list.append(['Chassis MacCount       ', str(chassis_eeprom.chassis_mac_count)])

    print('CHASSIS-EEPROM INFORMATION')
    print_table(field, item_list)
    return


def request_admintech(stub, filepath):
    stub.ReqAdminTech(platform_ndk_pb2.ReqAdminTechPb(admintech_path=filepath))


def set_temp_offset(stub, offset):
    stub.SetThermalOffset(platform_ndk_pb2.ReqTempParamsPb(temp_offset=offset))


def set_fan_algo_disable(stub, disable):
    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=1)
    req_fan_algo = platform_ndk_pb2.SetFanTrayAlgorithmPb(fantray_algo_disable=disable)
    stub.DisableFanAlgorithm(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx, fan_algo=req_fan_algo))


def set_fan_speed(stub, speed):
    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=1)
    stub.SetFanTargetSpeed(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx,
        fantray_speed=speed))


def set_fantray_led(stub, index, color):
    req_idx = platform_ndk_pb2.ReqFanTrayIndexPb(fantray_idx=index)
    _led_info = nokia_common.led_color_to_info(color)
    stub.SetFanLedStatus(platform_ndk_pb2.ReqFanTrayOpsPb(idx=req_idx, led_info=_led_info))


def set_fp_port_led(led_stub, port_id, port_up):
    led_type = platform_ndk_pb2.ReqLedType.LED_TYPE_PORT
    _intf_info = platform_ndk_pb2.UpdateIntfInfoPb(port_name='FPport'+str(port_id), port_up=port_up)
    response = led_stub.SetLed(platform_ndk_pb2.ReqLedInfoPb(led_type=led_type,
        intf_info=_intf_info))


def set_led(stub, type_str, color):
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

    _led_info = nokia_common.led_color_to_info(color)
    stub.SetLed(platform_ndk_pb2.ReqLedInfoPb(led_type=led_type, led_info=_led_info))


def set_log_level_module(stub, level, module):
    if (module == 'all'):
        _log_type = platform_ndk_pb2.ReqLogType.REQ_LOG_SET_ALL
        _log_info = platform_ndk_pb2.ReqLogInfoPb(level_current=level, module=module)
        stub.ReqLogSetAll(platform_ndk_pb2.ReqLogSettingPb(log_type=_log_type, log_info=_log_info))
    else:
        _log_type = platform_ndk_pb2.ReqLogType.REQ_LOG_SET_MODULE
        _log_info = platform_ndk_pb2.ReqLogInfoPb(level_current=level, module=module)
        stub.ReqLogSetModule(platform_ndk_pb2.ReqLogSettingPb(log_type=_log_type, log_info=_log_info))


def set_log_restore_default(stub):
    _log_type = platform_ndk_pb2.ReqLogType.REQ_LOG_RESET_ALL
    stub.ReqLogResetAll(platform_ndk_pb2.ReqLogSettingPb(log_type=_log_type))


def main():
    global DEBUG
    global args
    global format_type

    if len(sys.argv) < 2:
        show_all_help()

    options, args = getopt.getopt(sys.argv[1:], 'hd', ['help', 'debug'])
    for opt, arg in options:
        if opt in ('-h', '--help'):
            show_all_help()
        elif opt in ('-d', '--debug'):
            DEBUG = True
            logging.basicConfig(level=logging.INFO)
        else:
            logging.info('no option')

    num_args = 1
    arg = args[0]
    if arg == 'show':
        num_args += 1
        if not validate(arg, num_args, len(args)):
            return

        show_arg = args[1]
        num_args += 1
        if num_args == len(args):
            format_type = args[2]

        channel = generate_grpc_channel()
        if not channel:
            return
        if show_arg == 'sensors':
            thermal_stub = platform_ndk_pb2_grpc.ThermalPlatformNdkServiceStub(channel)
            show_temp(thermal_stub)
            fan_stub = platform_ndk_pb2_grpc.FanPlatformNdkServiceStub(channel)
            show_fan(fan_stub)
            show_fan_detail(fan_stub)
        elif show_arg == 'platform':
            chassis_stub = platform_ndk_pb2_grpc.ChassisPlatformNdkServiceStub(channel)
            show_platform(chassis_stub)
        elif show_arg == 'psus':
            psu_stub = platform_ndk_pb2_grpc.PsuPlatformNdkServiceStub(channel)
            show_psu(psu_stub)
            show_psu_detail(psu_stub)
        elif show_arg == 'fp-status':
            led_stub = platform_ndk_pb2_grpc.LedPlatformNdkServiceStub(channel)
            show_led(led_stub, 0, 35)
        elif show_arg == 'firmware':
            firmware_stub = platform_ndk_pb2_grpc.FirmwarePlatformNdkServiceStub(channel)
            show_firmware(firmware_stub)
        elif show_arg == 'sfm-summary':
            stub = platform_ndk_pb2_grpc.UtilPlatformNdkServiceStub(channel)
            if stub is None:
                return
            show_sfm_summary(stub)
        elif show_arg == 'sfm-imm-links':
            if not validate(arg, num_args, len(args)):
                return

            imm_slot = int(args[2])
            num_args += 1
            if num_args == len(args):
                format_type = args[3]

            stub = platform_ndk_pb2_grpc.UtilPlatformNdkServiceStub(channel)
            if stub is None:
                return
            show_sfm_imm_links(stub, imm_slot)
        elif show_arg == 'system-leds':
            led_stub = platform_ndk_pb2_grpc.LedPlatformNdkServiceStub(channel)
            show_system_led(led_stub)
        elif show_arg == "syseeprom":
            import sonic_platform.platform
            platform_chassis = sonic_platform.platform.Platform().get_chassis()
            show_syseeprom(platform_chassis)
        elif show_arg == "chassis":
            chassis_stub = platform_ndk_pb2_grpc.ChassisPlatformNdkServiceStub(channel)
            show_chassis(chassis_stub)
        elif show_arg == "power":
            chassis_stub = platform_ndk_pb2_grpc.ChassisPlatformNdkServiceStub(channel)
            show_power(chassis_stub)
        elif show_arg == 'midplane':
            if not validate(arg, num_args, len(args)):
                return

            hw_slot = int(args[2])
            num_args += 1
            chassis_stub = platform_ndk_pb2_grpc.ChassisPlatformNdkServiceStub(channel)
            show_midplane_status(chassis_stub, hw_slot)
        elif show_arg == "logging":
            util_stub = platform_ndk_pb2_grpc.UtilPlatformNdkServiceStub(channel)
            show_logging(util_stub)
        elif show_arg == "ndk-eeprom":
            eeprom_stub = platform_ndk_pb2_grpc.EepromPlatformNdkServiceStub(channel)
            show_ndk_eeprom(eeprom_stub)
        return

    elif arg == 'set':
        num_args += 1
        if not validate(arg, num_args, len(args)):
            return

        channel = generate_grpc_channel()
        if not channel:
            return
        set_arg = args[1]
        if set_arg == 'temp-offset':
            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            set_val = int(args[2])
            thermal_stub = platform_ndk_pb2_grpc.ThermalPlatformNdkServiceStub(channel)
            set_temp_offset(thermal_stub, set_val)
        elif set_arg == 'fan-algo-disable':
            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            set_val = int(args[2])
            fan_stub = platform_ndk_pb2_grpc.FanPlatformNdkServiceStub(channel)
            set_fan_algo_disable(fan_stub, set_val)
        elif set_arg == 'fan-speed-percent':
            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            set_val = int(args[2])
            fan_stub = platform_ndk_pb2_grpc.FanPlatformNdkServiceStub(channel)
            set_fan_speed(fan_stub, set_val)
        elif set_arg == 'fantray-led':
            led_list = ['off', 'red', 'amber', 'green']
            num_args += 2
            if not validate(arg, num_args, len(args)):
                return

            index = int(args[2])
            if args[3] not in led_list:
                print('Unsupported color for fan-tray-led. Choose from {}'.format(led_list))
                return

            fan_stub = platform_ndk_pb2_grpc.FanPlatformNdkServiceStub(channel)
            set_fantray_led(fan_stub, index, args[3])
        elif set_arg == 'fp-portup':
            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            set_val = int(args[2])
            led_stub = platform_ndk_pb2_grpc.LedPlatformNdkServiceStub(channel)
            set_fp_port_led(led_stub, set_val, True)
        elif set_arg == 'fp-portdown':
            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            set_val = int(args[2])
            led_stub = platform_ndk_pb2_grpc.LedPlatformNdkServiceStub(channel)
            set_fp_port_led(led_stub, set_val, False)
        elif set_arg == 'led':
            num_args += 2
            if not validate(arg, num_args, len(args)):
                return

            type_str = args[2]
            color_str = args[3]
            chassis_stub = platform_ndk_pb2_grpc.LedPlatformNdkServiceStub(channel)
            set_led(chassis_stub, type_str, color_str)
        elif set_arg == 'log-level':
            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            if args[2] == 'help':
                print("Levels - \n" +
                      "\t0 or EMERG\n" +
                      "\t1 or ALERT\n" +
                      "\t2 or CRIT\n" +
                      "\t3 or ERR\n" +
                      "\t4 or WARNING\n" +
                      "\t5 or NOTICE\n" +
                      "\t6 or INFO\n" +
                      "\t7 or DEBUG\n" +
                      "\t8 or TRACE\n")
                print("Modules - \n" +
                      "\tall\n" +
                      "\tcommon\n")
                return

            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            level_str = args[2]
            module_str = args[3]
            util_stub = platform_ndk_pb2_grpc.UtilPlatformNdkServiceStub(channel)
            set_log_level_module(util_stub, level_str, module_str)
        elif set_arg == 'log-level-restore':
            util_stub = platform_ndk_pb2_grpc.UtilPlatformNdkServiceStub(channel)
            set_log_restore_default(util_stub)
        return
    elif arg == 'ut':
        num_args += 1
        if not validate(arg, num_args, len(args)):
            return

        ut_arg = args[1]
        if ut_arg == 'fan':
            # Load new platform api class
            try:
                import sonic_platform.platform
                platform_chassis = sonic_platform.platform.Platform().get_chassis()
            except Exception as e:
                print('Failed to load chassis due to '+str(e))
        elif ut_arg == 'sfp':
            # Load new platform api class
            try:
                import sonic_platform.platform
                platform_chassis = sonic_platform.platform.Platform().get_chassis()
            except Exception as e:
                print('Failed to load chassis due to '+str(e))
        elif ut_arg == 'chassis':
            # Load new platform api class
            try:
                import sonic_platform.platform
                platform_chassis = sonic_platform.platform.Platform().get_chassis()
                platform_chassis.test_suite_chassis()
            except Exception as e:
                print('Failed to load chassis due to '+str(e))
        elif ut_arg == 'psu':
            # Load new platform api class
            try:
                import sonic_platform.platform
                platform_chassis = sonic_platform.platform.Platform().get_chassis()
                platform_chassis.test_suite_psu()
            except Exception as e:
                print('Failed to load chassis due to '+str(e))
        elif ut_arg == 'card':
            # Load new platform api class
            try:
                import sonic_platform.platform
                platform_chassis = sonic_platform.platform.Platform().get_chassis()
            except Exception as e:
                print('Failed to load chassis due to '+str(e))
        elif ut_arg == 'firmware':
            # Load new platform api class
            try:
                import sonic_platform.platform
                platform_chassis = sonic_platform.platform.Platform().get_chassis()
            except Exception as e:
                print('Failed to load chassis due to '+str(e))
        return
    elif arg == 'request':
        num_args += 1
        if not validate(arg, num_args, len(args)):
            return

        debug_arg = args[1]
        channel = generate_grpc_channel()
        if not channel:
            return
        if debug_arg == 'admintech':
            num_args += 1
            if not validate(arg, num_args, len(args)):
                return

            filepath_val = args[2]
            stub = platform_ndk_pb2_grpc.UtilPlatformNdkServiceStub(channel)
            if stub is None:
                return
            request_admintech(stub, filepath_val)
    else:
        show_all_help()

    return True


def validate(cmd, num_args, passed_args):
    if passed_args < num_args:
        if (cmd == 'show'):
            show_help()
        elif (cmd == 'set'):
            show_set_help()
        elif (cmd == 'ut'):
            show_ut_help()
        elif (cmd == 'request'):
            show_request_help()
        else:
            print('Unsupported command')
        return 0
    return 1


def show_help():
    print("show platform            [json-format]      - Show Platform Information")
    print("show sensors             [json-format]      - Show Temperature/Fan Information")
    print("show psus                [json-format]      - Show PSUs Information")
    print("show fp-status           [json-format]      - Show FrontPanel LEDs")
    print("show firmware            [json-format]      - Show Firmware")
    print("show system-leds         [json-format]      - Show System LEDs")
    print("show sfm-summary         [json-format]      - Show SFM summary")
    print("show sfm-imm-links   [imm-slot] [json-format]    - Show SFM summary")
    print("show syseeprom           [json-format]      - Show system eeprom")
    print("show chassis             [json-format]      - Show chassis info")
    print("show psu-master-led      [json-format]      - Show PSU master-led status")
    print("show power               [json-format]      - Show power consumed")
    print("show midplane  [hw-slot] [json-format]      - Show midplane reachability")
    print("show logging             [json-format]      - Show logging information")
    print("show ndk-eeprom          [json-format]      - Show eeprom information")


def show_set_help():
    print("set temp-offset <0-100>          - Set temperature offset")
    print("set fan-algo-disable <1/0>       - Set 1 to disable")
    print("set fan-speed-percent <0-100>    - Set fan speed manually")
    print("set fantray-led <index> <color>  - Set fantry led color")
    print("set fp-portup <1-36>             - Set FP port up")
    print("set fp-portdown <1-36>           - Set FP port down")
    print("set led <type> <color>           - Set <port/fantray/sfm/board/master-psu/master-fan/master-sfm> <off/red/amber/green>")
    print("set log-level <level> <module>   - Set <level> <module> or <help>")
    print("set log-level-restore            - Reset log to default")


def show_ut_help():
    print("ut chassis                       - To run chassis unit-tests")
    print("ut psu                           - To run psu unit-tests")
    print("ut fan                           - To run fan unit-tests")
    print("ut sfp                           - To run sfp unit-tests")
    print("ut card                          - To run card unit-tests")
    print("ut firmware                      - To run firmware unit-tests")


def show_request_help():
    print("request admintech <full-file-path>  - Request admin-tech dump")


def show_all_help():
    show_help()
    show_set_help()
    show_ut_help()
    show_request_help()
    sys.exit(0)


if __name__ == "__main__":
    main()
