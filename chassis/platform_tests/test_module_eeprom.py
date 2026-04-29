#!/usr/bin/env python3
#
# test_module_eeprom.py
#
# Unit tests for Module._get_module_eeprom_info().
#

import os
import unittest
from unittest.mock import MagicMock, patch

from sonic_platform.module import Module
from sonic_platform_base.module_base import ModuleBase


def _make_module():
    stub = MagicMock()
    return Module(
        module_index=0,
        module_name="LINE-CARD0",
        module_type=ModuleBase.MODULE_TYPE_LINE,
        module_slot=1,
        is_cpm=False,
        stub=stub,
    )


class TestGetModuleEepromInfo(unittest.TestCase):

    def _patched(self, tbl_mock):
        db_mock = MagicMock()
        p1 = patch("sonic_platform.module.daemon_base.db_connect", return_value=db_mock)
        p2 = patch("sonic_platform.module.swsscommon.Table", return_value=tbl_mock)
        return p1, p2

    def test_normal_eeprom_dict(self):
        """Normal dict round-trips correctly through ast.literal_eval."""
        eeprom_dict = {"0x24": "40:7C:7D:BB:27:21", "0x23": "EAG2-02-143"}
        tbl = MagicMock()
        tbl.get.return_value = (True, [("eeprom_info", str(eeprom_dict))])

        mod = _make_module()
        mod.get_presence = MagicMock(return_value=True)
        mod.get_name = MagicMock(return_value="LINE-CARD0")

        p1, p2 = self._patched(tbl)
        with p1, p2:
            result = mod._get_module_eeprom_info()

        self.assertEqual(result, eeprom_dict)

    def test_key_not_found_returns_none(self):
        """Missing DB key returns None."""
        tbl = MagicMock()
        tbl.get.return_value = (False, [])

        mod = _make_module()
        mod.get_presence = MagicMock(return_value=True)
        mod.get_name = MagicMock(return_value="LINE-CARD0")

        p1, p2 = self._patched(tbl)
        with p1, p2:
            result = mod._get_module_eeprom_info()

        self.assertIsNone(result)

    def test_not_present_returns_none(self):
        """Absent linecard returns None without touching DB."""
        mod = _make_module()
        mod.get_presence = MagicMock(return_value=False)
        self.assertIsNone(mod._get_module_eeprom_info())

    def test_injection_raises_not_executes(self):
        """Injected Python expression raises ValueError and does not execute."""
        probe_file = "/tmp/poc_f031_test.txt"
        if os.path.exists(probe_file):
            os.remove(probe_file)

        payload = f"__import__('os').system('touch {probe_file}')"
        tbl = MagicMock()
        tbl.get.return_value = (True, [("eeprom_info", payload)])

        mod = _make_module()
        mod.get_presence = MagicMock(return_value=True)
        mod.get_name = MagicMock(return_value="LINE-CARD0")

        p1, p2 = self._patched(tbl)
        with p1, p2:
            with self.assertRaises(ValueError):
                mod._get_module_eeprom_info()

        self.assertFalse(
            os.path.exists(probe_file),
            "Injection payload was executed — ast.literal_eval is not in effect!"
        )


if __name__ == "__main__":
    unittest.main()
