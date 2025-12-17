"""
    Nokia IXR7220 H4-32D

    Module contains an implementation of SONiC Platform Base API and
    provides the Fans' information which are available in the platform
"""

try:
    import glob
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

H4_32D_FANS_PER_DRAWER = 2

FAN_PN_F2B = "3HE19865AARA01"
FAN_PN_B2F = "3HE19866AARA01"
MAX_FAN_F_SPEED = 27000
MAX_FAN_R_SPEED = 23000
FAN_TOLERANCE = 50
WORKING_IXR7220_FAN_SPEED = 2300

HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/"
I2C_DEV_LIST = [("10-002e", "10-002e"),
                ("10-002e", "10-002e"),
                ("10-002e", "10-004c"),
                ("10-004c", "10-004c"),
                ("10-004c", "10-004c"),
                ("10-002d", "10-002d"),
                ("10-002d", "10-002d")]
FAN_INDEX_IN_DRAWER = [(1, 2),
                       (3, 4),
                       (5, 1),
                       (2, 3),
                       (4, 5),
                       (1, 2),
                       (3, 4)]
GPIO_DIR = "/sys/class/gpio/gpio{}/"
GPIO_PORT = [698, 699, 700, 701, 702, 703, 704]
FPGA_FAN_LED = "/sys/kernel/delta_fpga/fan-tray-led"
INDEX_FAN_LED = [0, 4, 8, 12, 16, 20 , 24]
sonic_logger = logger.Logger('fan')


class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, drawer_index, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan
        i2c_dev = I2C_DEV_LIST[drawer_index][fan_index]
        hwmon_path = glob.glob(HWMON_DIR.format(i2c_dev))

        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = drawer_index * H4_32D_FANS_PER_DRAWER + fan_index + 1
            self.drawer_index = drawer_index
            fan_index_emc230x = FAN_INDEX_IN_DRAWER[drawer_index][fan_index]
            self.set_fan_speed_reg = hwmon_path[0] + f"pwm{fan_index_emc230x}"
            self.get_fan_speed_reg = hwmon_path[0] + f"fan{fan_index_emc230x}_input"
            self.gpio_dir = GPIO_DIR.format(GPIO_PORT[drawer_index])
            self.system_led_supported_color = ['off', 'green', 'amber', 'green_blink',
                                               'amber_blink', 'alter_green_amber', 'off', 'off']

            # Fan eeprom
            self.eeprom = Eeprom(False, 0, True, drawer_index)

            if fan_index == 0:
                self.max_fan_speed = MAX_FAN_F_SPEED
            else:
                self.max_fan_speed = MAX_FAN_R_SPEED

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
        result = read_sysfs_file(self.gpio_dir + 'value')
        if result == '0':
            return True
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

        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if fan_speed != 'ERR':
            if int(fan_speed) > WORKING_IXR7220_FAN_SPEED:
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
        part_number = self.get_part_number()
        if part_number == FAN_PN_F2B:
            return self.FAN_DIRECTION_INTAKE
        if part_number == FAN_PN_B2F:
            return self.FAN_DIRECTION_EXHAUST
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
        Retrieves the speed of a Front FAN in the tray in revolutions per
                 minute defined by 1-based index
        :param index: An integer, 1-based index of the FAN to query speed
        :return: integer, denoting front FAN speed
        """
        speed = 0

        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if fan_speed != 'ERR':
            speed_in_rpm = int(fan_speed)
        else:
            speed_in_rpm = 0

        speed = 100*speed_in_rpm//self.max_fan_speed
        speed = min(speed, 100)
        return speed

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

        speed_to_duty = {
            range(0, 15): 0x00,
            range(15, 31): 85,
            range(31, 46): 102,
            range(46, 53): 128,
            range(53, 66): 141,
            range(66, 76): 179,
            range(76, 86): 204,
            range(86, 96): 230,
            range(96, 101): 255
        }

        fan_duty_cycle = None

        for speed_range, duty in speed_to_duty.items():
            if speed in speed_range:
                fan_duty_cycle = duty
                break

        rv = write_sysfs_file(self.set_fan_speed_reg, str(fan_duty_cycle)) != 'ERR'
        if rv != 'ERR':
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

            No indivial led for each H4-32D fans
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

        if not self.get_presence():
            return self.STATUS_LED_COLOR_OFF
        result = read_sysfs_file(FPGA_FAN_LED)
        val = int(result, 16)
        result = (val & (0x7<<INDEX_FAN_LED[self.drawer_index])) >> INDEX_FAN_LED[self.drawer_index]
        if result < len(self.system_led_supported_color):
            return self.system_led_supported_color[result]
        return 'N/A'

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0
            (off) to 100 (full speed)
        """
        duty_to_speed = {
            0: 0,
            85: 25,
            102: 35,
            128: 45,
            141: 50,
            179: 70,
            204: 80,
            230: 90,
            255: 100
        }

        fan_duty = read_sysfs_file(self.set_fan_speed_reg)
        if fan_duty != 'ERR':
            dutyspeed = int(fan_duty)
            return duty_to_speed.get(dutyspeed, 0)
        return 0
