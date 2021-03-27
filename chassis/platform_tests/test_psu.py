#!/usr/bin/env python
#
# Name: test_psu.py, version: 1.0
#
# Description: Module contains the definitions of Unit-test suite for
# PSU related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

import types
try:
    from sonic_platform_base.device_base import DeviceBase
    from sonic_platform.platform import Platform
    from sonic_platform.psu import Psu
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

chassis = Platform().get_chassis()


def _get_index(psu):
    return str(psu.index)


def test_psu_get_presence():
    count = 0
    for psu in chassis._get_psu_list():
        bool_val = psu.get_presence()
        assert (isinstance(bool_val, types.BooleanType))
        if bool_val:
            count += 1
        print('PSU {} Presence: {}'.format(_get_index(psu), bool_val))
        # At least 1 PSU should be present
    assert count != 0


def test_psu_get_status():
    count = 0
    for psu in chassis._get_psu_list():
        presence = psu.get_presence()
        status = psu.get_status()

        if status:
            count += 1
            assert status == presence
        print('PSU {} Presence: {} Status: {}'.format(_get_index(psu), presence, status))
        # At least 1 PSU should be present with valid status
    assert count != 0


def test_psu_get_serial():
    for psu in chassis._get_psu_list():
        presence = psu.get_presence()
        serial = psu.get_serial()
        if presence and not isinstance(serial, types.StringTypes):
            print('PSU {} Serial: {}'.format(_get_index(psu), serial))
            assert False


def test_psu_get_model():
    for psu in chassis._get_psu_list():
        presence = psu.get_presence()
        model = psu.get_model()
        if presence and not isinstance(model, types.StringTypes):
            print('PSU {} Model: {}'.format(_get_index(psu), model))
            assert False


def test_psu_get_min_voltage():
    for psu in chassis._get_psu_list():
        status = psu.get_status()

        if status is False:
            continue

        min_voltage = psu.get_voltage_low_threshold()
        if not isinstance(min_voltage, types.FloatType):
            print('PSU {} Min-Voltage: {}'.format(_get_index(psu), min_voltage))
            assert False

        voltage = psu.get_voltage()
        print('PSU {} Voltage {} < Min-Voltage: {}'.format(_get_index(psu),
                                                           voltage, min_voltage))
        assert voltage >= min_voltage


def test_psu_get_max_voltage():
    for psu in chassis._get_psu_list():
        status = psu.get_status()

        if status is False:
            continue

        max_voltage = psu.get_voltage_high_threshold()
        if not isinstance(max_voltage, types.FloatType):
            print('PSU {} Max-Voltage: {}'.format(_get_index(psu), max_voltage))
            assert False

        voltage = psu.get_voltage()
        print('PSU {} Voltage {} > Max-Voltage: {}'.format(_get_index(psu),
                                                           voltage, max_voltage))
        assert voltage <= max_voltage


def test_psu_get_max_temperature():
    for psu in chassis._get_psu_list():
        status = psu.get_status()

        if status is False:
            continue

        max_temp = psu.get_temperature_high_threshold()
        if not isinstance(max_temp, types.FloatType):
            print('PSU {} Max-Voltage: {}'.format(_get_index(psu), max_temp))
            assert False

        temp = psu.get_temperature()
        print('PSU {} Temp {} > Max-Temp: {}'.format(_get_index(psu),
                                                     temp, max_temp))
        assert temp <= max_temp


def test_psu_get_current():
    for psu in chassis._get_psu_list():
        status = psu.get_status()

        if status is False:
            continue

        current = psu.get_current()
        if not isinstance(current, types.FloatType):
            print('PSU {} Current: {}'.format(_get_index(psu), current))
            assert False


def test_psu_get_power():
    for psu in chassis._get_psu_list():
        status = psu.get_status()

        if status is False:
            continue

        power = psu.get_power()
        if not isinstance(power, types.FloatType):
            print('PSU {} Power: {}'.format(_get_index(psu), power))
            assert False


def test_psu_get_max_supplied_power():
    for psu in chassis._get_psu_list():
        status = psu.get_status()

        if status is False:
            continue

        power = psu.get_maximum_supplied_power()
        if not isinstance(power, types.FloatType):
            print('PSU {} Max-Supplied-Power: {}'.format(_get_index(psu), power))
            assert False


def test_psu_master_led_status():
    list = [DeviceBase.STATUS_LED_COLOR_OFF,
            DeviceBase.STATUS_LED_COLOR_RED,
            DeviceBase.STATUS_LED_COLOR_AMBER,
            DeviceBase.STATUS_LED_COLOR_GREEN]
    for color in list:
        Psu.set_status_master_led(color)
        assert color == Psu.get_status_master_led()


def test_psu_position_in_parent():
    for psu in chassis._get_psu_list():
        assert psu.get_position_in_parent() == -1


def test_psu_is_replaceable():
    for psu in chassis._get_psu_list():
        assert psu.is_replaceable() is True
