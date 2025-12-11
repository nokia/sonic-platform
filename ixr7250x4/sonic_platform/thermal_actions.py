try:
    from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
    from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

sonic_logger = logger.Logger('thermal_actions')
sonic_logger.set_min_log_priority_warning()

SPEED_FOR_HOT_START = 40

class SetFanSpeedAction(ThermalPolicyActionBase):
    """
    Base thermal action class to set speed for fans
    """
    # JSON field definition
    JSON_FIELD_SPEED = 'speed'
    JSON_FIELD_DEFAULT_SPEED = 'default_speed'
    JSON_FIELD_HIGHTEMP_SPEED = 'hightemp_speed'

    def __init__(self):
        """
        Constructor of SetFanSpeedAction
        """
        self.default_speed = 60
        self.hightemp_speed = 100
        self.speed = self.default_speed

    def load_from_json(self, json_obj):
        """
        Construct SetFanSpeedAction via JSON. JSON example:
            {
                "type": "fan.all.set_speed"
                "speed": "100"
            }
        :param json_obj: A JSON object representing a SetFanSpeedAction action.
        :return:
        """
        if SetFanSpeedAction.JSON_FIELD_SPEED in json_obj:
            speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_SPEED])
            if speed < 0 or speed > 100:
                raise ValueError('SetFanSpeedAction invalid speed value {} in JSON policy file, valid value should be [0, 100]'.
                                 format(speed))
            self.speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_SPEED])
        else:
            raise ValueError('SetFanSpeedAction missing mandatory field {} in JSON policy file'.
                             format(SetFanSpeedAction.JSON_FIELD_SPEED))

    @classmethod
    def set_all_fan_speed(cls, thermal_info_dict, speed):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fandrawer in fan_info_obj.get_presence_fans():
                for fan in fandrawer.get_all_fans():
                    fan.set_speed(int(speed))

    @classmethod
    def set_fan_speed_hot_start(cls, thermal_info_dict, last_drawer):
        from .thermal_infos import FanInfo
        if FanInfo.INFO_NAME in thermal_info_dict and isinstance(thermal_info_dict[FanInfo.INFO_NAME], FanInfo):
            fan_info_obj = thermal_info_dict[FanInfo.INFO_NAME]
            for fandrawer in fan_info_obj.get_presence_fans():
                if fandrawer.get_index() == last_drawer:
                    for fan in fandrawer.get_all_fans():
                        fan.set_speed(100)
                else:
                    for fan in fandrawer.get_all_fans():
                        fan.set_speed(int(SPEED_FOR_HOT_START))

@thermal_json_object('fan.all.set_speed')
class SetAllFanSpeedAction(SetFanSpeedAction):
    """
    Action to set speed for all fans
    """
    def execute(self, thermal_info_dict):
        """
        Set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        SetAllFanSpeedAction.set_all_fan_speed(thermal_info_dict, self.speed)

@thermal_json_object('thermal.temp_check_and_set_all_fan_speed')
class ThermalRecoverAction(SetFanSpeedAction):
    """
    Action to check thermal sensor temperature change status and set speed for all fans
    """
    def load_from_json(self, json_obj):
        """
        Construct ThermalRecoverAction via JSON. JSON example:
            {
                "type": "thermal.temp_check_and_set_all_fan_speed"
                "default_speed": "25",
                "threshold1_speed": "40",
                "threshold2_speed": "75",
                "hightemp_speed": "100"
            }
        :param json_obj: A JSON object representing a ThermalRecoverAction action.
        :return:
        """
        if SetFanSpeedAction.JSON_FIELD_DEFAULT_SPEED in json_obj:
            default_speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_DEFAULT_SPEED])
            if default_speed < 0 or default_speed > 100:
                raise ValueError('SetFanSpeedAction invalid default speed value {} in JSON policy file, valid value should be [0, 100]'.
                                 format(default_speed))
            self.default_speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_DEFAULT_SPEED])
        else:
            raise ValueError('SetFanSpeedAction missing mandatory field {} in JSON policy file'.
                             format(SetFanSpeedAction.JSON_FIELD_DEFAULT_SPEED))

        if SetFanSpeedAction.JSON_FIELD_HIGHTEMP_SPEED in json_obj:
            hightemp_speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_HIGHTEMP_SPEED])
            if hightemp_speed < 0 or hightemp_speed > 100:
                raise ValueError('SetFanSpeedAction invalid hightemp speed value {} in JSON policy file, valid value should be [0, 100]'.
                                 format(hightemp_speed))
            self.hightemp_speed = float(json_obj[SetFanSpeedAction.JSON_FIELD_HIGHTEMP_SPEED])
        else:
            raise ValueError('SetFanSpeedAction missing mandatory field {} in JSON policy file'.
                             format(SetFanSpeedAction.JSON_FIELD_HIGHTEMP_SPEED))

        sonic_logger.log_warning("ThermalRecoverAction: default: {}, hightemp: {}".format(self.default_speed, self.hightemp_speed))

    def execute(self, thermal_info_dict):
        """
        Check check thermal sensor temperature change status and set speed for all fans
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import ThermalInfo
        if ThermalInfo.INFO_NAME in thermal_info_dict and \
           isinstance(thermal_info_dict[ThermalInfo.INFO_NAME], ThermalInfo):
            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]
            if not thermal_info_obj.is_skip_fan_ctl():
                fanctl_speed = thermal_info_obj.get_fanctl_speed()
                last_drawer_index = thermal_info_obj.get_last_drawer_event()
                if last_drawer_index != 0:
                    sonic_logger.log_warning(f"Last fan drawer has been inserted, executing hot start sequence.")
                    ThermalRecoverAction.set_fan_speed_hot_start(thermal_info_dict, last_drawer_index)
                else:
                    if fanctl_speed != 0:
                        ThermalRecoverAction.set_all_fan_speed(thermal_info_dict, fanctl_speed)
                    else:
                        ThermalRecoverAction.set_all_fan_speed(thermal_info_dict, self.default_speed)
                        sonic_logger.log_warning(f"Wrong fanctl_speed, use default fan speed {self.default_speed}")
            else:
                sonic_logger.log_warning(f"Skip fan speed control for hot start sequence.")

