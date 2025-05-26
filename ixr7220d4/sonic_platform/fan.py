"""
    Nokia IXR7220 D4

    Module contains an implementation of SONiC Platform Base API and
    provides the Fans' information which are available in the platform
"""

try:
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

FAN_PN_F2B = "3HE17744AA"
FAN_PN_B2F = "3HE17745AA"
FANS_PER_DRAWER = 2
MAX_FAN_F2B_F_SPEED = 30900/FANS_PER_DRAWER
MAX_FAN_F2B_R_SPEED = 24900/FANS_PER_DRAWER
MAX_FAN_B2F_F_SPEED = 31000/FANS_PER_DRAWER
MAX_FAN_B2F_R_SPEED = 25000/FANS_PER_DRAWER
WORKING_IXR7220_F_FAN_SPEED = 3000/FANS_PER_DRAWER
WORKING_IXR7220_R_FAN_SPEED = 2400/FANS_PER_DRAWER
FAN_TOLERANCE   = 10


I2C_DIR = "/sys/bus/i2c/devices/i2c-14/14-0066/"

sonic_logger = logger.Logger('fan')

class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, drawer_index, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan

        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = drawer_index * FANS_PER_DRAWER + fan_index + 1
            self.drawer_index = drawer_index + 1
            self.get_fan_direction = I2C_DIR + f"fan{self.drawer_index}_direction"
            self.set_fan_speed_reg = I2C_DIR + "fans_pwm"
            self.get_fan_speed_reg = I2C_DIR + f"fan{self.index}_speed"
            self.fan_presence_reg = I2C_DIR + f"fan{self.drawer_index}_present"
            self.fan_led_reg = I2C_DIR + f"fan{self.drawer_index}_led"

            if fan_index == 0:
                    self.working_fan_speed = WORKING_IXR7220_F_FAN_SPEED
                    if self.get_direction() == self.FAN_DIRECTION_INTAKE:
                        self.max_fan_speed = MAX_FAN_F2B_F_SPEED
                    else:
                        self.max_fan_speed = MAX_FAN_B2F_F_SPEED
            else:
                    self.working_fan_speed = WORKING_IXR7220_R_FAN_SPEED
                    if self.get_direction() == self.FAN_DIRECTION_INTAKE:
                        self.max_fan_speed = MAX_FAN_F2B_R_SPEED
                    else:
                        self.max_fan_speed = MAX_FAN_B2F_R_SPEED
        else:
            # this is a PSU Fan
            self.index = fan_index
            self.dependency = dependency

    def get_name(self):
        """
        Retrieves the name of the Fan

        Returns:
            string: The name of the Fan
        """
        if not self.is_psu_fan:
            return f"Fan{self.index}"
        return f"PSU{self.index} Fan"

    def get_presence(self):
        """
        Retrieves the presence of the Fan Unit

        Returns:
            bool: True if Fan is present, False if not
        """
        result = read_sysfs_file(self.fan_presence_reg)
        if result == '1':
            return True
        return False

    def get_model(self):
        """
        Retrieves the model number of the Fan

        Returns:
            string: Model number of Fan. Use part number for this.
        """
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the Fan

        Returns:
            string: Serial number of Fan
        """
        return 'N/A'

    def get_part_number(self):
        """
        Retrieves the part number of the Fan

        Returns:
            string: Part number of Fan
        """
        return FAN_PN_F2B if self.get_direction()== self.FAN_DIRECTION_INTAKE else FAN_PN_B2F

    def get_service_tag(self):
        """
        Retrieves the service tag of the Fan

        Returns:
            string: Service Tag of Fan
        """
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the Fan

        Returns:
            bool: True if Fan is operating properly, False if not
        """
        status = False

        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if fan_speed != 'ERR':
            status = True if int(fan_speed) > self.working_fan_speed else False

        return status

    def get_direction(self):
        """
        Retrieves the fan airflow direction
        Possible fan directions (relative to port-side of device)
        Returns:
            A string, either FAN_DIRECTION_INTAKE or
            FAN_DIRECTION_EXHAUST depending on fan direction
        """

        direction = read_sysfs_file(self.get_fan_direction)
        if direction != 'ERR':
            return self.FAN_DIRECTION_INTAKE if int(direction) == 1 else self.FAN_DIRECTION_EXHAUST
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
        return True

    def get_speed(self):
        """
        Retrieves the speed of fan as a percentage of full speed

        Returns:
            An integer, the percentage of full fan speed, in the range 0 (off)
                 to 100 (full speed)
        """
        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if fan_speed != 'ERR':
            speed_in_rpm = int(fan_speed)*100
        else:
            speed_in_rpm = 0

        speed = speed_in_rpm//self.max_fan_speed
        speed = min(speed, 100)
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

        if self.get_direction() == self.FAN_DIRECTION_INTAKE:
            speed_to_duty = {
                range(0, 31): 0x0,
                range(31, 42): 0x4,
                range(42, 47): 0x5,
                range(47, 53): 0x6,
                range(53, 59): 0x7,
                range(59, 65): 0x8,
                range(65, 70): 0x9,
                range(70, 76): 0xA,
                range(76, 82): 0xB,
                range(82, 90): 0xC,
                range(90, 94): 0xD,
                range(94, 99): 0xE,
                range(99, 101): 0xF
            }
        else:
            speed_to_duty = {
                range(0, 31): 0x0,
                range(31, 42): 0x4,
                range(42, 47): 0x5,
                range(47, 54): 0x6,
                range(54, 59): 0x7,
                range(59, 65): 0x8,
                range(65, 71): 0x9,
                range(71, 77): 0xA,
                range(77, 83): 0xB,
                range(83, 88): 0xC,
                range(88, 95): 0xD,
                range(95, 99): 0xE,
                range(99, 101): 0xF
            }

        fan_duty_cycle = None

        for speed_range, duty in speed_to_duty.items():
            if speed in speed_range:
                fan_duty_cycle = duty
                break

        if fan_duty_cycle is not None:
            if write_sysfs_file(self.set_fan_speed_reg, str(fan_duty_cycle)) != 'ERR':
                return True
        return False

    def set_status_led(self, _color):
        """
        Set led to expected color
        Args:
            color: A string representing the color with which to set the
                   fan module status LED
        Returns:
            bool: True if set success, False if fail.
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

        result = read_sysfs_file(self.fan_led_reg)

        if result in {"base", "off"}:
            return self.STATUS_LED_COLOR_OFF
        if result == "green":
            return self.STATUS_LED_COLOR_GREEN
        if result == "red":
            return self.STATUS_LED_COLOR_RED
        return 'N/A'

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0
            (off) to 100 (full speed)
        """
        if self.get_direction() == self.FAN_DIRECTION_INTAKE:
            duty_to_speed = {
                0x0: 25,
                0x4: 38,
                0x5: 45,
                0x6: 50,
                0x7: 56,
                0x8: 62,
                0x9: 67,
                0xA: 73,
                0xB: 79,
                0xC: 86,
                0xD: 92,
                0xE: 96,
                0xF: 100
            }
        else:
            duty_to_speed = {
                0x0: 25,
                0x4: 38,
                0x5: 45,
                0x6: 50,
                0x7: 56,
                0x8: 62,
                0x9: 68,
                0xA: 74,
                0xB: 80,
                0xC: 85,
                0xD: 91,
                0xE: 96,
                0xF: 100
            }

        fan_duty = read_sysfs_file(self.set_fan_speed_reg)
        if fan_duty != 'ERR':
            dutyspeed = int(fan_duty)
            return duty_to_speed.get(dutyspeed, 0)
        return 0
