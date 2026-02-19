#!/usr/bin/env python3
"""
    port_notify:
    notify port status change from Sonic DB
"""

try:
    from swsscommon import swsscommon
    from sonic_py_common import daemon_base, logger
    from sonic_platform.sysfs import write_sysfs_file
except ImportError as e:
    raise ImportError (str(e) + " - required module not found")

SYSLOG_IDENTIFIER = "ports_notify"

SELECT_TIMEOUT_MSECS = 1000

PORT_NUM = 130
SYSFS_DIR = "/sys/bus/i2c/devices/{}/"
PORTPLD_ADDR = ["152-0076", "153-0076", "148-0074", "149-0075", "150-0073", "151-0073"]
ADDR_IDX = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
            2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
            4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,3,3]
PORT_IDX = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
            1,2,5,6,9,10,13,14,17,18,21,22,25,26,29,30,1,2,5,6,9,10,13,14,17,18,21,22,25,26,29,30,
            3,4,7,8,11,12,15,16,19,20,23,24,27,28,31,32,3,4,7,8,11,12,15,16,19,20,23,24,27,28,31,32,
            1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,33,34]

# Global logger class instance
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)

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
    port_tbl.filter = ['admin_status']
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

            if 'admin_status' in fvp:
                if 'index' in fvp:
                    port_index = int(fvp['index'])
                    if port_index in range(1, PORT_NUM+1):
                        pld_path = SYSFS_DIR.format(PORTPLD_ADDR[ADDR_IDX[port_index-1]])
                        pld_port_idx = PORT_IDX[port_index-1]
                        file_name = pld_path + f"port_{pld_port_idx}_en"
                    else:
                        logger.log_warning(f"Wrong port index {port_index} for port {port_name}")
                        continue
                else:
                    logger.log_warning(f"Wrong index from port {port_name}: {fvp}")
                    continue

                if fvp['admin_status'] == 'up':
                    write_sysfs_file(file_name, '1')
                elif fvp['admin_status'] == 'down':
                    write_sysfs_file(file_name, '0')

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
