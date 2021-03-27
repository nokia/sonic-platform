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
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

chassis = Chassis()


def test_get_component_list():
    for component in chassis._component_list:
        name = component.get_name()
        assert name != ''
        descr = component.get_description
        assert descr != ''
        model = component.get_model()
        assert model != ''
        serial = component.get_serial()
        assert serial != ''
        presence = component.get_presence()
        assert presence is True
        status = component.get_status()
        assert status is True
        pos = component.get_position_in_parent()
        assert pos != ''
        rep = component.is_replaceable()
        assert rep != ''

        ver = component.get_firmware_version()
        assert ver != ''
        available = component.get_available_firmware_version("None")
        assert available != ''
        notify = component.get_firmware_update_notification("None")
        assert notify != ''
        assert component.install_firmware("None") is False
        assert component.update_firmware("None") is False
