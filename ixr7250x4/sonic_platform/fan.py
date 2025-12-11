"""
    NOKIA 7250 IXR-X

    Module contains an implementation of SONiC Platform Base API and
    provides the Fans' information which are available in the platform
"""

try:
    import time
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
WORKING_FAN_SPEED = 2300

REG_DIR = "/sys/bus/pci/devices/0000:01:00.0/"
HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/"
I2C_BUS = [11, 12, 13]
FAN_INDEX_IN_DRAWER = [1, 2, 4, 5]
CTRLOR_ARRD = '20'
EEPROM_ADDR = '54'
LED_ADDR = '60'

sonic_logger = logger.Logger('nokia_fan')

class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, drawer_index, drawer_eeprom=None, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan
        i2c_bus = I2C_BUS[drawer_index]
        self.i2c_dev = f"{i2c_bus}-00{CTRLOR_ARRD}"
        self.fan_led_color = ['off', 'green', 'amber', 'green_blink']
        self.led_dir = f"/sys/bus/i2c/devices/{i2c_bus}-00{LED_ADDR}/"
        self.eeprom_dir = f"/sys/bus/i2c/devices/{i2c_bus}-00{EEPROM_ADDR}/"
        
        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = drawer_index * FANS_PER_DRAWER + fan_index + 1
            self.reg_dir = REG_DIR
            self.fan_drawer = drawer_index
            self.tach_index = FAN_INDEX_IN_DRAWER[fan_index]
            self.fan_inited = False
            
            if self.tach_index == 1 or self.tach_index == 2:
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

    def fan_init(self):
        """
        Retrieves the presence of the Fan Unit

        Returns:
            bool: True if Fan is present, False if not
        """
        hwmon_path = glob.glob(HWMON_DIR.format(self.i2c_dev))
        self.set_fan_speed_reg = hwmon_path[0] + f"pwm{self.tach_index}"
        self.get_fan_speed_reg = hwmon_path[0] + f"fan{self.tach_index}_input"
        self.fan_speed_enable_reg = hwmon_path[0] + f"fan{self.tach_index}_enable"
        self.pwm_enable_reg = hwmon_path[0] + f"pwm{self.tach_index}_enable"
        
        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            if (int(fan_speed) > WORKING_FAN_SPEED):
                self.fan_inited = True
                return True
            
        result = write_sysfs_file(self.pwm_enable_reg, '0')
        if (result == 'ERR'):
            return False
        time.sleep(0.1)
        write_sysfs_file(self.pwm_enable_reg, '1')
        time.sleep(0.1)
        write_sysfs_file(self.fan_speed_enable_reg, '1')
        self.fan_inited = True
        return True

    def get_presence(self):
        """
        Retrieves the presence of the Fan Unit

        Returns:
            bool: True if Fan is present, False if not
        """
        result = read_sysfs_file(self.reg_dir + f'fandraw_{self.fan_drawer+1}_prs')
        if result == '0': # present
            if not self.fan_inited:
                self.fan_init()

            return True
        
        if self.fan_inited:
            self.fan_inited = False
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

        if not self.fan_inited:
            if not self.fan_init():
                return status
        
        fan_speed = read_sysfs_file(self.get_fan_speed_reg)
        if (fan_speed != 'ERR'):
            speed_in_rpm = int(fan_speed)
            if speed_in_rpm == 0:
                write_sysfs_file(self.pwm_enable_reg, '0')
                time.sleep(0.1)
                write_sysfs_file(self.pwm_enable_reg, '1')
                time.sleep(0.1)
                write_sysfs_file(self.fan_speed_enable_reg, '1')
                fan_speed = read_sysfs_file(self.get_fan_speed_reg)
                speed_in_rpm = int(fan_speed)
            target_speed = self.get_target_speed()
            if ((speed_in_rpm / self.max_fan_speed) > ((target_speed - 25) / 100)) and (speed_in_rpm > WORKING_FAN_SPEED):
                status = True
            else:
                sonic_logger.log_warning(f"!Warning: {self.get_name()} speed: {speed_in_rpm} RPM, target speed: {target_speed}%")
        
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
        if not self.fan_inited:
            if not self.get_presence():
                return 0
        
        if not self.get_status():
            return 0

        fan_duty = read_sysfs_file(self.set_fan_speed_reg)
        if fan_duty != 'ERR':
            dutyspeed = int(fan_duty)
            fan_speed = round(dutyspeed / 2.55)
            return fan_speed
        return 0
    
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
        if not self.fan_inited:
            if not self.get_presence():
                return False
        
        if speed >= 20 and speed <= 100:
            fan_duty_cycle = round(speed * 2.55)
        elif speed >= 0 and speed < 20:
            fan_duty_cycle = 0
        else:
            return False

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

        result = read_sysfs_file(self.led_dir + 'fan_led')
        val = int(result)
        if val < len(self.fan_led_color):
            return self.fan_led_color[val]
        return 'N/A'

    def get_target_speed(self):
        """
        Retrieves the target (expected) speed of the fan

        Returns:
            An integer, the percentage of full fan speed, in the range 0
            (off) to 100 (full speed)
        """
        if not self.fan_inited:
            if not self.get_presence():
                return False
        
        fan_duty = read_sysfs_file(self.set_fan_speed_reg)
        if fan_duty != 'ERR':
            dutyspeed = int(fan_duty)
            target_speed = round(dutyspeed / 2.55)
            return target_speed
        return 0
