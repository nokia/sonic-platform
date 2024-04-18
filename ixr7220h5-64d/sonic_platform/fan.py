########################################################################
# Nokia IXR7220 H5-64D
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
    from os import *
    from mmap import *
    from sonic_platform_base.fan_base import FanBase
    from sonic_platform.eeprom import Eeprom
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

H5_64D_FANS_PER_DRAWER = 2

FAN_PN_F2B = "F2B_FAN"
FAN_PN_B2F = "B2F_FAN"
MAX_FAN_F_SPEED = 15700
MAX_FAN_R_SPEED = 14600
FAN_TOLERANCE = 50
WORKING_ixr7220_FAN_SPEED = 1500

HWMON_DIR = "/sys/bus/i2c/devices/{}/hwmon/hwmon*/" 
I2C_DEV_LIST = [("5-002d", "5-002d"),
                ("5-002d", "5-002d"),                
                ("5-004c", "5-004c"),
                ("5-004c", "5-004c")]
FAN_INDEX_IN_DRAWER = [(1, 2),
                       (3, 4),
                       (1, 2),
                       (3, 4)]
GPIO_DIR = "/sys/class/gpio/gpio{}/" 
GPIO_PORT = [10224, 10225, 10226, 10227]
RESOURCE = "/sys/bus/pci/devices/0000:02:00.0/resource0"
REG_FAN_LED = 0x00A0
INDEX_FAN_LED = [0, 4, 8, 12]

sonic_logger = logger.Logger('fan')


class Fan(FanBase):
    """Nokia platform-specific Fan class"""

    def __init__(self, fan_index, drawer_index, psu_fan=False, dependency=None):
        self.is_psu_fan = psu_fan
        i2c_dev = I2C_DEV_LIST[drawer_index][fan_index]
        hwmon_path = glob.glob(HWMON_DIR.format(i2c_dev))

        if not self.is_psu_fan:
            # Fan is 1-based in Nokia platforms
            self.index = drawer_index * H5_64D_FANS_PER_DRAWER + fan_index + 1
            self.fan_drawer = drawer_index
            fan_index_emc230x = FAN_INDEX_IN_DRAWER[drawer_index][fan_index]
            self.set_fan_speed_reg = hwmon_path[0] + "pwm{}".format(fan_index_emc230x)
            self.get_fan_speed_reg = hwmon_path[0] + "fan{}_input".format(fan_index_emc230x)
            self.gpio_dir = GPIO_DIR.format(GPIO_PORT[drawer_index])
           
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
    
    def pci_set_value(resource, data, offset):
        fd = open(resource, O_RDWR)
        mm = mmap(fd, 0)
        mm.seek(offset)
        mm.write(struct.pack('I', data))
        mm.close()
        close(fd)

    def pci_get_value(resource, offset):
        fd = open(resource, O_RDWR)
        mm = mmap(fd, 0)
        mm.seek(offset)
        read_data_stream = mm.read(4)
        reg_val = struct.unpack('I', read_data_stream)
        mm.close()
        close(fd)
        return reg_val

    
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
        part_number = self.get_part_number()
        if part_number == FAN_PN_F2B:
            return self.FAN_DIRECTION_INTAKE
        elif part_number == FAN_PN_F2B:
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

        if speed in range(0, 10):
            fandutycycle = 0x00
        elif speed in range(10, 21):
            fandutycycle = 32
        elif speed in range(21, 31):
            fandutycycle = 64
        elif speed in range(31, 46):
            fandutycycle = 102
        elif speed in range(46, 61):
            fandutycycle = 140
        elif speed in range(61, 81):
            fandutycycle = 204
        elif speed in range(81, 101):
            fandutycycle = 255
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

        val = self.pci_get_value(RESOURCE, REG_FAN_LED) 
        result = val[0] & (0x7<<INDEX_FAN_LED[self.fan_drawer]) >> INDEX_FAN_LED[self.fan_drawer]

        if result == 0 or result == 6 or result == 7:
            return self.STATUS_LED_COLOR_OFF
        elif result == 1:
            return self.STATUS_LED_COLOR_GREEN
        elif result == 2:
            return self.STATUS_LED_COLOR_AMBER
        elif result == 3:
            return self.STATUS_LED_COLOR_GREEN_BLINK
        elif result == 4:
            return self.STATUS_LED_COLOR_AMBER_BLINK
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
            elif dutyspeed == 32:
                speed = 15
            elif dutyspeed == 64:
                speed = 25
            elif dutyspeed == 102:
                speed = 40
            elif dutyspeed == 140:
                speed = 50
            elif dutyspeed == 204:
                speed = 70
            elif dutyspeed == 255:
                speed = 100

        return speed
