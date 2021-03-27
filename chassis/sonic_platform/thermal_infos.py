from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
from sonic_py_common import daemon_base
from sonic_py_common.logger import Logger
from swsscommon import swsscommon
from sonic_platform_base.module_base import ModuleBase

logger = Logger('thermal_infos')


@thermal_json_object('fan_info')
class FanInfo(ThermalPolicyInfoBase):
    """
    FanOverride information needed by thermal policy
    """
    INFO_NAME = 'fan_info'

    def __init__(self):
        self._chassis = None
        self._fan_platform_override = False

    def collect(self, chassis):
        """
        Collect platform chassis.
        :param chassis: The chassis object
        :return:
        """
        self._chassis = chassis

    def get_fanalgo_override(self):
        """
        Retrieves platform fancontrol algorithm override option
        :return: A boolean.
        """
        return self._chassis.allow_fan_platform_override()


@thermal_json_object('thermal_info')
class ThermalInfo(ThermalPolicyInfoBase):
    """
    Thermal information needed by thermal policy
    """

    # Thermal information name
    INFO_NAME = 'thermal_info'

    NOKIA_J2_TEMP_THRESHOLD = 105.0
    NOKIA_IMM_TEMP_THRESHOLD = 60.0
    NOKIA_INVALID_TEMP = -128.0
    NOKIA_MAX_TEMP = 127.0
    NOKIA_MIN_TEMP = -128.0

    def __init__(self):
        self.init = False
        self.lc_thermal_dict = {}

    def init_extreme(self):
        self.curr_temp = self.NOKIA_INVALID_TEMP
        self.min_temp = self.NOKIA_MAX_TEMP
        self.max_temp = self.NOKIA_MIN_TEMP
        self.margin_temp = self.NOKIA_MAX_TEMP

    def updateminmax(self, temp):
        if temp == self.NOKIA_INVALID_TEMP:
            return
        if temp > self.max_temp:
            self.max_temp = temp
        if (temp < self.min_temp) or (self.min_temp == self.NOKIA_INVALID_TEMP):
            self.min_temp = temp

    def collect(self, chassis):
        """
        Collect thermal sensor temperature change status
        :param chassis: The chassis object
        :return:
        """
        num_modules = chassis.get_num_modules()

        if chassis.get_supervisor_slot() != chassis.get_my_slot():
            return

        chassis_state_db = daemon_base.db_connect("CHASSIS_STATE_DB")
        for module_index in range(1, num_modules + 1):
            # if chassis.get_module(module_index - 1).get_status() is not 'Online':
            #    continue
            if chassis.get_module(module_index - 1).get_type() != ModuleBase.MODULE_TYPE_LINE:
                continue

            slot = chassis.get_module(module_index - 1).get_slot()
            table_name = 'TEMPERATURE_INFO_'+str(slot)
            lc_thermal_tbl = swsscommon.Table(chassis_state_db, table_name)

            # Find min, max, curr, margin temp for each module
            self.init_extreme()

            # Walk the table and store in the dictionary
            keys = lc_thermal_tbl.getKeys()
            if not keys:
                continue

            for key in keys:
                status, fvs = lc_thermal_tbl.get(key)
                fv_dict = dict(fvs)

                # Skip temperature sensors not valid for FAN reading.
                # Can convert this check into a RPC API
                if (float(fv_dict['minimum_temperature']) == self.NOKIA_INVALID_TEMP) and (
                        float(fv_dict['maximum_temperature']) == self.NOKIA_INVALID_TEMP):
                    continue

                # For J2 temperatures, there is remote normalization
                normalize = 0.0
                if (float(fv_dict['high_threshold']) == self.NOKIA_J2_TEMP_THRESHOLD):
                    normalize = self.NOKIA_J2_TEMP_THRESHOLD - self.NOKIA_IMM_TEMP_THRESHOLD

                self.curr_temp = max(float(self.curr_temp), float(fv_dict['temperature']) - normalize)
                self.updateminmax(float(fv_dict['minimum_temperature']) - normalize)
                self.updateminmax(float(fv_dict['maximum_temperature']) - normalize)
                margin = float(fv_dict['high_threshold']) - float(fv_dict['temperature'])
                self.margin_temp = min(float(self.margin_temp), margin)

            slot_avg_dict = {}
            slot_avg_dict['curr_temp'] = int(self.curr_temp)
            slot_avg_dict['min_temp'] = int(self.min_temp)
            slot_avg_dict['max_temp'] = int(self.max_temp)
            slot_avg_dict['margin_temp'] = int(self.margin_temp)

            self.lc_thermal_dict[slot] = slot_avg_dict


@thermal_json_object('chassis_info')
class ChassisInfo(ThermalPolicyInfoBase):
    """
    Chassis information needed by thermal policy
    """
    INFO_NAME = 'chassis_info'

    def __init__(self):
        self._chassis = None

    def collect(self, chassis):
        """
        Collect platform chassis.
        :param chassis: The chassis object
        :return:
        """
        self._chassis = chassis

    def get_chassis(self):
        """
        Retrieves platform chassis object
        :return: A platform chassis object.
        """
        return self._chassis
