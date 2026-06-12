#!/usr/bin/env python3
"""
    port_notify:
    notify port status change from Sonic DB
"""

try:
    from swsscommon import swsscommon
    from sonic_py_common import daemon_base, logger
    from sonic_platform.sysfs import write_sysfs_file, read_sysfs_file
except ImportError as e:
    raise ImportError (str(e) + " - required module not found")

SYSLOG_IDENTIFIER = "ports_notify"

SELECT_TIMEOUT_MSECS = 1000

PORT_END = 129
SYSFS_DIR = "/sys/bus/i2c/devices/{}/"
PORTPLD_ADDR = ["153-0076", "154-0076", "149-0074", "150-0075", "151-0073", "152-0073"]
ADDR_IDX = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
            4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,3]
PORT_IDX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
            1,2,5,6,9,10,13,14,17,18,21,22,25,26,29,30,1,2,5,6,9,10,13,14,17,18,21,22,25,26,29,30,
            3,4,7,8,11,12,15,16,19,20,23,24,27,28,31,32,3,4,7,8,11,12,15,16,19,20,23,24,27,28,31,32,
            1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,33]

# Global logger class instance
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
# sonic_logger.set_min_log_priority_info()

def wait_for_port_init_done():
    # Connect to APPL_DB and subscribe to PORT table notifications
    appl_db = daemon_base.db_connect("APPL_DB")
    sel = swsscommon.Select()
    sst = swsscommon.SubscriberStateTable(appl_db, swsscommon.APP_PORT_TABLE_NAME)
    sel.addSelectable(sst)

    # Make sure this daemon started after all port configured
    while True:
        (state, c) = sel.select(1000)
        if state == swsscommon.Select.TIMEOUT:
            continue
        if state != swsscommon.Select.OBJECT:
            sonic_logger.log_warning("sel.select() did not return swsscommon.Select.OBJECT")
            continue

        (key, op, fvp) = sst.pop()

        # Wait until PortInitDone
        if key in ["PortInitDone"]:
            break

def subscribe_port_config_change():
    sel = swsscommon.Select()
    config_db = daemon_base.db_connect("CONFIG_DB")
    port_tbl = swsscommon.SubscriberStateTable(config_db, swsscommon.CFG_PORT_TABLE_NAME)
    port_tbl.filter = ['admin_status', 'lanes']
    sel.addSelectable(port_tbl)
    return sel, port_tbl

def handle_port_config_change(sel, port_config, logger):
    """Select PORT table changes, once there is a port configuration add/remove, notify observers
    """
    try:
        (state, _) = sel.select(SELECT_TIMEOUT_MSECS)
    except Exception:
        return -1

    if state == swsscommon.Select.TIMEOUT:
        return 0
    if state != swsscommon.Select.OBJECT:
        return -2

    while True:
        (port_name, op, fvp) = port_config.pop()
        if not port_name:
            break

        if fvp is not None:
            fvp = dict(fvp)

            if 'index' in fvp:
                port_index = int(fvp['index'])
                if port_index in range(1, PORT_END+1):
                    pld_path = SYSFS_DIR.format(PORTPLD_ADDR[ADDR_IDX[port_index-1]])
                    pld_port_idx = PORT_IDX[port_index-1]
                    admin_file_name = pld_path + f"port_{pld_port_idx}_en"
                    brkt_file_name = pld_path + f"port_{pld_port_idx}_brkt"
                else:
                    logger.log_info(f"Wrong port index {port_index} for port {port_name}")
                    continue
            else:
                logger.log_info(f"Wrong index from port {port_name}: {fvp}")
                continue

            if 'lanes' in fvp:
                lanes = len(fvp['lanes'].split(','))
                if lanes <= 0 and lanes > 8:
                    continue
                else:
                    if lanes == 8:
                        if read_sysfs_file(brkt_file_name) != '0x00':
                            write_sysfs_file(brkt_file_name, '0x00')
                    elif lanes == 4:
                        if read_sysfs_file(brkt_file_name) != '0x11':
                            write_sysfs_file(brkt_file_name, '0x11')
                    elif lanes == 2:
                        if read_sysfs_file(brkt_file_name) != '0x55':
                            write_sysfs_file(brkt_file_name, '0x55')
                    elif lanes == 1:
                        if read_sysfs_file(brkt_file_name) != '0xff':
                            write_sysfs_file(brkt_file_name, '0xff')
            else:
                continue
                
            if 'admin_status' in fvp:
                if 'subport' in fvp and fvp['subport'] == '0':
                    if fvp['admin_status'] == 'up':
                        write_sysfs_file(admin_file_name, '0xff')
                    elif fvp['admin_status'] == 'down':
                        write_sysfs_file(admin_file_name, '0x0')
                else:
                    mask = 0xff >> (8 - lanes)
                    subport = int(fvp['subport'])
                    reg_mask = (~(mask << ((subport - 1) * lanes))) & 0xFF
                    reg_val = int(read_sysfs_file(admin_file_name), 16)
                    reg_val = reg_val & reg_mask
                    admin = {'up': 1, 'down': 0}[fvp['admin_status']]
                    set_bits = mask if admin else 0
                    result = reg_val | (set_bits << ((subport - 1) * lanes))
                    write_sysfs_file(admin_file_name, hex(result))

    return 0

def main():

    # Wait for PortInitDone
    wait_for_port_init_done()

    sonic_logger.log_info("port init done!")

    sel, port_config = subscribe_port_config_change()

    while True:
        status = handle_port_config_change(sel, port_config, sonic_logger)
        if status < 0:
            return -1


if __name__ == '__main__':
    main()
