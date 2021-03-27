#!/usr/bin/env python
#
# Name: test_sfp.py, version: 1.0
#
# Description: Module contains the definitions of Unit-test suite for
# SFP related APIs for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

import types
try:
    from sonic_platform.platform import Platform
    # from sonic_platform.sfp_event import sfp_event
    from platform_ndk import platform_ndk_pb2
    from sonic_platform.sfp import ALL_PAGES_TYPE
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

chassis = Platform().get_chassis()


def _get_index(sfp):
    return str(sfp.index)


def test_sfp_reset():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        if presence:
            original_mode = xcvr.get_reset_status()
            new_mode = original_mode ^ 1
            new_status = xcvr.reset(new_mode)
            new_mode = xcvr.get_reset_status()
            if new_mode != original_mode:
                # set it back
                if new_mode is True:
                    # take it out of reset or many other tests will fail
                    newest_status = xcvr.reset(False)
                    if ((not isinstance(original_mode, types.BooleanType)) or
                        (not isinstance(new_mode, types.BooleanType)) or
                            (not isinstance(newest_status, types.BooleanType))):
                        print('SFP {} type failure! orig_mode:{} new_mode:{} newest_status:{}'.format(
                            _get_index(xcvr), original_mode, new_mode, newest_status))
                        assert False
            else:
                print('SFP {} reset failed! orig_mode:{} new_mode:{} new_status:{}'.format(
                    _get_index(xcvr), original_mode, new_mode, new_status))
                assert False


def test_sfp_get_presence():
    count = 0
    for xcvr in chassis.get_all_sfps():
        bool_val = xcvr.get_presence()
        assert (isinstance(bool_val, types.BooleanType))
        if bool_val:
            count += 1
        print('SFP {} Presence: {}'.format(_get_index(xcvr), bool_val))
        # At least 1 SFP should be present
    assert count != 0


def test_sfp_get_serial():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        serial = xcvr.get_serial()
        if presence and not isinstance(serial, types.StringTypes):
            print('SFP {} Serial: {}'.format(_get_index(xcvr), serial))
            assert False


def test_sfp_get_status():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        assert (isinstance(status, types.BooleanType))
        print('SFP {} Presence: {} Status: {}'.format(_get_index(xcvr), presence, status))


def test_sfp_get_model():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        model = xcvr.get_model()
        if presence and not isinstance(model, types.StringTypes):
            print('SFP {} Model: {}'.format(_get_index(xcvr), model))
            assert False


def test_sfp_get_name():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        name = xcvr.get_name()
        if presence and not isinstance(name, types.StringTypes):
            print('SFP {} Name: {}'.format(_get_index(xcvr), name))
            assert False


def test_sfp_invalidate_page_cache():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        if presence:
            xcvr.invalidate_page_cache(platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_CTRL)
            xcvr.invalidate_page_cache(platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_DATA)
            xcvr.invalidate_page_cache(platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_EXT_CTRL)
            xcvr.invalidate_page_cache(platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_UPPER_PAGE)
            xcvr.invalidate_page_cache(ALL_PAGES_TYPE)


def test_sfp_SfpHasBeenTransitioned():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        if presence:
            xcvr.SfpHasBeenTransitioned(xcvr.index, False)


def test_sfp_debug_eeprom_ops():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        if presence:
            xcvr.debug_eeprom_ops(platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_DEBUG, 0, 128)
            # just one port worth is enough
            return


def test_sfp_position_in_parent():
    for xcvr in chassis.get_all_sfps():
        assert xcvr.get_position_in_parent() == -1


def test_sfp_is_replaceable():
    for xcvr in chassis.get_all_sfps():
        assert xcvr.is_replaceable() is True


def test_sfp_get_transceiver_info():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        if presence and status:
            xcvr_dict = xcvr.get_transceiver_info()
            if not isinstance(xcvr_dict, types.DictType):
                print('SFP {} xcvr_dict: {}'.format(_get_index(xcvr), xcvr_dict))
                assert False


def test_sfp_get_transceiver_bulk_status():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        if presence and status:
            xcvr_dict = xcvr.get_transceiver_bulk_status()
            if not isinstance(xcvr_dict, types.DictType):
                print('SFP {} xcvr_dict: {}'.format(_get_index(xcvr), xcvr_dict))
                assert False


def test_sfp_get_transceiver_threshold_info():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        if presence and status:
            xcvr_dict = xcvr.get_transceiver_threshold_info()
            if not isinstance(xcvr_dict, types.DictType):
                print('SFP {} xcvr_dict: {}'.format(_get_index(xcvr), xcvr_dict))
                assert False


