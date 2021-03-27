#!/usr/bin/env python
#
# Name: test_chassis.py, version: 1.0
#
# Description: Module contains the definitions of Unit-test suite for
# Chassis related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform_base.chassis_base import ChassisBase
    from sonic_platform.chassis import Chassis
    from platform_ndk import nokia_common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


def test_status_led():
    chassis = Chassis()
    list = [ChassisBase.STATUS_LED_COLOR_OFF,
            ChassisBase.STATUS_LED_COLOR_RED,
            ChassisBase.STATUS_LED_COLOR_AMBER,
            ChassisBase.STATUS_LED_COLOR_GREEN]
    for color in list:
        print('{}'.format(color))
        chassis.set_status_led(color)
        print('Chassis get-led status is {}'.format(chassis.get_status_led()))
        assert color == chassis.get_status_led()


def test_chassis_info():
    chassis = Chassis()
    assert chassis.get_presence() is True
    assert chassis.get_status() is True
    reboot = chassis.get_reboot_cause()
    print("reboot_casue: {}".format(reboot))
    assert reboot != ''
    assert chassis.is_modular_chassis() is True
    slot = chassis.get_supervisor_slot()
    print("supervisor slot: {}".format(slot))
    assert slot == nokia_common.NOKIA_CPM_SLOT_NUMBER
    slot = chassis.get_my_slot()
    print("my slot: {}".format(slot))
    assert slot != ''
