from sonic_platform_base.sonic_thermal_control.thermal_action_base import ThermalPolicyActionBase
from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object

from sonic_py_common.logger import Logger
from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2

logger = Logger('thermal_actions')


@thermal_json_object('thermal.platform.publish')
class PublishThermalAlgoAction(ThermalPolicyActionBase):
    """
    Action to publish thermal information to platform
    """

    def execute(self, thermal_info_dict):
        """
        Publish thermal information to platform
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import ChassisInfo
        from .thermal_infos import ThermalInfo
        if ChassisInfo.INFO_NAME in thermal_info_dict:
            # chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
            # chassis = chassis_info_obj.get_chassis()
            pass
        else:
            return

        if ThermalInfo.INFO_NAME in thermal_info_dict:
            thermal_info_obj = thermal_info_dict[ThermalInfo.INFO_NAME]

            for slot, lc_info in thermal_info_obj.lc_thermal_dict.items():
                update_hwslot_temp = platform_ndk_pb2.UpdateTempInfoPb(
                    slot_num=slot, current_temp=int(lc_info['curr_temp']), min_temp=int(lc_info['min_temp']),
                    max_temp=int(lc_info['max_temp']), margin=int(lc_info['margin_temp']))
                channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_THERMAL_SERVICE)
                if not channel or not stub:
                    continue
                ret, response = nokia_common.try_grpc(stub.UpdateThermalHwSlot,
                                                      platform_ndk_pb2.ReqTempParamsPb(hwslot_temp=update_hwslot_temp))
                nokia_common.channel_shutdown(channel)


@thermal_json_object('fan.all.disable_algorithm')
class DisableFanAlgoAction(ThermalPolicyActionBase):
    """
    Action to enable/disable the fan control algorithm
    """
    # JSON field definition
    JSON_FIELD_STATUS = 'status'

    def __init__(self):
        self.status = False

    def load_from_json(self, json_obj):
        """
        Construct DisableFanAlgoAction via JSON. JSON example:
            {
                "type": "fan.all.disable_algorithm"
                "status": "true"
            }
        :param json_obj: A JSON object representing a DisableFanAlgoAction action.
        :return:
        """
        if DisableFanAlgoAction.JSON_FIELD_STATUS in json_obj:
            status_str = json_obj[DisableFanAlgoAction.JSON_FIELD_STATUS].lower()
            if status_str == 'true':
                self.status = True
            elif status_str == 'false':
                self.status = False
            else:
                raise ValueError('Invalid {} field value, please specify true of false'.
                                 format(DisableFanAlgoAction.JSON_FIELD_STATUS))
        else:
            raise ValueError('DisableFanAlgoAction '
                             'missing mandatory field {} in JSON policy file'.
                             format(DisableFanAlgoAction.JSON_FIELD_STATUS))

    def execute(self, thermal_info_dict):
        """
        Disable thermal control algorithm
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import FanInfo
        from .thermal_infos import ChassisInfo
        if ChassisInfo.INFO_NAME in thermal_info_dict:
            chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
            chassis = chassis_info_obj.get_chassis()
        else:
            return

        if FanInfo.INFO_NAME in thermal_info_dict:
            for drawer in chassis.get_all_fan_drawers():
                for fan in drawer.get_all_fans():
                    fan.disable_fan_algorithm(self.status)


@thermal_json_object('fan.all.set_speed')
class SetFanSpeedAction(ThermalPolicyActionBase):
    """
    Base thermal action class to set speed for fans
    """
    # JSON field definition
    JSON_FIELD_SPEED = 'speed'

    def __init__(self):
        """
        Constructor of SetFanSpeedAction
        """
        self.speed = 50

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
            speed = int(json_obj[SetFanSpeedAction.JSON_FIELD_SPEED])
            if speed < 0 or speed > 100:
                raise ValueError('SetFanSpeedAction invalid speed value {} in JSON policy file, '
                                 'valid value should be [0, 100]'.format(speed))
            self.speed = speed
        else:
            raise ValueError('SetFanSpeedAction missing mandatory field {} in JSON policy file'.
                             format(SetFanSpeedAction.JSON_FIELD_SPEED))

    def execute(self, thermal_info_dict):
        """
        Disable thermal control algorithm
        :param thermal_info_dict: A dictionary stores all thermal information.
        :return:
        """
        from .thermal_infos import FanInfo
        from .thermal_infos import ChassisInfo
        if ChassisInfo.INFO_NAME in thermal_info_dict:
            chassis_info_obj = thermal_info_dict[ChassisInfo.INFO_NAME]
            chassis = chassis_info_obj.get_chassis()
        else:
            return

        if FanInfo.INFO_NAME in thermal_info_dict:
            for drawer in chassis.get_all_fan_drawers():
                for fan in drawer.get_all_fans():
                    fan.set_speed(self.speed)
