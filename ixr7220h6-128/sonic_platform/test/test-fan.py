#!/usr/bin/python

from sonic_platform.chassis import Chassis

def main():
    print("---------------------")
    print("Chassis Fan Unit Test")
    print("---------------------")

    chassis = Chassis()

    for fandraw in chassis.get_all_fan_drawers():
        if not fandraw.get_presence():
            print("\n    Name: {} not present".format(fandraw.get_name()))
        else:
            print("    Name:", fandraw.get_name())
            print("        Presence: {}, Status: {}, Direction: {}, LED: {}".format(fandraw.get_presence(),
                                                                                    fandraw.get_status(),
                                                                                    fandraw.fan_direction_intake,
                                                                                    fandraw.get_status_led()))
            print("        Part_number: {}, Serial: {}".format(fandraw.get_part_number(), 
                                                               fandraw.get_serial()))
            print("        Service Tag: {}, mfg_date: {}, ".format(fandraw.get_service_tag(), 
                                                                   fandraw.get_manuf_date()))
            for fan in fandraw.get_all_fans():
                fan_status = fan.get_status()
                print("        {} Status: {}, Target Speed: {}%,  Speed: {}%".format(fan.get_name(),
                                                                                     fan_status,
                                                                                    str(fan.get_target_speed()),
                                                                                    str(fan.get_speed())))
    return

if __name__ == '__main__':
    main()
