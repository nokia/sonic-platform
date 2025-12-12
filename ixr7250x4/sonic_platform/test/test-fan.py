#!/usr/bin/python

from sonic_platform.chassis import Chassis

def read_sysfs_file(sysfs_file):
    """
    On successful read, returns the value read from given
    reg_name and on failure returns ERR
    """
    rv = 'ERR'

    try:
        with open(sysfs_file, 'r', encoding='utf-8') as fd:
            rv = fd.read()
            fd.close()
    except FileNotFoundError:
        print(f"Error: {sysfs_file} doesn't exist.")
    except PermissionError:
        print(f"Error: Permission denied when reading file {sysfs_file}.")
    except IOError:
        print(f"IOError: An error occurred while reading file {sysfs_file}.")
    if rv != 'ERR':
        rv = rv.rstrip('\r\n')
        rv = rv.lstrip(" ")
    return rv

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
            print("        Presence: {}, Status: {}, LED: {}".format(fandraw.get_presence(),
                                                                     fandraw.get_status(),
                                                                     fandraw.get_status_led()))
            mfg_date = read_sysfs_file(fandraw.eeprom_dir + "mfg_date")
            print("        Part_number: {}, mfg_date: {}, Serial: {}".format(fandraw.get_model(),
                                                                             mfg_date.strip(),
                                                                             fandraw.get_serial()))
            clei = read_sysfs_file(fandraw.eeprom_dir + "clei")
            assembly_num = read_sysfs_file(fandraw.eeprom_dir + "assembly_num")
            platforms = read_sysfs_file(fandraw.eeprom_dir + "platforms")            
            print("        CLEI: {}, assembly_num: {}, platforms: {}, ".format(clei.strip(), 
                                                                               assembly_num.strip(),
                                                                               platforms.strip()))
            hw_type = read_sysfs_file(fandraw.eeprom_dir + "hw_type")
            hw_directives = read_sysfs_file(fandraw.eeprom_dir + "hw_directives")
            print("        hw_type: {}, hw_directives: {}".format(hw_type.strip(), hw_directives.strip()))
            for fan in fandraw.get_all_fans():
                fan_status = fan.get_status()
                rpm = read_sysfs_file(fan.get_fan_speed_reg)
                print("        {} Status: {}, Target Speed: {}%,  Speed: {}%, RPM: {}".format(fan.get_name(),
                                                                                     fan_status,
                                                                                    str(fan.get_target_speed()),
                                                                                    str(fan.get_speed()), int(rpm)))
    return


if __name__ == '__main__':
    main()
