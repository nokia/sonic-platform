"""
    NOKIA 7250 IXR-X3B

    Module contains an implementation of SONiC Platform Base API and
    provides the Fans' information which are available in the platform
"""

try:
    import os
    import glob
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.sysfs import read_sysfs_file, write_sysfs_file
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

FANS_PER_DRAWER = 4
MAX_FAN_F_SPEED = 27000
MAX_FAN_R_SPEED = 23000
FAN_TOLERANCE = 50
WORKING_FAN_SPEED = 2205

REG_DIR = "/sys/bus/pci/devices/0000:01:00.0/"
HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/"
I2C_BUS = [11, 12, 13]
FAN_INDEX_IN_DRAWER = [1, 2, 4, 5]
CTRLOR_ARRD = '20'
EEPROM_ADDR = '54'

sonic_logger = logger.Logger('fan')

class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, drawer_index, drawer_eeprom=None, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan
        i2c_bus = I2C_BUS[drawer_index]
        i2c_dev = f"{i2c_bus}-00{CTRLOR_ARRD}"
        hwmon_path = glob.glob(HWMON_DIR.format(i2c_dev))
        self.fan_led_color = ['off', 'green','amber', 'green_blink',
                            'amber_blink', 'alter_green_amber', 'off', 'off']        
        
        self.eeprom_dir = f"/sys/bus/i2c/devices/{i2c_bus}-00{EEPROM_ADDR}/"
        self.new_cmd = f"echo fan_eeprom 0x{EEPROM_ADDR} > /sys/bus/i2c/devices/i2c-{i2c_bus}/new_device"
        self.del_cmd = f"echo 0x{EEPROM_ADDR} > /sys/bus/i2c/devices/i2c-{i2c_bus}/delete_device"

        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = drawer_index * FANS_PER_DRAWER + fan_index + 1
            self.reg_dir = REG_DIR
            self.fan_drawer = drawer_index
            tach_index = FAN_INDEX_IN_DRAWER[fan_index]
            self.set_fan_speed_reg = hwmon_path[0] + f"pwm{tach_index}"
            self.get_fan_speed_reg = hwmon_path[0] + f"fan{tach_index}_input"
            self.fan_speed_enable_reg = hwmon_path[0] + f"fan{tach_index}_enable"
            self.pwm_enable_reg = hwmon_path[0] + f"pwm{tach_index}_enable"
            self.pwm_enabled = False
            
            if tach_index == 1 or tach_index == 2:
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
        else:
            return f"PSU{self.index}_Fan"

    def get_presence(self):
        """
        Retrieves the presence of the Fan Unit

        Returns:
            bool: True if Fan is present, False if not
        """
        result = read_sysfs_file(self.reg_dir + f'fandraw_{self.fan_drawer+1}_prs')
        if result == '0': # present
            if not os.path.exists(self.eeprom_dir):
                os.system(self.new_cmd)
            if not self.pwm_enabled:
                result = write_sysfs_file(self.pwm_enable_reg, '1')
                if (result == 'ERR'):
                    return False
                self.pwm_enabled = True

            return True
        # not present
        if os.path.exists(self.eeprom_dir):
            os.system(self.del_cmd)
        if self.pwm_enabled:
            self.pwm_enabled = False
        return False

    def get_model(self):
        """
        Retrieves the model number of the Fan

        Returns:
            string: Model number of Fan. Use part number for this.
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "part_number")
            return result.strip()
        return 'N/A'

    def get_serial(self):
        """
        Retrieves the serial number of the Fan

        Returns:
            string: Serial number of Fan
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "serial_number")
            return result.strip()
        return 'N/A'

    def get_part_number(self):
        """
        Retrieves the part number of the Fan

        Returns:
            string: Part number of Fan
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "part_number")
            return result.strip()
        return 'N/A'

    def get_service_tag(self):
        """
        Retrieves the service tag of the Fan

        Returns:
            string: Service Tag of Fan
        """
        if self.get_presence():
            result = read_sysfs_file(self.eeprom_dir + "clei")
            return result.strip()
        return 'N/A'

    def get_status(self):
        """
        Retrieves the operational status of the Fan

        Returns:
            bool: True if Fan is operating properly, False if not
        """
        status = False

        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            if (int(fan_speed) > WORKING_FAN_SPEED):
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

        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            speed_in_rpm = int(fan_speed)
            if speed_in_rpm == 0:
                fan_enable = read_sysfs_file(self.fan_speed_enable_reg)
                if fan_enable == '0':
                    rv = write_sysfs_file(self.fan_speed_enable_reg, '1')
                    if (rv == 'ERR'):
                        return 0
                    fan_speed = read_sysfs_file(self.get_fan_speed_reg)
                    speed_in_rpm = int(fan_speed)
        else:
            speed_in_rpm = 0

        speed = round(100*speed_in_rpm/self.max_fan_speed)

        return min(speed, 100)

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
            range(0, 10): 0x00,
            range(10, 20): 40,   # 15%
            range(20, 30): 64,   # 25%
            range(30, 40): 90,   # 35%
            range(40, 54): 115,  # 45%
            range(54, 66): 153,  # 60%
            range(66, 76): 179,  # 70%
            range(76, 86): 204,  # 80%
            range(86, 96): 230,  # 90%
            range(96, 101): 255  # 100%
        }

        fan_duty_cycle = None

        for speed_range, duty in speed_to_duty.items():
            if speed in speed_range:
                fan_duty_cycle = duty
                break

        pwm_enable = read_sysfs_file(self.pwm_enable_reg)
        if pwm_enable == '0':
            write_sysfs_file(self.pwm_enable_reg, '1')
        rv = write_sysfs_file(self.set_fan_speed_reg, str(fan_duty_cycle))
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
        """
        return False

    def get_status_led(self):
        """
        Gets the state of the fan status LED

        Returns:
            A string, one of the predefined STATUS_LED_COLOR_* strings.
        """
        if not self.get_presence():
            return 'N/A'

        # result = read_sysfs_file(FPGA_DIR + f'fan{self.fan_drawer+1}_led')
        # val = int(result, 16) & 0x7

        # if val < len(self.fan_led_color):
        #     return self.fan_led_color[val]

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
            40: 15,
            64: 25,
            90: 35,
            115: 45,
            153: 60,
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