@thermal_json_object('switch.shutdown')
class SwitchPolicyAction(ThermalPolicyActionBase):
    """
    Base class for thermal action. Once all thermal conditions in a thermal policy are matched,
    all predefined thermal action will be executed.
    """
    def execute(self, thermal_info_dict):
        """
        Take action when thermal condition matches. For example, adjust speed of fan or shut
        down the switch.
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        try:
            import os
            from sonic_platform.chassis import Chassis
            for i in range(3):
                fan_obj = Chassis().get_fan_drawer(i)
                if fan_obj.get_presence():
                    sonic_logger.log_warning(f"Fan {fan_obj.get_name()} speed: "
                                             f"{fan_obj.get_fan(0).get_speed()}%, {fan_obj.get_fan(1).get_speed()}%, "
                                             f"{fan_obj.get_fan(2).get_speed()}%, {fan_obj.get_fan(3).get_speed()}%.")
                else:
                    sonic_logger.log_warning(f"Fan {fan_obj.get_name()} not presence.")
            for i in range(2):
                psu_obj = Chassis().get_psu(i)
                if psu_obj.get_presence():
                    sonic_logger.log_warning(f"{psu_obj.get_name()}: {psu_obj.get_voltage()}V, "
                                             f"{psu_obj.get_current()}A, {psu_obj.get_power()}W.")
                else:
                    sonic_logger.log_warning(f"{fan_obj.get_name()} not presence.")
        except Exception as e:
            sonic_logger.log_warning(" Fail to save fan and psu info {}".format(repr(e)))
        
        sonic_logger.log_error("Alarm for temperature critical is detected, reboot Device")
        os.system('sync')
        os.system('sync')
        os.system('echo b > /proc/sysrq-trigger')
        
@thermal_json_object('thermal_control.control')
class ControlThermalAlgoAction(ThermalPolicyActionBase):
    """
    Action to control the thermal control algorithm
    """
    # JSON field definition
    JSON_FIELD_STATUS = 'status'

    def __init__(self):
        self.status = True

    def load_from_json(self, json_obj):
        """
        Construct ControlThermalAlgoAction via JSON. JSON example:
            {
                "type": "thermal_control.control"
                "status": "true"
            }
        :param json_obj: A JSON object representing a ControlThermalAlgoAction action.
        :return:
        """
        if ControlThermalAlgoAction.JSON_FIELD_STATUS in json_obj:
            status_str = json_obj[ControlThermalAlgoAction.JSON_FIELD_STATUS].lower()
            if status_str == 'true':
                self.status = True
            elif status_str == 'false':
                self.status = False
            else:
                raise ValueError('Invalid {} field value, please specify true of false'.
                                 format(ControlThermalAlgoAction.JSON_FIELD_STATUS))
        else:
            raise ValueError('ControlThermalAlgoAction '
                             'missing mandatory field {} in JSON policy file'.
                             format(ControlThermalAlgoAction.JSON_FIELD_STATUS))

    def execute(self, thermal_info_dict):
        """
        Disable thermal control algorithm
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import ChassisInfo
        if ChassisInfo.INFO_NAME in thermal_info_dict:
            chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
            chassis = chassis_info_obj.get_chassis()
            thermal_manager = chassis.get_thermal_manager()
            if self.status:
                thermal_manager.start_thermal_control_algorithm()
            else:
                thermal_manager.stop_thermal_control_algorithm()
