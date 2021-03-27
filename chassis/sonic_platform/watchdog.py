from __future__ import print_function

try:
    from sonic_platform_base.watchdog_base import WatchdogBase
    from sonic_py_common.logger import Logger

except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
logger = Logger("wdog")


class Watchdog(WatchdogBase):
    _initialized = False

    def __init__(self, watchdog):
        # logger.set_min_log_priority_info()
        logger.log_info("Watchdog __init__{}".format(self))

    def arm(self, seconds):
        self._initialized = True
        logger.log_info("Watchdog arm")
        # hook this up to actual kicker shortly
        return seconds

    def disarm(self):
        logger.log_info("Watchdog disarm")
        return False

    def is_armed(self):
        if self._initialized is True:
            logger.log_info("Watchdog is_armed")
            return True
        else:
            return False

    def get_remaining_time(self):
        logger.log_info("Watchdog get_remaining_time")
        raise NotImplementedError
