########################################################################
# Nokia IXR7220 H4-64D
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform
#
########################################################################


try:
    import os
    import time
    import glob
    import struct    
    from mmap import *
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FANS_PER_DRAWER = 2

FAN_PN_F2B = "3HE19865AARA01"
MAX_SPEED = 13600
FAN_TOLERANCE = 20
WORKING_ixr7220_FAN_SPEED = 1360

I2C_DIR = "/sys/bus/i2c/devices/i2c-25/25-0033/" 

sonic_logger = logger.Logger('fan')


class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, drawer_index, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan

        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = drawer_index * FANS_PER_DRAWER + fan_index + 1
            self.drawer_index = drawer_index + 1
            self.set_fan_speed_reg = I2C_DIR + "pwm{}".format(self.index)
            self.get_fan_speed_reg = I2C_DIR + "fan{}_speed".format(self.index)
            self.fan_presence_reg = I2C_DIR + "fan{}_present".format(self.drawer_index)
            self.fan_led_reg = I2C_DIR + "fan{}_led".format(self.drawer_index)
            self.system_led_supported_color = ['off', 'red', 'green']
           
            # Fan eeprom
            self.eeprom = Eeprom(False, 0, True, drawer_index)
            self.max_fan_speed = MAX_SPEED                    
                  
        else:
            # this is a PSU Fan
            self.index = fan_index
            self.dependency = dependency

    def _read_sysfs_file(self, sysfs_file):
        # On successful read, returns the value read from given
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'r') as fd:
                rv = fd.read()
                fd.close()
        except Exception as e:
            rv = 'ERR'

        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
        return rv

    def _write_sysfs_file(self, sysfs_file, value):
        # On successful write, the value read will be written on
        # reg_name and on failure returns 'ERR'
        rv = 'ERR'

        if (not os.path.isfile(sysfs_file)):
            return rv
        try:
            with open(sysfs_file, 'w') as fd:
                rv = fd.write(value)
                fd.close()
        except Exception as e:
            rv = 'ERR'

        return rv
    
    def get_name(self):
        """
        Retrieves the name of the Fan

        Returns:
            string: The name of the Fan
        """
        if not self.is_psu_fan:
            return "Fan{}".format(self.index)
        else:
            return "PSU{} Fan".format(self.index)

    def get_presence(self):
        """
        Retrieves the presence of the Fan Unit

        Returns:
            bool: True if Fan is present, False if not
        """
        result = self._read_sysfs_file(self.fan_presence_reg)
        if result == '1':
            return True
        else:
            return False

    def get_model(self):
        """
        Retrieves the model number of the Fan

        Returns:
            string: Model number of Fan. Use part number for this.
        """
        return self.eeprom.modelstr()

    def get_serial(self):
        """
        Retrieves the serial number of the Fan

        Returns:
            string: Serial number of Fan
        """
        return self.eeprom.serial_number_str()

    def get_part_number(self):
        """
        Retrieves the part number of the Fan

        Returns:
            string: Part number of Fan
        """
        return self.eeprom.part_number_str()

    def get_service_tag(self):
        """
        Retrieves the service tag of the Fan

        Returns:
            string: Service Tag of Fan
        """
        return self.eeprom.service_tag_str()

    def get_status(self):
        """
        Retrieves the operational status of the Fan

        Returns:
            bool: True if Fan is operating properly, False if not
        """
        status = False

        fan_speed = self._read_sysfs_file(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            if (int(fan_speed) > WORKING_ixr7220_FAN_SPEED):
                status = True

        return status

    def get_direction(self):
        """
        Retrieves the fan airflow direction
        Possible fan directions (relative to port-side of device)
        Returns:
            A string, either FAN_DIRECTION_INTAKE or
            FAN_DIRECTION_EXHAUST depending on fan direction
        """
        
        return self.FAN_DIRECTION_INTAKE
        

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device
        Returns:
            integer: The 1-based relative physical position in parent device
        """
        return self.index

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def get_speed(self):
        """
        Retrieves the speed of a Front FAN in the tray in revolutions per
                 minute defined by 1-based index
        :param index: An integer, 1-based index of the FAN to query speed
        :return: integer, denoting front FAN speed
        """
        speed = 0

        fan_speed = self._read_sysfs_file(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            speed_in_rpm = int(fan_speed)
        else:
            speed_in_rpm = 0

        speed = 100*speed_in_rpm//self.max_fan_speed
        if speed > 100:
            speed = 100

        return int(speed)

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed
            which is considered tolerable
        """
         
        return FAN_TOLERANCE

    def set_speed(self, speed):
        """
        Set fan speed to expected value
        Args:
            speed: An integer, the percentage of full fan speed to set
            fan to, in the range 0 (off) to 100 (full speed)
        Returns:
            bool: True if set success, False if fail.
        """
        if self.is_psu_fan:
            return False

        if speed in range(0, 10):
            fandutycycle = 0x00
        elif speed in range(10, 100):
            fandutycycle = (speed*255)//100 
        else:
            return False

        rv = self._write_sysfs_file(self.set_fan_speed_reg, str(fandutycycle))
        if (rv != 'ERR'):
            return True
        else:
            return False

    def set_status_led(self, color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if set success, False if fail.

            No indivial led for each H4-64D fans
        """        
        return False

    def get_status_led(self):
        """
        Gets the state of the fan drawer status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if not self.get_presence(): 
            return self.STATUS_LED_COLOR_OFF        

        result = self._read_sysfs_file(self.fan_led_reg)

        if result == "base" or result == "off":
            return self.STATUS_LED_COLOR_OFF
        elif result == "green":
            return self.STATUS_LED_COLOR_GREEN
        elif result == "red":
            return self.STATUS_LED_COLOR_RED
        else:
            return 'N/A'

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0
            (off) to 100 (full speed)
        """
        speed = 0

        fan_duty = self._read_sysfs_file(self.set_fan_speed_reg)
        if (fan_duty != 'ERR'):
            dutyspeed = int(fan_duty)
            if dutyspeed == 0:
                speed = 0
            else:
                speed = (dutyspeed*100)//255

        return int(speed)
