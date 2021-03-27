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
    from sonic_platform.platform import Platform
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

INVALID_TEMP = -128.0
chassis = Platform().get_chassis()


def _get_index(thermal):
    return str(thermal.index)


def test_thermal_get_name():
    for thermal in chassis.get_all_thermals():
        thermal_name = thermal.get_name()
        print('Thermal {} Name: {}'.format(_get_index(thermal), thermal_name))
        assert thermal_name != ''


def test_thermal_get_presence():
    count = 0
    for thermal in chassis.get_all_thermals():
        bool_val = thermal.get_presence()
        assert (isinstance(bool_val, types.BooleanType))
        if bool_val:
            count += 1
        # All thermals should be present
    assert count == chassis.get_num_thermals()


def test_thermal_get_status():
    count = 0
    for thermal in chassis.get_all_thermals():
        presence = thermal.get_presence()
        status = thermal.get_status()

        if status:
            count += 1
            assert status == presence
        print('Thermal {} Presence: {} Status: {}'.format(_get_index(thermal), presence, status))
        # All thermals should be present with valid status
    assert count == chassis.get_num_thermals()


def test_thermal_get_serial():
    for thermal in chassis.get_all_thermals():
        presence = thermal.get_presence()
        serial = thermal.get_serial()
        if presence and not isinstance(serial, types.StringTypes):
            print('Thermal {} Serial: {}'.format(_get_index(thermal), serial))
            assert False


def test_thermal_get_model():
    for thermal in chassis.get_all_thermals():
        presence = thermal.get_presence()
        model = thermal.get_model()
        if presence and not isinstance(model, types.StringTypes):
            print('Thermal {} Model: {}'.format(_get_index(thermal), model))
            assert False


def test_thermal_get_low_threshold():
    for thermal in chassis.get_all_thermals():
        low_threshold = thermal.get_low_threshold()
        if not isinstance(low_threshold, types.FloatType):
            print('Thermal {} Min-Threshold: {}'.format(_get_index(thermal), low_threshold))
            assert False

        low_critical_threshold = thermal.get_low_critical_threshold()
        if not isinstance(low_critical_threshold, types.FloatType):
            print('Thermal {} Min-Critical-Threshold: {}'.format(_get_index(thermal), low_critical_threshold))
            assert False

        temperature = thermal.get_temperature()
        assert temperature >= low_threshold
        assert low_threshold > low_critical_threshold
        print('Thermal {} Temperature {} > Min-Threshold {} > '
              'Min-Critical-Threshold: {}'.format(_get_index(thermal),
                                                  temperature, low_threshold, low_critical_threshold))


def test_thermal_get_high_threshold():
    for thermal in chassis.get_all_thermals():
        high_threshold = thermal.get_high_threshold()
        if not isinstance(high_threshold, types.FloatType):
            print('Thermal {} Max-Threshold: {}'.format(_get_index(thermal), high_threshold))
            assert False

        high_critical_threshold = thermal.get_high_critical_threshold()
        if not isinstance(high_threshold, types.FloatType):
            print('Thermal {} Max-Critical-Threshold: {}'.format(_get_index(thermal),
                                                                 high_critical_threshold))
            assert False

        temperature = thermal.get_temperature()
        assert temperature <= high_threshold
        assert high_threshold < high_critical_threshold
        print('Thermal {} Temperature {} > Max-Threshold {} > '
              'Max-Critical-Threshold: {}'.format(_get_index(thermal),
                                                  temperature, high_threshold, high_critical_threshold))


def test_thermal_minmax_temperature():
    for thermal in chassis.get_all_thermals():
        min_temp = thermal.get_minimum_recorded()
        max_temp = thermal.get_maximum_recorded()

        temperature = thermal.get_temperature()
        if min_temp != INVALID_TEMP:
            assert temperature >= min_temp
        if max_temp != INVALID_TEMP:
            assert temperature <= max_temp


def test_thermal_position_in_parent():
    for thermal in chassis.get_all_thermals():
        assert thermal.get_position_in_parent() == -1


def test_thermal_is_replaceable():
    for thermal in chassis.get_all_thermals():
        assert thermal.is_replaceable() is False
