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

import time
import types
import random
try:
    from sonic_platform_base.device_base import DeviceBase
    from sonic_platform.platform import Platform
    from sonic_platform.fan import Fan
    from platform_ndk import nokia_common
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

WAIT_TIME_SEC = 2
TEST_SPEED_PERCENTAGE = 90
chassis = Platform().get_chassis()


def _get_index(fantray):
    return str(fantray.fantray_idx)


def test_fantray_get_name():
    for fantray in chassis.get_all_fan_drawers():
        fantray_name = fantray.get_name()
        for fan in fantray.get_all_fans():
            fan_name = fan.get_name()
            assert fan_name != ''
        print('Fantray {} Name: {}'.format(_get_index(fantray), fantray_name))
        assert fantray_name != ''


def test_fantray_get_presence():
    count = 0
    for fantray in chassis.get_all_fan_drawers():
        bool_val = fantray.get_presence()
        assert (isinstance(bool_val, types.BooleanType))
        if bool_val:
            count += 1
        print('Fantray {} Presence: {}'.format(_get_index(fantray), bool_val))
        # At least 1 FanTray should be present
    assert count != 0


def test_fantray_get_status():
    count = 0
    for fantray in chassis.get_all_fan_drawers():
        presence = fantray.get_presence()
        status = fantray.get_status()

        if status:
            count += 1
            assert status == presence
        print('FanTray {} Presence: {} Status: {}'.format(_get_index(fantray), presence, status))
        # At least 1 FanTray should be present with valid status
    assert count != 0


def test_fantray_get_serial():
    for fantray in chassis.get_all_fan_drawers():
        presence = fantray.get_presence()
        serial = fantray.get_serial()
        if presence and not isinstance(serial, types.StringTypes):
            print('FanTray {} Serial: {}'.format(_get_index(fantray), serial))
            assert False


def test_fantray_get_model():
    for fantray in chassis.get_all_fan_drawers():
        presence = fantray.get_presence()
        model = fantray.get_model()
        if presence and not isinstance(model, types.StringTypes):
            print('FanTray {} Model: {}'.format(_get_index(fantray), model))
            assert False


def test_fantray_set_status_led():
    list = [DeviceBase.STATUS_LED_COLOR_OFF,
            DeviceBase.STATUS_LED_COLOR_RED,
            DeviceBase.STATUS_LED_COLOR_AMBER,
            DeviceBase.STATUS_LED_COLOR_GREEN]
    for fantray in chassis.get_all_fan_drawers():
        for color in list:
            fantray.set_status_led(color)
            assert color == fantray.get_status_led()


def test_fantray_position_in_parent():
    for fantray in chassis.get_all_fan_drawers():
        assert fantray.get_position_in_parent() != -1
        fan_list = fantray.get_all_fans()
        if len(fan_list):
            fan = fan_list[0]
            assert fan.get_position_in_parent() == -1


def test_fantray_is_replaceable():
    for fantray in chassis.get_all_fan_drawers():
        assert fantray.is_replaceable() is True
        fan_list = fantray.get_all_fans()
        if len(fan_list):
            fan = fan_list[0]
            assert fan.is_replaceable() is False


def test_fantray_check_max_consumed_power():
    for fantray in chassis.get_all_fan_drawers():
        status = fantray.get_status()

        if status is False:
            continue

        default_power = fantray.get_maximum_consumed_power()
        if not isinstance(default_power, types.FloatType):
            print('Fantray {} Max-Consumed-Power: {}'.format(_get_index(fantray), default_power))
            assert False

        rand_power = random.uniform(100, 500)
        fantray.set_maximum_consumed_power(rand_power)
        assert(rand_power == fantray.get_maximum_consumed_power())
        fantray.set_maximum_consumed_power(default_power)
        assert(default_power == fantray.get_maximum_consumed_power())


def test_fantray_get_direction():
    for fantray in chassis.get_all_fan_drawers():
        for fan in fantray.get_all_fans():
            direction = fan.get_direction()

            assert (direction == Fan.FAN_DIRECTION_INTAKE or
                    direction == Fan.FAN_DIRECTION_EXHAUST)


def test_fan_set_speed():
    fan = None
    for fantray in chassis.get_all_fan_drawers():
        fan_list = fantray.get_all_fans()
        print('Fan list {}'.format(fan_list))
        if len(fan_list):
            fan = fan_list[0]

    if fan is not None:
        default_speed = fan.get_target_speed()
        print("Fan-{} disabling fan-algorithm".format(_get_index(fan)))
        tolerance = fan.get_speed_tolerance()

        fan.disable_fan_algorithm(1)
        # Set new speed
        new_speed = random.randint(50, 100)
        time.sleep(WAIT_TIME_SEC * 5)
        fan.set_speed(new_speed)
        time.sleep(WAIT_TIME_SEC * 5)
        target_speed = fan.get_target_speed()
        actual_speed = fan.get_speed()

        # Validate new speed
        assert abs(actual_speed - target_speed) <= tolerance
        fan.set_speed(default_speed)
        time.sleep(WAIT_TIME_SEC * 5)
        fan.disable_fan_algorithm(0)


# Negative test for non CPM cards
def test_all_apis_for_non_cpm_card():
    for fantray in chassis.get_all_fan_drawers():
        fan_list = fantray.get_all_fans()
        print('Fan list {}'.format(fan_list))
        if len(fan_list):
            fan = fan_list[0]

    if fan is not None:
        fan.is_cpm = 0
        assert fan.get_model() == nokia_common.NOKIA_INVALID_STRING
        assert fan.get_serial() == nokia_common.NOKIA_INVALID_STRING
        assert fan.get_presence() is False
        assert fan.get_status() is False
        assert fan.get_speed() == 0
        assert fan.get_direction() == Fan.FAN_DIRECTION_EXHAUST
        assert fan.disable_fan_algorithm(0) is False
        assert fan.set_speed(50) is False
        assert fan.get_status_led() == Fan.STATUS_LED_COLOR_OFF
        assert fan.set_status_led(Fan.STATUS_LED_COLOR_OFF) is False


# This test will be at the end
def test_all_apis_for_empty_fan_list():
    for fantray in chassis.get_all_fan_drawers():
        chassis.empty_fan_list(int(_get_index(fantray)))
        assert fantray.get_model() == nokia_common.NOKIA_INVALID_STRING
        assert fantray.get_serial() == nokia_common.NOKIA_INVALID_STRING
        assert fantray.get_presence() is False
        assert fantray.get_status() is False
        assert fantray.get_status_led() == Fan.STATUS_LED_COLOR_OFF
        assert fantray.set_status_led(Fan.STATUS_LED_COLOR_OFF) is False
