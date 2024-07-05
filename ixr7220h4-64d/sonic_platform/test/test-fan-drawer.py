#!/usr/bin/python
import unittest
from sonic_platform.chassis import Chassis

class Test1(unittest.TestCase):
    def test_1(self):
        print("---------------------------")
        print("Chassis Fan Drawer Unit Test")
        print("---------------------------")

        chassis = Chassis()

        for fan_drawer in chassis.get_all_fan_drawers():
            if not fan_drawer.get_presence():
                print("    Name: {} not present".format(fan_drawer.get_name()))
            else:
                print("    Name:", fan_drawer.get_name())
                print("        Presence: {}, Status: {}, LED: {}".format(fan_drawer.get_presence(),
                                                                        fan_drawer.get_status(),
                                                                        fan_drawer.get_status_led()))
                print("        Model: {}, Serial#: {}".format(fan_drawer.get_model(),
                                                            fan_drawer.get_serial()))
                print("        Part#: {}".format(fan_drawer.get_part_number()))
                print("        Direction: {}\n".format(fan_drawer.get_direction()))
                print("        Replaceable: {}, Index: {}\n".format(fan_drawer.is_replaceable(), fan_drawer.get_position_in_parent()))


            for color in ['off', 'red', 'green']:
                fan_drawer.set_status_led(color)
                t_value = fan_drawer.get_status_led()
                self.assertEqual(color, t_value)

if __name__ == '__main__':
    unittest.main()