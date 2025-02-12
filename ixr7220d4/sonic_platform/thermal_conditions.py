try:
    from sonic_platform_base.sonic_thermal_control.thermal_condition_base import ThermalPolicyConditionBase
    from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

class FanCondition(ThermalPolicyConditionBase):
    def get_fan_info(self, thermal_info_dict):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            return thermal_info_dict[FanInfo.INFO_NAME]
        return None


@thermal_json_object('fan.any.absence')
class AnyFanAbsenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        return len(fan_info_obj.get_absence_fans()) > 0 if fan_info_obj else False


@thermal_json_object('fan.f2b.all.presence')
class AllFanPresenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        if fan_info_obj and fan_info_obj.get_fan_type() == "intake":
            return len(fan_info_obj.get_absence_fans()) == 0
        else:
            return False

@thermal_json_object('fan.b2f.all.presence')
class AllFanPresenceCondition(FanCondition):
    def is_match(self, thermal_info_dict):
        fan_info_obj = self.get_fan_info(thermal_info_dict)
        if fan_info_obj and fan_info_obj.get_fan_type() == "exhaust":
            return len(fan_info_obj.get_absence_fans()) == 0
        else:
            return False


class ThermalCondition(ThermalPolicyConditionBase):
    def get_thermal_info(self, thermal_info_dict):
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):
            return thermal_info_dict[ThermalInfo.INFO_NAME]
        return None


@thermal_json_object('thermal.over.high_critical_threshold')
class ThermalOverHighCriticalCondition(ThermalCondition):
    def is_match(self, thermal_info_dict):
        thermal_info_obj = self.get_thermal_info(thermal_info_dict)
        if thermal_info_obj:
            return thermal_info_obj.is_over_high_critical_threshold()
        return False

