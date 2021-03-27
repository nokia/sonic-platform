#!/usr/bin/env python
#
# Name: test_fan.py, version: 1.0
#
# Description: Module contains the definitions of Unit-test suite for
# FAN related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform.chassis import Chassis
    from sonic_platform.eeprom import Eeprom
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def test_eeprom_apis():
    chassis = Chassis()
    mac = chassis.get_base_mac()
    assert mac != ''
    serial = chassis.get_serial_number()
    serial2 = chassis.get_serial()
    assert serial == serial2
    name = chassis.get_name()
    assert name != ''
    part = Eeprom().get_part_number()
    assert part != ''
    print(' ')
    print("product Name: {}".format(name))
    print("    base mac: {}".format(mac))
    print("      serial: {}".format(serial))
    print(" part number: {}".format(part))


def test_get_system_eeprom_info():
    # remove the eeprom file to trigger the GRPC to
    # get the eeprom again
    eeprom = Eeprom()
    eeprom.reset()

    chassis = Chassis()
    eeprom_info = chassis.get_system_eeprom_info()
    assert eeprom_info != ''
    print('')
    for key, value in eeprom_info.items():
        print("{}: {}".format(key, value))