def test_sfp_get_rx_los():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        dom_supported = xcvr.is_dom_supported()
        if presence and status and dom_supported:
            los_list = xcvr.get_rx_los()
            # return value could be none if assembly doens't support DOM
            if not isinstance(los_list, types.ListType):
                print('SFP {} los_list: {}'.format(_get_index(xcvr), los_list))
                assert False


def test_sfp_get_tx_fault():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        dom_supported = xcvr.is_dom_supported()
        if presence and status and dom_supported:
            fault_list = xcvr.get_tx_fault()
            # return value could be none if assembly doens't support DOM
            if not isinstance(fault_list, types.ListType):
                print('SFP {} fault_list: {}'.format(_get_index(xcvr), fault_list))
                assert False


def test_sfp_tx_disable():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        tx_disable_status = xcvr.tx_disable(True)
        assert (isinstance(tx_disable_status, types.BooleanType))
        dom_supported = xcvr.is_dom_supported()
        dom_tx_disable_supported = xcvr.is_dom_tx_disable_supported()
        if presence and status and dom_supported and dom_tx_disable_supported:
            disable_list = xcvr.get_tx_disable()
            if not isinstance(disable_list, types.ListType):
                print('SFP {} disable_list: {}'.format(_get_index(xcvr), disable_list))
                assert False
            channel_disable_list = xcvr.get_tx_disable_channel()
            if not isinstance(channel_disable_list, types.IntType):
                print('SFP {} channel_disable_list: {}'.format(_get_index(xcvr), channel_disable_list))
                assert False


def test_sfp_get_power_override():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        if presence and status:
            override_status = xcvr.get_power_override()
            if override_status is not NotImplementedError:
                if not isinstance(override_status, types.BooleanType):
                    print('SFP {} override_status: {}'.format(_get_index(xcvr), override_status))
                    assert False
            # one of these valid calls is enough
            return


def test_sfp_get_temperature():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        dom_supported = xcvr.is_dom_supported()
        if presence and status and dom_supported:
            temp = xcvr.get_temperature()
            if not isinstance(temp, types.FloatType) and temp is not None:
                print('SFP {} temp: {}'.format(_get_index(xcvr), temp))
                assert False


def test_sfp_get_voltage():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        dom_supported = xcvr.is_dom_supported()
        if presence and status and dom_supported:
            volts = xcvr.get_voltage()
            if not isinstance(volts, types.FloatType) and volts is not None:
                print('SFP {} volts: {}'.format(_get_index(xcvr), volts))
                assert False


def test_sfp_get_tx_power():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        dom_supported = xcvr.is_dom_supported()
        if presence and status and dom_supported:
            power_list = xcvr.get_tx_power()
            # return value could be none if assembly doens't support DOM
            if not isinstance(power_list, types.ListType):
                print('SFP {} power_list: {}'.format(_get_index(xcvr), power_list))
                assert False


def test_sfp_get_rx_power():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        dom_supported = xcvr.is_dom_supported()
        if presence and status and dom_supported:
            power_list = xcvr.get_rx_power()
            # return value could be none if assembly doens't support DOM
            if not isinstance(power_list, types.ListType):
                print('SFP {} power_list: {}'.format(_get_index(xcvr), power_list))
                assert False


def test_sfp_get_tx_bias():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        dom_supported = xcvr.is_dom_supported()
        if presence and status and dom_supported:
            bias_list = xcvr.get_tx_bias()
            # return value could be none if assembly doens't support DOM
            if not isinstance(bias_list, types.ListType):
                print('SFP {} bias_list: {}'.format(_get_index(xcvr), bias_list))
                assert False


def test_sfp_lpmode():
    for xcvr in chassis.get_all_sfps():
        presence = xcvr.get_presence()
        status = xcvr.get_status()
        if presence and status:
            original_lpmode = xcvr.get_lpmode()
            new_lpmode = original_lpmode ^ 1
            xcvr.set_lpmode(new_lpmode)
            new_lpmode = xcvr.get_lpmode()
            if new_lpmode != original_lpmode:
                # set it back
                newest_lpmode_status = xcvr.set_lpmode(original_lpmode)
                if (not isinstance(original_lpmode, types.BooleanType)) or \
                        (not isinstance(new_lpmode, types.BooleanType)) or \
                        (not isinstance(newest_lpmode_status, types.BooleanType)):
                    print('SFP {} orig_lpmode:{} new_lpmode:{} newest_mode_status:{}'.format(
                        _get_index(xcvr), original_lpmode, new_lpmode, newest_lpmode_status))
                    assert False
