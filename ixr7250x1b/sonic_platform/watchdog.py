"""
    Module contains an implementation of SONiC Platform Base API and
    provides access to hardware watchdog
"""
try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "nokia-wd"
sonic_logger = logger.Logger(SYSLOG_IDENTIFIER)
sonic_logger.set_min_log_priority_info()

class WatchdogImplBase(WatchdogBase):
    _initialized = False

    def __init__(self):
        super(WatchdogImplBase, self).__init__()
        sonic_logger.log_info("Watchdog __init__{}".format(self))

    def arm(self, seconds):
        self._initialized = True
        sonic_logger.log_info("Watchdog arm")
        # hook this up to actual kicker shortly
        return seconds

    def disarm(self):
        sonic_logger.log_info("Watchdog disarm")
        return False

    def is_armed(self):
        if self._initialized is True:
            sonic_logger.log_info("Watchdog is_armed")
            return True
        else:
            return False

    def get_remaining_time(self):
        sonic_logger.log_info("Watchdog get_remaining_time")
        return 30
