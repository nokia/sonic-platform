import os
import sys
import types
from unittest.mock import MagicMock

# ---------------------------------------------------------------------------
# Add the chassis package root to sys.path
# ---------------------------------------------------------------------------
_CHASSIS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if _CHASSIS_DIR not in sys.path:
    sys.path.insert(0, _CHASSIS_DIR)

# ---------------------------------------------------------------------------
# Stub heavy dependencies not available outside Nokia hardware environment
# ---------------------------------------------------------------------------

# Base stub class reused across all sonic_platform_base submodules
class _StubBase:
    MODULE_TYPE_LINE = "LINE-CARD"
    MODULE_TYPE_FABRIC = "FABRIC-CARD"
    MODULE_STATUS_EMPTY = "Empty"
    def __init__(self): pass
    def reset(self): pass

# sonic_platform_base — stub all submodules imported transitively
_spb = types.ModuleType("sonic_platform_base")
_spb.__path__ = []
sys.modules["sonic_platform_base"] = _spb

for _sub in ["module_base", "platform_base", "chassis_base", "psu_base",
             "fan_base", "fan_drawer_base", "thermal_base", "sfp_base",
             "component_base", "watchdog_base", "sensor_base"]:
    _m = types.ModuleType(f"sonic_platform_base.{_sub}")
    for _cls in ["ModuleBase", "PlatformBase", "ChassisBase", "PsuBase",
                 "FanBase", "FanDrawerBase", "ThermalBase", "SfpBase",
                 "ComponentBase", "WatchdogBase"]:
        setattr(_m, _cls, _StubBase)
    sys.modules[f"sonic_platform_base.{_sub}"] = _m
    setattr(_spb, _sub, _m)

# platform_ndk
_ndk = types.ModuleType("platform_ndk")
_ndk.__path__ = []
sys.modules["platform_ndk"] = _ndk

_nc = types.ModuleType("platform_ndk.nokia_common")
_nc._get_my_slot = lambda: -1
sys.modules["platform_ndk.nokia_common"] = _nc
_ndk.nokia_common = _nc

_pb2 = types.ModuleType("platform_ndk.platform_ndk_pb2")
class _HwChassisType:
    HW_CHASSIS_TYPE_INVALID = 0
_pb2.HwChassisType = _HwChassisType
sys.modules["platform_ndk.platform_ndk_pb2"] = _pb2
_ndk.platform_ndk_pb2 = _pb2

# sonic_py_common
_spc = types.ModuleType("sonic_py_common")
_spc.__path__ = []
sys.modules["sonic_py_common"] = _spc

_db_mod = types.ModuleType("sonic_py_common.daemon_base")
_db_mod.db_connect = MagicMock()
sys.modules["sonic_py_common.daemon_base"] = _db_mod
_spc.daemon_base = _db_mod

_di_mod = types.ModuleType("sonic_py_common.device_info")
sys.modules["sonic_py_common.device_info"] = _di_mod
_spc.device_info = _di_mod

_lg_mod = types.ModuleType("sonic_py_common.logger")
class _Logger:
    def log_info(self, *a, **kw): pass
    def log_error(self, *a, **kw): pass
    def log_warning(self, *a, **kw): pass
_lg_mod.Logger = _Logger
sys.modules["sonic_py_common.logger"] = _lg_mod
_spc.logger = _lg_mod

# swsscommon
_swss_pkg = types.ModuleType("swsscommon")
_swss_pkg.__path__ = []
sys.modules["swsscommon"] = _swss_pkg

_swss_inner = types.ModuleType("swsscommon.swsscommon")
class _Table:
    def __init__(self, db, tbl): pass
    def get(self, key): return False, []
_swss_inner.Table = _Table
_swss_inner.FieldValuePairs = list
sys.modules["swsscommon.swsscommon"] = _swss_inner
_swss_pkg.swsscommon = _swss_inner

# sonic_platform.eeprom (stub — requires real Nokia gRPC)
_sp_eeprom = types.ModuleType("sonic_platform.eeprom")
_sp_eeprom.Eeprom = MagicMock()
sys.modules["sonic_platform.eeprom"] = _sp_eeprom
