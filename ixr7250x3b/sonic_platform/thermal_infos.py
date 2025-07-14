try:
    from sonic_platform_base.sonic_thermal_control.thermal_info_base import ThermalPolicyInfoBase
    from sonic_platform_base.sonic_thermal_control.thermal_json_object import thermal_json_object
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

sonic_logger = logger.Logger('thermal_info')
sonic_logger.set_min_log_priority_warning()

FM = 3
TI = 5

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

    def collect(self, chassis):
        """
        Collect absence and presence fans.
        :param chassis: The chassis object
        :return:
        """
        self._status_changed = False
        for fandrawer in chassis.get_all_fan_drawers():
            fandrawer_pres = fandrawer.get_presence()
            if fandrawer_pres and fandrawer not in self._presence_fans:
                self._presence_fans.add(fandrawer)
                self._status_changed = True
                if fandrawer in self._absence_fans:
                    self._absence_fans.remove(fandrawer)
            elif not fandrawer_pres and fandrawer not in self._absence_fans:
                self._absence_fans.add(fandrawer)
                self._status_changed = True
                if fandrawer in self._presence_fans:
                    self._presence_fans.remove(fandrawer)

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
        self._num_fan_levels = 2
        self.p = 0
        self.se = 0
        self.ps = 1
        self.mc = 100
        self.nc = 38
        self.last_drawer_event = False
        self._fan_drawer = 0

    def collect(self, chassis):
        """
        Collect thermal sensor temperature change status
        :param chassis: The chassis object
        :return:
        """
        self._temps = []
        self._t = []
        self._fan_speeds = []
        self._over_high_critical_threshold = False
        self._set_fan_default_speed = False
        self._set_fan_ctl_speed = False
        self._set_fan_high_temp_speed = False
        
        num_of_thermals = chassis.get_num_thermals()
        for index in range(num_of_thermals):
            temp_current = chassis.get_thermal(index).get_temperature()    
            self._temps.insert(index, temp_current)
            self._t.insert(index, chassis.get_thermal(index).get_high_threshold() - temp_current)
            
        t_min = min(self._t)

        self.good_fan = 0
        fan_speed_sum = 0
        self.fan_drawer = 0
        for fandrawer in chassis.get_all_fan_drawers():
            if fandrawer.get_presence():
                self.fan_drawer += 1
                for fan in fandrawer.get_all_fans():
                    fan_speed = fan.get_speed()
                    if (fan_speed >= (self.nc - 5)) or (fan_speed <= self.mc):
                        fan_speed_sum += fan_speed
                        self.good_fan += 1
        
        avg_fan_speed = round(fan_speed_sum / self.good_fan)

        diff = self.fc(t_min-FM, avg_fan_speed)
        if (avg_fan_speed - diff <= self.nc):
            self.fanctl_speed = self.nc
        elif (avg_fan_speed - diff >= self.mc):
            self.fanctl_speed = self.mc
        else:
            self.fanctl_speed = avg_fan_speed - diff

        self._set_fan_ctl_speed = True

        if self._fan_drawer == 2 and self.fan_drawer == 3:
            self.last_drawer_insert = True
        else:
            self.last_drawer_insert = False
        self._fan_drawer = self.fan_drawer

    def is_set_fan_default_speed(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return self._set_fan_default_speed

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

    def is_set_fan_ctl_speed(self):
        """
        Retrieves if the temperature is warm up and over high threshold
        :return: True if the temperature is warm up and over high threshold else False
        """
        return self._set_fan_ctl_speed
    
    def fc(self, t, f):
        m = (t - self.p)/TI

        if (t == 0 or (self.p > 0 and t < 0) or (self.p < 0 and t > 0)):
            self.se = 0
            self.ps = 1
        
        if (t < -1):
            a = 0.4 * -1 * t
        else:
            a = 0.4

        if ((f > self.nc) and (f < self.mc)):
            self.se += t
            self.ps += 1
        
        result = (a * t) + (0.002 * self.se)/self.ps + (10 * m)
 
        self.p = t
        return round(result)
    
    def get_fanctl_speed(self):
        if (self.fanctl_speed >= self.nc or self.fanctl_speed <= self.mc):
            return self.fanctl_speed
        else:
            return 0
        
    def get_last_drawer_event(self):
        return self.last_drawer_insert

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
