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
    print("Chassis PSU Unit Test")
    print("---------------------")

    chassis = Chassis()

    for psu in chassis.get_all_psus():
        if not psu.get_presence():
            print("    Name: {} not present".format(psu.get_name()))
        else:
            print("    Name:", psu.get_name())
            print("        Presence: {}, Status: {}, LED: {}".format(psu.get_presence(),
                                                                     psu.get_status(),
                                                                     psu.get_status_led()))
            print("        Part_number: {}, Mfg_date: {}, Serial: {}".format(psu.get_part_number(),
                                                                     psu.get_revision(),
                                                                     psu.get_serial()))
            clei = read_sysfs_file(psu.eeprom_dir + "clei")
            hw_type = read_sysfs_file(psu.eeprom_dir + "hw_type")
            hw_directives = read_sysfs_file(psu.eeprom_dir + "hw_directives")
            print("        CLEI: {}, hw_type: {}, hw_directives: {}".format(clei.strip(),
                                                                            hw_type.strip(),
                                                                            hw_directives.strip()))
            try:
                current = psu.get_current()
            except NotImplementedError:
                current = "NA"
            try:
                power = psu.get_power()
            except NotImplementedError:
                power = "NA"

            print("        Output Voltage: {}, Output Current: {}, Input Power: {}\n".format(psu.get_voltage(),
                                                                         current,
                                                                         power))
    return


if __name__ == '__main__':
    main()
