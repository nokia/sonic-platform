try:
    from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
    from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
    from sonic_py_common.logger import Logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

logger = Logger()

@thermal_json_object('fan_info')
class FanInfo(ThermalPolicyInfoBase):
    """
    Fan information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'fan_info'

    def __init__(self):
        self._absence_fans = set()
        self._presence_fans = set()
        self._status_changed = False
        self._fan_type = " "

    def collect(self, chassis):
        """
        Collect absence and presence fans.
        :param chassis: The chassis object
        :return:
        """
        self._status_changed = False
        for fan in chassis.get_all_fans():
            if fan.get_presence() and fan not in self._presence_fans:
                self._presence_fans.add(fan)
                self._status_changed = True
                if fan in self._absence_fans:
                    self._absence_fans.remove(fan)
            elif not fan.get_presence() and fan not in self._absence_fans:
                self._absence_fans.add(fan)
                self._status_changed = True
                if fan in self._presence_fans:
                    self._presence_fans.remove(fan)

        self._fan_type = chassis.get_chassis_fan_type()

    def get_absence_fans(self):
        """
        Retrieves absence fans
        :return: A set of absence fans
        """
        return self._absence_fans

    def get_presence_fans(self):
        """
        Retrieves presence fans
        :return: A set of presence fans
        """
        return self._presence_fans

    def is_status_changed(self):
        """
        Retrieves if the status of fan information changed
        :return: True if status changed else False
        """
        return self._status_changed

    def get_fan_type(self):
        """
        Retrieves the fan type (B2F, F2B) that currently is loaded into the chassis.
        All fans should be either B2F or F2B.

        Returns:
            Airflow direction: FAN_DIRECTION_INTAKE for F2B,
                               FAN_DIRECTION_EXHAUST for B2F,
                               o/w " ".
        """
        return self._fan_type

@thermal_json_object('thermal_info')
class ThermalInfo(ThermalPolicyInfoBase):
    """
    Thermal information needed by thermal policy
    """

    # Fan information name
    INFO_NAME = 'thermal_info'

    def __init__(self):
        self._old_threshold_level = -1
        self._current_threshold_level = 0
        self._num_fan_levels = 3

    def collect(self, chassis):
        """
        Collect thermal sensor temperature change status
        :param chassis: The chassis object
        :return:
        """
        self._temps = []
        self._over_high_critical_threshold = False
        self._set_fan_default_speed = False
        self._set_fan_threshold_one_speed = False
        self._set_fan_threshold_two_speed = False
        self._set_fan_high_temp_speed = False
        self.__set_level_up_down_thresholds(chassis)

        # Calculate average temp within the device
        num_of_thermals = chassis.get_num_thermals()
        for thermal_id in range(num_of_thermals):
            self._temps.insert(thermal_id, chassis.get_thermal(thermal_id).get_temperature())


       # Find current required threshold level
        max_level = 0
        min_level = [self._num_fan_levels for i in range(num_of_thermals)]

        for thermal_id in range(num_of_thermals):
            for level in range(self._num_fan_levels):

                if self._temps[thermal_id]>self._level_up_threshold[level][thermal_id]:
                    if max_level<level+1:
                        max_level=level+1
                if self._temps[thermal_id]<self._level_down_threshold[level][thermal_id]:
                    if min_level[thermal_id]>level:
                        min_level[thermal_id]=level

        max_of_min_level=max(min_level)

        #compare with running threshold level
        if max_of_min_level > self._old_threshold_level:
            max_of_min_level=self._old_threshold_level

        self._current_threshold_level = max(max_of_min_level,max_level)

        #set fan to max speed if one fan is down
        for fan in chassis.get_all_fans():
            if not fan.get_status():
                self._current_threshold_level = 3

       # Decide fan speed based on threshold level

        if self._current_threshold_level != self._old_threshold_level:
            if self._current_threshold_level == 0:
                self._set_fan_default_speed = True
            elif self._current_threshold_level == 1:
                self._set_fan_threshold_one_speed = True
            elif self._current_threshold_level == 2:
                self._set_fan_threshold_two_speed = True
            elif self._current_threshold_level == 3:
                self._set_fan_high_temp_speed = True

        self._old_threshold_level=self._current_threshold_level

    def __set_level_up_down_thresholds(self, chassis):
        fan_type = chassis.get_chassis_fan_type()
        match fan_type:
            case "intake":
                self._level_up_threshold = [[38,38,38,40,38,84,61],
                                            [49,51,48,48,50,95,74],
                                            [52,54,52,53,54,98,78]]
                self._level_down_threshold = [[33,33,33,34,33,73,53],
                                              [45,46,45,44,44,88,72],
                                              [49,51,48,48,50,96,74]]
            case "exhaust":
                self._level_up_threshold = [[43,33,38,33,27,55,62],
                                            [58,45,51,45,36,71,79],
                                            [61,49,55,50,45,77,83]]
                self._level_down_threshold = [[35,24,30,26,22,48,52],
                                              [55,42,49,41,33,67,72],
                                              [58,45,51,47,39,74,79]]
            case _:
                self._level_up_threshold = []
                self._level_down_threshold = []

    def is_set_fan_default_speed(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return self._set_fan_default_speed

    def is_set_fan_threshold_one_speed(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return self._set_fan_threshold_one_speed

    def is_set_fan_threshold_two_speed(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return self._set_fan_threshold_two_speed

    def is_set_fan_high_temp_speed(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return self._set_fan_high_temp_speed

    def is_over_high_critical_threshold(self):
        """
        Retrieves if the temperature is over high critical threshold
        :return: True if the temperature is over high critical threshold else False
        """
        return self._over_high_critical_threshold


@thermal_json_object('psu_info')
class PsuInfo(ThermalPolicyInfoBase):
    """
    PSU information needed by thermal policy
    """
    INFO_NAME = 'psu_info'

    def __init__(self):
        self._absence_psus = set()
        self._presence_psus = set()
        self._status_changed = False

    def collect(self, chassis):
        """
        Collect absence and presence PSUs.
        :param chassis: The chassis object
        :return:
        """
        self._status_changed = False
        for psu in chassis.get_all_psus():
            if psu.get_presence() and psu.get_powergood_status() and psu not in self._presence_psus:
                self._presence_psus.add(psu)
                self._status_changed = True
                if psu in self._absence_psus:
                    self._absence_psus.remove(psu)
            elif (not psu.get_presence() or not psu.get_powergood_status()) and psu not in self._absence_psus:
                self._absence_psus.add(psu)
                self._status_changed = True
                if psu in self._presence_psus:
                    self._presence_psus.remove(psu)

    def get_absence_psus(self):
        """
        Retrieves presence PSUs
        :return: A set of absence PSUs
        """
        return self._absence_psus

    def get_presence_psus(self):
        """
        Retrieves presence PSUs
        :return: A set of presence fans
        """
        return self._presence_psus

    def is_status_changed(self):
        """
        Retrieves if the status of PSU information changed
        :return: True if status changed else False
        """
        return self._status_changed


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
