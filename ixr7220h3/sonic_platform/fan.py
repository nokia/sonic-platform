########################################################################
# Nokia IXR7220 H3
#
# Module contains an implementation of SONiC Platform Base API and
# provides the Fans' information which are available in the platform
#
########################################################################


try:
    import os
    import time
    import glob
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

MAX_7220H3_FANS_PER_DRAWER = 2

FORWARD_FAN_F_SPEED = 23000
FORWARD_FAN_R_SPEED = 20500
FORWARD_FAN_TOLERANCE = 50    #8
REVERSE_FAN_F_SPEED = 27000
REVERSE_FAN_R_SPEED = 23000
REVERSE_FAN_TOLERANCE = 50    #10

HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/" 
I2C_DEV_LIST = [("13-002e", "13-002e"),
                ("13-004c", "13-004c"),
                ("13-004c", "13-004c"),
                ("13-004c", "13-002d"),
                ("13-002d", "13-002d"),
                ("13-002d", "13-002d")]
FAN_INDEX_IN_DRAWER = [(1, 2),
                       (1, 2),
                       (3, 4),
                       (5, 1),
                       (2, 3),
                       (4, 5)]
GPIO_DIR = "/sys/class/gpio/gpio{}/" 
GPIO_PORT = [10224, 10225, 10226, 10227, 10228, 10229]
SWPLD1_DIR = "/sys/bus/i2c/devices/17-0032/"

sonic_logger = logger.Logger('fan')


class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, drawer_index, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan
        i2c_dev = I2C_DEV_LIST[drawer_index][fan_index]
        hwmon_path = glob.glob(HWMON_DIR.format(i2c_dev))

        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = drawer_index * MAX_7220H3_FANS_PER_DRAWER + fan_index + 1
            self.fan_drawer = drawer_index
            fan_index_emc230x = FAN_INDEX_IN_DRAWER[drawer_index][fan_index]
            self.set_fan_speed_reg = hwmon_path[0] + "pwm{}".format(fan_index_emc230x)
            self.get_fan_speed_reg = hwmon_path[0] + "fan{}_input".format(fan_index_emc230x)
            self.gpio_dir = GPIO_DIR.format(GPIO_PORT[drawer_index])
           
            # Fan eeprom
            self.eeprom = Eeprom(False, 0, True, drawer_index)

            if self.get_direction() == self.FAN_DIRECTION_EXHAUST:
                if fan_index == 0:
                    self.std_fan_speed = REVERSE_FAN_F_SPEED
                    #self.max_fan_speed = REVERSE_FAN_F_SPEED * (1 + REVERSE_FAN_TOLERANCE / 100)
                    #self.min_fan_speed = REVERSE_FAN_F_SPEED * (1 - REVERSE_FAN_TOLERANCE / 100)
                else:
                    self.std_fan_speed = REVERSE_FAN_R_SPEED
                    #self.max_fan_speed = REVERSE_FAN_R_SPEED * (1 + REVERSE_FAN_TOLERANCE / 100)
                    #self.min_fan_speed = REVERSE_FAN_R_SPEED * (1 - REVERSE_FAN_TOLERANCE / 100)                
            else:
                if fan_index == 0:
                    self.std_fan_speed = FORWARD_FAN_F_SPEED
                    #self.max_fan_speed = FORWARD_FAN_F_SPEED * (1 + FORWARD_FAN_TOLERANCE / 100)
                    #self.min_fan_speed = FORWARD_FAN_F_SPEED * (1 - FORWARD_FAN_TOLERANCE / 100)
                else:
                    self.std_fan_speed = FORWARD_FAN_R_SPEED
                    #self.max_fan_speed = FORWARD_FAN_R_SPEED * (1 + FORWARD_FAN_TOLERANCE / 100)
                    #self.min_fan_speed = FORWARD_FAN_R_SPEED * (1 - FORWARD_FAN_TOLERANCE / 100)

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
        except Exception as e:
            rv = 'ERR'

        # Ensure that the write operation has succeeded
        if (int(self._read_sysfs_file(sysfs_file)) != value ):
            time.sleep(3)
            if (int(self._read_sysfs_file(sysfs_file)) != value ):
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
        result = self._read_sysfs_file(self.gpio_dir + 'value')
        if result == '0':
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

        fan_speed = self.get_speed()
        target_speed = self.get_target_speed()
        tolerance = self.get_speed_tolerance()      
        if (fan_speed != 0 and target_speed != 0 and tolerance != 0):
            if (abs(fan_speed - target_speed) <= tolerance):            
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
        model_name = self.get_part_number()
        if model_name[0:10] == "3HE16436AA":
            return self.FAN_DIRECTION_INTAKE
        elif model_name[0:10] == "3HE16437AA":
            return self.FAN_DIRECTION_EXHAUST
        else:
            return 'N/A'

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
        return False

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

        speed = 100*speed_in_rpm//self.std_fan_speed
        if speed > 100:
            speed = 100

        return speed

    def get_speed_tolerance(self):
        """
        Retrieves the speed tolerance of the fan

        Returns:
            An integer, the percentage of variance from target speed
            which is considered tolerable
        """
        model_name = self.get_part_number()
        self.tolerance = 0
        if model_name[0:10] == "3HE16436AA":
            self.tolerance = FORWARD_FAN_TOLERANCE
        elif model_name[0:10] == "3HE16437AA":
            self.tolerance = REVERSE_FAN_TOLERANCE
        else:
            return 'N/A'        

        return self.tolerance

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

        if speed >= 0 and speed <= 100:
            fandutycycle = round(float(speed)/100*255)        
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

            off , red and green are the only settings 7215 fans
        """
        
        return False

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if not self.get_presence(): 
            return self.STATUS_LED_COLOR_OFF
         
        file_str = SWPLD1_DIR + "fan{}_led".format(self.fan_drawer+1)
        result = self._read_sysfs_file(file_str)
        if result == '1':
            return self.STATUS_LED_COLOR_GREEN
        elif result == '0':
            return self.STATUS_LED_COLOR_OFF
        elif result == '2':
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
            speed = round(float(fan_duty)/255*100)            

        return speed
