#!/usr/bin/env python
#
# Name: test_module.py, version: 1.0
#
# Description: Module contains the definitions of Unit-test suite for
# PSU related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

import types
import random
try:
    from sonic_platform_base.module_base import ModuleBase
    from sonic_platform.platform import Platform
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

chassis = Platform().get_chassis()


def _get_index(module):
    return str(module.module_index)


def test_module_get_presence():
    count = 0
    for module in chassis.get_all_modules():
        bool_val = module.get_presence()
        assert (isinstance(bool_val, types.BooleanType))
        if bool_val:
            count += 1
        print('PSU {} Presence: {}'.format(_get_index(module), bool_val))
        # At least 1 PSU should be present
    assert count != 0


def test_module_get_name():
    for module in chassis.get_all_modules():
        module_type = module.get_type()
        assert module.get_name().startswith(module_type)
        assert module.get_description() != ''


def test_module_get_presence_2():
    count = 0
    for module in chassis.get_all_modules():
        bool_val = module.get_presence()
        assert (isinstance(bool_val, types.BooleanType))
        if bool_val:
            count += 1
        print('Module {} Presence: {}'.format(module.get_name(), bool_val))
        # At least 1 Module(self) should be present
    assert count != 0


def test_module_get_status():
    count = 0
    for module in chassis.get_all_modules():
        presence = module.get_presence()
        status = module.get_status()

        if status:
            count += 1
            assert status == presence
        print('Module {} Presence: {} Status: {}'.format(module.get_slot(), presence, status))
        # At least 1 Module should be present with valid status
    assert count != 0


def test_module_check_max_consumed_power():
    for module in chassis.get_all_modules():
        if module.get_status() is False:
            continue

        default_power = module.get_maximum_consumed_power()
        if not isinstance(default_power, types.FloatType):
            print('Module {} Max-Consumed-Power: {}'.format(module.get_slot(), default_power))
            assert False

        rand_power = random.uniform(100, 500)
        module.set_maximum_consumed_power(rand_power)
        assert(rand_power == module.get_maximum_consumed_power())
        module.set_maximum_consumed_power(default_power)
        assert(default_power == module.get_maximum_consumed_power())


def test_module_check_midplane():
    for module in chassis.get_all_modules():
        if module.get_status() is False:
            continue

        if module.get_type() == ModuleBase.MODULE_TYPE_FABRIC:
            continue

        if module.get_slot() == chassis.get_my_slot():
            continue

        midplane_ip_str = str(module.get_midplane_ip())
        print('Midplane-IP {}'.format(midplane_ip_str))
        assert module.is_midplane_reachable() is not False
        assert midplane_ip_str.startswith('10.0')


def test_module_sanity():
    for module in chassis.get_all_modules():
        if module.get_status() is False:
            continue

        assert module.is_replaceable() is True
        assert module.get_position_in_parent() != -1
        assert module.set_admin_state(True) is False
