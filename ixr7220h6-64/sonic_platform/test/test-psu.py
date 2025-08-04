#!/usr/bin/python

from sonic_platform.chassis import Chassis


def main():
    print("---------------------")
    print("Chassis PSU Unit Test")
    print("---------------------")

    chassis = Chassis()

    for psu in chassis.get_all_psus():
        if psu.index == 1:
            print("    Active num:", psu._get_active_psus())
            print("    Master LED: {}\n".format(psu.get_status_master_led()))

        if not psu.get_presence():
            print("    Name: {} not present\n".format(psu.get_name()))
        else:
            print("    Name:", psu.get_name())
            print("        Presence: {}, Status: {}, LED: {}".format(psu.get_presence(),
                                                                     psu.get_status(),
                                                                     psu.get_status_led()))
            print("        Model: {}, Serial#: {}, Part#: {}".format(psu.get_model(),
                                                                     psu.get_serial(),
                                                                     psu.get_part_number()))
            print("        Mfg_date: {}, FW_Rev: {}".format(psu.get_mfg_date(),
                                                            psu.get_fw_rev()))
            try:
                current = psu.get_current()
            except NotImplementedError:
                current = "NA"
            try:
                power = psu.get_power()
            except NotImplementedError:
                power = "NA"

            print("        Voltage: {}, Current: {}, Power: {}\n".format(psu.get_voltage(),
                                                                         current,
                                                                         power))

if __name__ == '__main__':
    main()
