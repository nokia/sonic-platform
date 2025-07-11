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

CPLD2_DIR = "/sys/bus/i2c/devices/10-0062/"  # qsfpx_port  x in [19,36]
CPLD3_DIR = "/sys/bus/i2c/devices/10-0064/"  # qsfpx_port  x in [ 1,18]

PORT_NUM = 36

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
    appl_db = daemon_base.db_connect("APPL_DB")
    port_tbl = swsscommon.SubscriberStateTable(appl_db, swsscommon.APP_PORT_TABLE_NAME)
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
                    if port_index in range(1, 19):
                        file_name = CPLD3_DIR + f"qsfp{port_index}_port"
                    elif port_index in range(19, PORT_NUM+1):
                        file_name = CPLD2_DIR + f"qsfp{port_index}_port"
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
