# Name: sfp.py, version: 1.0
#
# Description: Module contains the definitions of SFP related APIs
# for Nokia IXR 7250 platform.
#
# Copyright (c) 2022, Nokia
# All rights reserved.
#

try:
    # from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2
    from sonic_py_common.logger import Logger
    from sonic_py_common import device_info
    import time
    import fcntl
    import threading
    from multiprocessing import RawValue, RawArray
    import signal
    import mmap
    import os
    import sys
    import inspect


except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
logger = Logger()

READ_TYPE = 0
KEY_OFFSET = 1
KEY_WIDTH = 2
FUNC_NAME = 3
STRUCT_TYPE = 4
PAGE_NUMBER = 5


INFO_TYPE = platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_DATA
CTRL_TYPE = platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_CTRL
EXT_CTRL_TYPE = platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_EXT_CTRL
UPPER_PAGE_TYPE = platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_UPPER_PAGE
UPPER_BANK_PAGE_TYPE = platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_UPPER_BANK_AND_PAGE
DIRECT_TYPE = platform_ndk_pb2.ReqSfpEepromType.SFP_EEPROM_DIRECT

# Module Direct IPC attributes
# MDIPC_BASE_NAME = '/dev/shm/MDIPC'
MDIPC_BASE_NAME = '/var/run/redis/MDIPC'
MDIPC_NUM_CHANNELS = 10

MDIPC_OWN_NDK = 0x5A5A3C3C
MDIPC_OWN_NOS = 0X43211234
MDIPC_OWN_NOS_PREP = 0XDEADBEEF
MDIPC_OWN_NOS_RSP = 0xCBCBAF5F

MDIPC_READ  = 0
MDIPC_WRITE = 1
MDIPC_PRESENCE = 2
MDIPC_WRITE_CDB_CHAIN = 3
MDIPC_RSP_SUCCESS = 0
MDIPC_RSP_FAIL = 1
MDIPC_RSP_NOTPRESENT = 2


class MDIPC_CHAN():
    def __init__(self, chan_index):
        self.index              = chan_index
        self.name               = MDIPC_BASE_NAME + str(chan_index)
        self.in_use             = False
        self.stat_already_in_use = 0
        self.msgID              = 0
        # these are local stats
        self.stat_num_msgs      = 0
        self.stat_num_success   = 0
        self.stat_num_fail      = 0
        self.stat_num_notpresent = 0
        self.stat_num_unknown    = 0
        self.stat_min_rsp_wait   = 999999
        self.stat_max_rsp_wait   = 0
        self.stat_long_rsp       = 0
        self.stat_num_timeouts   = 0
        
        try:
           self.fd = os.open(self.name, os.O_RDWR | os.O_NOFOLLOW | os.O_CLOEXEC)
           self.mm = mmap.mmap(self.fd, 0)
           logger.log_warning("MDIPC_CHAN: file {} size {} fd {} successfully opened".format(self.name, self.mm.size(), self.fd))

        except os.error:
           logger.log_error("MDIPC_CHAN: file error for {}".format(self.name))
           self.mm = None
           self.fd = None
           if (chan_index == 0):
              sys.exit("MDIPC_CHAN channel 0 doesn't exist")

    def get_fd(chan_index):
        return self.fd

class MDIPC():
    channels = []
    initialized = False
    lock_held = False
    signals_initialized = False

    def __init__(self):
        pid = os.getpid()
        tid = threading.get_native_id()
        if MDIPC.initialized is True:
            logger.log_warning("MDIPC ({} {}): {} channels already initialized".format(pid, tid, MDIPC_NUM_CHANNELS))
            return
        for x in range(0, MDIPC_NUM_CHANNELS):
            chan = MDIPC_CHAN(x)
            MDIPC.channels.append(chan)

        MDIPC.initialized = True
        self.stat_no_channel_avail = 0
        self.Tmutex = threading.RLock()
        logger.log_warning("MDIPC ({} {}): {} channels initialized".format(pid, tid, MDIPC_NUM_CHANNELS))
        if (pid == tid):
            self.install_sighandlers()

    def __del__(self):
        pid = os.getpid()
        tid = threading.get_native_id()
        self.dump_stats()
        logger.log_warning("MDIPC destruction ({} {})".format(pid, tid))

    def install_sighandlers(self):
        pid = os.getpid()
        tid = threading.get_native_id()
        if (self.signals_initialized):
            logger.log_warning("MDIPC ({} {}): sighandlers are already initialized!".format(pid, tid))
            return True
        if (pid == tid):
            self.sighandlers = [None] * signal.SIGSYS
            self.sighandlers[signal.SIGABRT] = signal.signal(signal.SIGABRT, self.cleanup_channels)
            self.sighandlers[signal.SIGINT] = signal.signal(signal.SIGINT, self.cleanup_channels)
            self.sighandlers[signal.SIGQUIT] = signal.signal(signal.SIGQUIT, self.cleanup_channels)
            self.sighandlers[signal.SIGTERM] = signal.signal(signal.SIGTERM, self.cleanup_channels)
            self.sighandlers[signal.SIGUSR1] = signal.signal(signal.SIGUSR1, self.dump_stats_signal)
            MDIPC.signals_initialized = True
            logger.log_warning("MDIPC ({} {}): sighandlers initialized".format(pid, tid))
            return True
        else:
            return False

    def sighandlers_installed(self):
        return MDIPC.signals_initialized

    def dump_stats(self):
        pid = os.getpid()
        tid = threading.get_native_id()
        for chan in MDIPC.channels:
            min_rsp_wait = chan.stat_min_rsp_wait
            if (min_rsp_wait == 999999):
                min_rsp_wait = -1 
            logger.log_warning("MDIPC ({} {}) channel {} local: msgs {} success {} fail {} notpresent {} unknown {} minrspwait {} maxrspwait {} long_rsp {} timeouts {} already_in_use {}".format(pid, tid, chan.index, chan.stat_num_msgs,
                chan.stat_num_success, chan.stat_num_fail, chan.stat_num_notpresent, chan.stat_num_unknown, min_rsp_wait, chan.stat_max_rsp_wait, chan.stat_long_rsp, chan.stat_num_timeouts, chan.stat_already_in_use))
        logger.log_warning("MDIPC ({} {}) no_channel_avail {} lock_held {}".format(pid, tid, self.stat_no_channel_avail, self.lock_held))

    def dump_stats_signal(self, signum, frame):
        logger.log_warning("MDIPC got USR1 signal")
        self.dump_stats()
        if (self.lock_held == True):
            self.Plock_release()

    def Plock_acquire(self):
        fcntl.flock(MDIPC.channels[0].fd, fcntl.LOCK_EX)
        self.lock_held = True

    def Plock_release(self):
        fcntl.flock(MDIPC.channels[0].fd, fcntl.LOCK_UN)
        self.lock_held = False

    def obtain_channel(self):
        pid = os.getpid()
        tid = threading.get_native_id()
        index = None
        self.Tmutex.acquire()        # thread protection
        self.Plock_acquire()         # process protection
        for chan in MDIPC.channels:
            own = int.from_bytes(chan.mm[0:4],sys.byteorder)
            if (own == MDIPC_OWN_NOS):
                ownerID = int.from_bytes(chan.mm[8:12],sys.byteorder)
                if (ownerID == 0):
                   if (chan.in_use == True):
                      msgID = int.from_bytes(chan.mm[4:8],sys.byteorder)
                      chan.stat_already_in_use += 1
                      logger.log_error("obtain_channel({} {}): got chan.in_use while allocating chan {} with last msgID {} : count {}".format(pid, tid, chan.index, msgID, chan.stat_already_in_use))
                   chan.mm[0:4] = MDIPC_OWN_NOS_PREP.to_bytes(4, sys.byteorder)
                   chan.mm[8:12] = tid.to_bytes(4, sys.byteorder)
                   chan.in_use = True
                   index = chan.index
                   break
                else:
                   msgID = int.from_bytes(chan.mm[4:8],sys.byteorder) 
                   logger.log_debug("obtain_channel({} {}): got free channel {} with ownerID {} and chan.in_use {}  own {} msgID {}".format(pid, tid, chan.index, ownerID, chan.in_use, hex(own), msgID))
                   # keep looking for an available channel for this thread
            else:
                msgID = int.from_bytes(chan.mm[4:8],sys.byteorder)
                logger.log_debug("obtain_channel({} {}) {} unavailable : own {} ownerID {} msgID {}".format(pid, tid, chan.index, hex(own), int.from_bytes(chan.mm[8:12],sys.byteorder), msgID))
        self.Plock_release()
        self.Tmutex.release()
        return index

    def free_channel(self, index, stat = None):
        pid = os.getpid()
        tid = threading.get_native_id()
        if (stat != None):
            # todo:  update client stats kept within the shared channel
           pass
        ownerID = int.from_bytes(MDIPC.channels[index].mm[8:12],sys.byteorder)
        if (ownerID != tid):
           own = int.from_bytes(MDIPC.channels[index].mm[0:4],sys.byteorder)
           msgID = int.from_bytes(MDIPC.channels[index].mm[4:8],sys.byteorder)
           logger.log_error("free_channel({} {}) chan {} has bogus ownerID {} with msgID {} and own {}".format(pid, tid, index, ownerID, msgID, hex(own)))
        ownerID = 0
        MDIPC.channels[index].in_use = False
        MDIPC.channels[index].mm[8:12] = ownerID.to_bytes(4, sys.byteorder)
        MDIPC.channels[index].mm[0:4] =  MDIPC_OWN_NOS.to_bytes(4, sys.byteorder)     # chan up for grabs again

    def cleanup_channels(self, signum, frame):
        pid = os.getpid()
        tid = threading.get_native_id()
        self.Tmutex.acquire()
        self.Plock_acquire()
        for chan in MDIPC.channels:
            own = int.from_bytes(chan.mm[0:4],sys.byteorder)
            msgID = int.from_bytes(chan.mm[4:8],sys.byteorder)
            ownerID = int.from_bytes(chan.mm[8:12],sys.byteorder)
            if (ownerID == tid):
               self.free_channel(chan.index)
               logger.log_warning("MDIPC termination handler({} {},{}): cleaning up channel {} with in-flight msgID {} and own {}".format(pid, tid, signum, chan.index, msgID, hex(own)))
        self.Plock_release()
        self.Tmutex.release()
        self.dump_stats()
        logger.log_warning("MDIPC termination handler({} {},{}): cleaned up".format(pid, tid, signum))
        # signal.signal(signum, self.sighandlers[signum])
        sys.exit()

    def msg_send(self, op, hw_port_id, page, offset, num_bytes, data=None):

        start_time = int(time.monotonic_ns() / 1000)
        index = self.obtain_channel()
        if (index is None):
            self.stat_no_channel_avail += 1
            logger.log_error("msg_send ({} {}): no free channel available!".format(os.getpid(), threading.get_native_id()))
            # caller = inspect.stack(0)
            # logger.log_error(" msg_send ({} {}): call stack is: {}".format(os.getpid(), threading.get_native_id(), caller))            
            return MDIPC_RSP_FAIL, None

        msg = memoryview(MDIPC.channels[index].mm)
        msgID = int.from_bytes(msg[4:8],sys.byteorder) + 1
        msg[4:8] = msgID.to_bytes(4, sys.byteorder)
        pid = os.getpid()
        tid = threading.get_native_id()
        msg[8:12] = tid.to_bytes(4, sys.byteorder)
        msg[12:16] = hw_port_id.to_bytes(4, sys.byteorder)
        msg[16:20] = op.to_bytes(4, sys.byteorder)
        msg[20:24] = page.to_bytes(4, sys.byteorder)
        msg[24:28] = offset.to_bytes(4, sys.byteorder)
        if (num_bytes > 128) and (page < 160):
            logger.log_error("msg_send ({},{} {}):  request with num_bytes {} truncated to 128!".format(index, pid, tid, num_bytes))
            logger.log_error("          op {} msgID {} hw_port_id {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
            num_bytes = 128
        msg[28:32] = num_bytes.to_bytes(4, sys.byteorder)
        msg[32:36] = msgID.to_bytes(4, sys.byteorder)       # status validation check

        if (op == MDIPC_WRITE) or (op == MDIPC_WRITE_CDB_CHAIN):
            if (data is None):
               logger.log_error("msg_send ({},{} {}): op {} requested but no data provided!".format(index, pid, tid, op))
               logger.log_error("          op {} msgID {} hw_port_id {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
               self.free_channel(index)
               return MDIPC_RSP_FAIL, None
            msg[36:(36+num_bytes)] = data[:num_bytes]
        elif (op == MDIPC_READ):
            pass
        elif (op == MDIPC_PRESENCE):
            pass
        else:
            logger.log_error("msg_send ({},{} {}): unknown op {} requested!".format(index, pid, tid, op))
            logger.log_error("          op {} msgID {} hw_port_id {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
            self.free_channel(index)
            return MDIPC_RSP_FAIL, None

        # over it goes 
        msg[0:4] = MDIPC_OWN_NDK.to_bytes(4, sys.byteorder)
        handoff_time = int(time.monotonic_ns() / 1000)
        MDIPC.channels[index].stat_num_msgs += 1

        # wait for response now...
        timed_out = False
        sleep_time = .001           # 1ms
        sleep_iters = 0
        while (timed_out != True):
            time.sleep(sleep_time)
            if (msg[0:4] == MDIPC_OWN_NOS_RSP.to_bytes(4, sys.byteorder)):
               break
            sleep_iters+=1
            if (sleep_iters > 3000):
               timed_out = True       

        done_time = int(time.monotonic_ns() / 1000)
        delta_time = done_time - handoff_time
        if (timed_out == True):
            MDIPC.channels[index].stat_num_timeouts += 1
            logger.log_error("msg_send ({},{} {}): timeout ({}/{})! own {} : starttime {} handofftime {} endtime {} rsptime(us) {}".format(index, pid, tid, sleep_iters, MDIPC.channels[index].stat_num_timeouts, hex(int.from_bytes(msg[0:4],sys.byteorder)), start_time, handoff_time, done_time, delta_time))
            logger.log_error("          op {} msgID {} index {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
            self.free_channel(index)
            return MDIPC_RSP_FAIL, None
        else:
            status = int.from_bytes(msg[32:36],sys.byteorder)
            if (delta_time >=  300000):
               MDIPC.channels[index].stat_long_rsp += 1
               # logger.log_warning("msg_send ({},{} {}): op {} msgID {} : index {} pg {} offset {} num_bytes {} : rsp status {} starttime {} handofftime {} endtime {} rsptime(us) {}".format(index, pid, tid, op, msgID, hw_port_id, page, offset, num_bytes, status, start_time, handoff_time, done_time, delta_time))
            
            ret_data = None
            if (status == MDIPC_RSP_SUCCESS):         # this also catches 'not present' responses to MDIPC_PRESENCE ops
               MDIPC.channels[index].stat_num_success += 1
               if (op == MDIPC_READ):
                   # logger.log_debug("          op {} msgID {} index {} pg {} offset {} num_bytes {} ret_data {}".format(op, msgID, hw_port_id, page, offset, num_bytes, bytearray(msg[36:(36+num_bytes)])))
                   # copy data to prevent continued peering directly into mmap window
                   ret_data = bytearray(msg[36:(36+num_bytes)])
               else:
                   # logger.log_debug("          op {} msgID {} index {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
                   pass
            elif (status == MDIPC_RSP_FAIL):
               if (op != MDIPC_PRESENCE):
                  MDIPC.channels[index].stat_num_fail += 1
               else:
                  MDIPC.channels[index].stat_num_success += 1       # this catches 'present' responses to MDIPC_PRESENCE ops
            elif (status == MDIPC_RSP_NOTPRESENT):
               MDIPC.channels[index].stat_num_notpresent += 1       # only for read/write ops that result in NOTPRESENT
            else:
               MDIPC.channels[index].stat_num_unknown += 1
            if (delta_time < MDIPC.channels[index].stat_min_rsp_wait) and (op != MDIPC_PRESENCE):
               MDIPC.channels[index].stat_min_rsp_wait = delta_time
               logger.log_debug("**** msg_send ({},{} {}): op {} msgID {} new minrspwait {}".format(index, pid, tid, op, msgID, delta_time))
            if (delta_time > MDIPC.channels[index].stat_max_rsp_wait):
               MDIPC.channels[index].stat_max_rsp_wait = delta_time
               logger.log_debug("**** msg_send ({},{} {}): op {} msgID {} new maxrspwait {}".format(index, pid, tid, op, msgID, delta_time))

        self.free_channel(index)
        return status, ret_data


# caching modes
CACHE_NORMAL  = 0
CACHE_FRESHEN = 1

class CachePage():
    def __init__(self, sfp_index, page, size, expiration = 2):
        self.sfp_index          = sfp_index
        self.page_num           = page
        self.cache_page_data =  bytearray(size)
        self.cache_page_valid   = False
        self.cache_page_ts      = 0
        self.cache_page0_admin  = False
        self.expiration         = expiration
        self.pending            = 0
        self.Tmutex = threading.RLock()

    def get_page_num(self):
        return self.page_num

    def cache_page_fresh(self):
        if (self.cache_page_valid is True):
            current_time = time.time()
            if (self.cache_page_ts != 0) and ((current_time - self.cache_page_ts) < self.expiration):
                return True
        self.cache_page_valid = False
        return False

    def cache_page(self, mode=CACHE_NORMAL, verbose=False):
        if (self.cache_page_fresh() == True):
            if (mode == CACHE_FRESHEN):
                self.cache_page_ts = time.time()
                if (verbose):
                    logger.log_warning("*** SFP{} freshened cached page {} new TS {}".format(self.sfp_index, self.page_num, self.cache_page_ts))
            return True

        self.Tmutex.acquire()        
        if (self.pending != 0):
            self.Tmutex.release()
            start_time = int(time.monotonic_ns() / 1000)
            logger.log_warning("*** ({} {}): SFP{} pg{} cache op is pending via {}".format(os.getpid(), threading.get_native_id(), self.sfp_index, self.page_num, self.pending))
            # wait for op to complete...
            timed_out = False
            sleep_time = .001           # 1ms
            sleep_iters = 0
            while (timed_out != True):
                time.sleep(sleep_time)
                if (self.pending == 0):
                    done_time = int(time.monotonic_ns() / 1000)
                    delta_time = done_time - start_time
                    logger.log_warning("*** ({} {}): SFP{} pg{} pending cache op completed in {}us {} sleepiters".format(os.getpid(), threading.get_native_id(), self.sfp_index, self.page_num, delta_time, sleep_iters))
                    return False
                sleep_iters+=1
                if (sleep_iters > 1000):
                   timed_out = True
            if (timed_out == True):
                logger.log_error("*** ({} {}): SFP{} pg{} pending cache op never completed via {}".format(os.getpid(), threading.get_native_id(), self.sfp_index, self.page_num, self.pending))
                return False

        self.pending = threading.get_native_id()
        self.Tmutex.release()

        offset = 128
        num_bytes = 128

        if (self.page_num == 0):
            offset = 0

        ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_READ, self.sfp_index, self.page_num, offset, num_bytes)       

        if (self.page_num == 0):
            if (ret != MDIPC_RSP_SUCCESS) or (data is None):
                logger.log_info("*** ({} {}): SFP{} cache page0 failed 1st chunk for offset {} num_bytes {} ret {}".format(os.getpid(), threading.get_native_id(), self.sfp_index, offset, num_bytes, ret))
                self.pending = 0
                return False

            self.cache_page_data[offset:128] = data
            if (verbose):
               logger.log_warning("*** ({} {}): SFP{} cached page0 1st chunk for offset {} num_bytes {} ret {}".format(os.getpid(), threading.get_native_id(), self.sfp_index, offset, num_bytes, ret))
               logger.log_warning("        raw bytes {}".format(self.cache_page_data[1:4]))
            offset = 128

            if (self.cache_page0_admin != True):
               ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_READ, self.sfp_index, self.page_num, offset, num_bytes)

               if (ret != MDIPC_RSP_SUCCESS) or (data is None):
                  logger.log_info("*** ({} {}): SFP{} cache page0 failed 2nd chunk for offset {} num_bytes {} ret {}".format(os.getpid(), threading.get_native_id(), self.sfp_index, offset, num_bytes, ret))
                  self.pending = 0
                  return False

               self.cache_page_data[offset:] = data
               self.cache_page0_admin = True
               if (verbose):
                  logger.log_warning("*** ({} {}): SFP{} cached page0 2nd chunk for offset {} num_bytes {} ret {}".format(os.getpid(), threading.get_native_id(), self.sfp_index, offset, num_bytes, ret))
                  logger.log_warning("        raw bytes {} {}".format(self.cache_page_data[1:10], self.cache_page_data[offset:offset+10]))
            elif (verbose):
               logger.log_warning("*** ({} {}): SFP{} skipping upper page0".format(os.getpid(), threading.get_native_id(), self.sfp_index))
               logger.log_warning("        raw bytes {} {}".format(self.cache_page_data[1:10], self.cache_page_data[offset:offset+10]))

        if (ret != MDIPC_RSP_SUCCESS) or (data is None):
            logger.log_info("*** SFP{} cache page {} failed for offset {} num_bytes {} ret {}".format(self.sfp_index, self.page_num, offset, num_bytes, ret))
            self.pending = 0
            return False

        self.cache_page_valid = True
        self.cache_page_ts = time.time()

        if (self.page_num != 0):
            self.cache_page_data = data

        self.pending = 0
        
        if (verbose):
            logger.log_warning("### ({} {}): SFP{} cached page {} num_bytes {} cache_page.len {}".format(os.getpid(), threading.get_native_id(), self.sfp_index, self.page_num, num_bytes, len(self.cache_page_data)))

        return True

    def page_flush(self):
        if (self.page_num == 0):
           self.cache_page0_admin = False
        self.cache_page_valid = False


class Sfp(SfpOptoeBase):
    """
    Nokia IXR-7250 Platform-specific Sfp class
    """

    instances = []
    MDIPC_hdl = None
    MDIPC_Initialized = False
    precache = False
    debug = False
    # hardwire for max 100 ports per IMM
    presence = RawArray('I', 100)
    sfp_event_live = RawValue('I', 0)
    initialized = [False] * 100

    # used by sfp_event to synchronize presence info
    @staticmethod
    def SfpHasBeenTransitioned(port, status):
        if (Sfp.sfp_event_live != True):
           logger.log_warning("SfpHasBeenTransitioned({} {}): sfp_event mechanism has gone live...".format(os.getpid(), threading.get_native_id()))
           Sfp.sfp_event_live = True

        for inst in Sfp.instances:
            if (inst.index == port):
                inst.page_cache_flush(True)
                lastPresence = Sfp.presence[inst.index]

                if (status == '0'):
                    Sfp.presence[inst.index] = False
                else:
                    Sfp.presence[inst.index] = True
                    if (lastPresence == False):
                       inst._xcvr_api = None
                       inst.get_xcvr_api()
                       logger.log_warning("SfpHasBeenTransitioned: Sfp index{} xcvr_api reinitialized".format(inst.index))

                logger.log_warning(
                    "SfpHasBeenTransitioned({} {}): invalidating_page_cache for Sfp index{} : status is {} lastPresence {}:{}".format(os.getpid(), threading.get_native_id(), inst.index, status, lastPresence, Sfp.presence[inst.index]))
                return
        logger.log_warning("SfpHasBeenTransitioned({} {}): no match for port index{}".format(os.getpid(), threading.get_native_id(), port))

    @staticmethod
    def SfpTypeToString(type):
        converter = {
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_INVALID:   "INVALID",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_SFP:       "SFP",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_SFP_PLUS:  "SFP+",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_SFP28:     "SFP28",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_SFP56:     "SFP56",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_QSFP:      "QSFP",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_QSFP_PLUS: "QSFP+",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_QSFP28:    "QSFP28",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_QSFP56:    "QSFP56",
            platform_ndk_pb2.RespSfpModuleType.SFP_MODULE_TYPE_QSFPDD:    "QSFPDD"
        }
        return converter.get(type, "INVALID")

    def __init__(self, index, sfp_type, stub):
        SfpOptoeBase.__init__(self)
        self.sfpi_obj = None
        self.sfpd_obj = None
        self.sfp_type = sfp_type

        self.index = index
        self._version_info = device_info.get_sonic_version_info()
        self.lastPresence = False
        self.cache_override_disable = True
        pid = os.getpid()
        tid = threading.get_native_id()
        # caller = inspect.stack(0)
        # logger.log_error(" SFP init ({} {}): index {} call stack is: {}".format(pid, tid, index, caller))

        if Sfp.MDIPC_hdl is None:
            Sfp.MDIPC_hdl = MDIPC()
            Sfp.MDIPC_Initialized = True
            # caller = inspect.stack(0)
            # logger.log_error(" SFP init index {} call stack is: {}".format(index, caller))
        else:
            if (Sfp.MDIPC_hdl.sighandlers_installed() == False):
                if (Sfp.MDIPC_hdl.install_sighandlers()):
                    logger.log_warning("SFP init ({} {}): MDIPC sighandlers initialized".format(pid, tid))
            logger.log_debug("SFP init ({} {}): Skipping MDIPC init".format(pid, tid))

        if (Sfp.initialized[index] == True):
            logger.log_error("SFP init ({} {}): Attempted reinitialization of index {} ignored".format(pid, tid, index))
            return

        Sfp.initialized[index] = True
        self.debug = False

        """        
        if (index==xx):
           self.debug = True
        """
        
        self.page_cache = []
        self.cache_page0 = CachePage(index, 0, 256)
        self.page_cache.append(self.cache_page0)        
        for page in range(1, 256):
            self.page_cache.append(CachePage(index, page, 128))

        self.write_cache = bytearray(2048)
        self.last_write_page = 0
        self.cdb_chain_pending = False

        self.stub = stub

        # set name
        self.name = self.SfpTypeToString(sfp_type) + '_' + str(index)
        logger.log_debug("Sfp __init__ index {} setting name to {}".format(index, self.name))
        Sfp.instances.append(self)
        # self.get_presence()

    def get_eeprom_path(self):
        return 'dummy'

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        """
        if (Sfp.sfp_event_live == True):
            # rely on SFP event subsystem notification of status change once it is running
            return bool(Sfp.presence[self.index])
        else:
            logger.log_debug("({} {}) getting MDIPC presence for SFP{}".format(os.getpid(), threading.get_native_id(), self.index))

        status, data = Sfp.MDIPC_hdl.msg_send(MDIPC_PRESENCE, self.index, 0, 0, 0)
        lastPresence = Sfp.presence[self.index]

        if (lastPresence != status):
            logger.log_warning("MDIPC ({} {}) get_presence status changed for SFP{} from {} to {}".format(os.getpid(), threading.get_native_id(), self.index, lastPresence, status))
            Sfp.presence[self.index] = status
            self.page_cache_flush(True)
            if (status) and (Sfp.precache):
                logger.log_warning("caching page0 for SFP{} due to lastPresence {}".format(self.index, status))
                self.cache_page0.cache_page()

        if (status):
            return True
        else:
            return False

    def get_cached_page(self, page):
        inst = self.page_cache[page]
        num = inst.get_page_num()
        if (num != page):
           logger.log_error("SFP{}: page_cache[{}] instance reports unmatched page {}".format(self.index, page, num))
           sys.exit("get_cached_page exception")
        if (inst.cache_page_fresh() == True):
           return inst.cache_page_data
        else:
           if (self.debug or Sfp.debug):
              logger.log_warning("*** SFP{}: page {} is stale".format(self.index, page))
           return None

    def page_cache_flush(self, hard = False, page = -1):
        if (page != -1):
            inst = self.page_cache[page]
            num = inst.get_page_num()
            if (num != page):
                logger.log_error("SFP{}: page_cache[{}] instance reports unmatched page {}".format(self.index, page, num))
                sys.exit("page_cache_flush exception")
            if (hard == True):
               inst.page_flush()
            else:
               inst.cache_page_valid = False
            if (self.debug or Sfp.debug):                        
                logger.log_warning("*** SFP{}: flushing page {} hard {} from cache".format(self.index, hard, inst.page_num))
            return
        # else
        logged_once = False
        for inst in self.page_cache:
            if (hard == True):
               inst.page_flush()
            else:
               inst.cache_page_valid = False
            if (self.debug or Sfp.debug):
                if (logged_once == False):                        
                    logger.log_warning("*** SFP{}: flushing all pages hard {} from cache".format(self.index, hard))
                    logged_once = True

    def get_name(self):
        """
        Retrieves the name of the sfp
        Returns : QSFP or QSFP+ or QSFP28
        """
        """
        identifier = None
        iface_data = self._get_eeprom_data('type')
        if (iface_data is not None):
            identifier = iface_data['data']['type']['value']
        else:
            return None
        identifier += str(self.index)

        return identifier
        """
        return self.name

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device.
        Returns:
            integer: The 1-based relative physical position in parent device or
                     -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True

    def _get_error_code(self):
        """
        Get error code of the SFP module

        Returns:
            The error code
        """
        return NotImplementedError

    def get_error_description(self):
        """
        Get error description

        Args:
            error_code: The error code returned by _get_error_code

        Returns:
            The error description
        """
        if not self.get_presence():
            error_description = self.SFP_STATUS_UNPLUGGED
        else:
            error_description = self.SFP_STATUS_OK

        return error_description
        # return NotImplementedError

    def get_reset_status(self):
        """
        Retrieves the reset status of this SFP

        Returns:
            A Boolean, True if our FPGA has the module latched in-reset, False if not
        """
        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_NORMAL

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.GetSfpResetStatus,
                                              platform_ndk_pb2.ReqSfpOpsPb(type=op_type, hw_port_id_begin=self.index))
        nokia_common.channel_shutdown(channel)
        if ret is False:
            return False
        status_msg = response.sfp_status

        return status_msg.status

    def get_status(self):
        """
        Retrieves the operational status of the device
        """
        reset = self.get_reset_status()

        if reset is True:
            status = False
        else:
            status = True

        return status

    def reset(self, leave_in_reset=0):
        """
        Reset SFP and return all user module settings to their default srate.
        """
        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_NORMAL

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.ReqSfpReset,
                                              platform_ndk_pb2.ReqSfpOpsPb(type=op_type,
                                                                           hw_port_id_begin=self.index,
                                                                           val=leave_in_reset))
        nokia_common.channel_shutdown(channel)
        self.page_cache_flush(True)

        if ret is False:
            return False
        status = response.sfp_status.status

        if status == 0:
            return True
        else:
            logger.log_error("reset Ethernet{} failed with status {}".format(self.index, status))
            return False

    def set_lpmode(self, lpmode):
        """
        Sets the lpmode (low power mode) of SFP
        """
        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_NORMAL

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.ReqSfpLPMode,
                                              platform_ndk_pb2.ReqSfpOpsPb(type=op_type,
                                                                           hw_port_id_begin=self.index,
                                                                           val=lpmode))
        nokia_common.channel_shutdown(channel)
        self.page_cache_flush(True)

        if ret is False:
            return False
        # status_msg = response.sfp_status
        
        return(True)
        # return status_msg.status

    def get_lpmode(self):
        """
        Retrieves the lpmode (low power mode) status of this SFP

        Returns:
            A Boolean, True if lpmode is enabled, False if disabled
        """
        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_NORMAL

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)

        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.GetSfpLPStatus,
                                              platform_ndk_pb2.ReqSfpOpsPb(type=op_type, hw_port_id_begin=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return False
        status_msg = response.sfp_status
        return status_msg.status


    """
    Direct control (optoe) SFP refactored api_factory compliance below

    def get_transceiver_status(self):
        tmpval = self.debug
        self.debug = False
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_status INVOKED for SFP{}!".format(self.index))
        self.cache_override_disable = True            
        transceiver_info_dict = super().get_transceiver_status()
        self.cache_override_disable = False
        self.debug = tmpval
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_status DONE for SFP{}!".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_info_dict))
        return transceiver_info_dict

    def get_transceiver_info(self):
        tmpval = self.debug    
        self.debug = False
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_info INVOKED for SFP{}!".format(self.index))
        self.cache_override_disable = True
        transceiver_info_dict = super().get_transceiver_info()
        self.cache_override_disable = False
        self.debug = tmpval
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_info DONE for SFP{}!".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_info_dict))
        return transceiver_info_dict

    def get_transceiver_bulk_status(self):
        tmpval = self.debug
        self.debug = False
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_bulk_status INVOKED for SFP{}!".format(self.index))
        self.cache_override_disable = True
        transceiver_bulk_dict = super().get_transceiver_bulk_status()
        self.cache_override_disable = False    
        self.debug = tmpval
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_bulk_status DONE for SFP{}!".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_bulk_dict))
        return transceiver_bulk_dict

    def get_transceiver_threshold_info(self):
        tmpval = self.debug
        self.debug = False
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_threshold_info INVOKED for SFP{}!".format(self.index))
        self.cache_override_disable = True
        transceiver_threshold_dict = super().get_transceiver_threshold_info()        
        self.cache_override_disable = False        
        self.debug = tmpval
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_threshold_info DONE for SFP{}!".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_threshold_dict))
        return transceiver_threshold_dict

    def get_transceiver_pm(self):
        tmpval = self.debug
        self.debug = False
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_pm INVOKED for SFP{}!".format(self.index))
        self.cache_override_disable = True
        transceiver_info_dict = super().get_transceiver_pm()
        self.cache_override_disable = False
        self.debug = tmpval
        if (Sfp.debug) or (self.debug):
            logger.log_warning("get_transceiver_pm DONE for SFP{}!".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_info_dict))
        return transceiver_info_dict
    """

    def smart_cache(self, page, offset, num_bytes):
        if (page < 256):
           num = self.page_cache[page].get_page_num()
           if (num != page):
              logger.log_error("page_cache[{}] instance shows unmatched page {} : possible corruption {}".format(page, num))
              sys.exit("smart_cache exception")
           if ((page == 0) and (offset > 2) and (offset+num_bytes < 129)) or (page == 3) or (page == 5) or ((page >= 16) and (page <= 18)) or (page == 23) or (page == 44) or (page >= 159):
              if (Sfp.debug) or (self.debug):
                 logger.log_warning("###   SFP{} smart_cache skipping page {} offset {} num_bytes {}".format(self.index, page, offset, num_bytes))
              return
           self.page_cache[page].cache_page(CACHE_NORMAL, self.debug)
        else:
           logger.log_error("###   SFP{} smart_cache page {} offset {} num_bytes {} out of range!".format(self.index, page, offset, num_bytes))

    def override_cache(self, page, offset, num_bytes):
        if (page == 0) and ((offset >= 3) and (offset <= 84)):
            return True
        elif (page == 17) and (offset == 128):
            return True

        return False   


    # ********************************************************************************
    # ********************************************************************************
    # ********************************************************************************
    #                        Module Direct IPC methods
    # ********************************************************************************
    # ********************************************************************************
    # ********************************************************************************

    def read_eeprom(self, offset, num_bytes):
        """
        read eeprom specfic bytes beginning from a random offset with size as num_bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be read

        Returns:
            bytearray, if raw sequence of bytes are read correctly from the offset of size num_bytes
            None, if the read_eeprom fails
        """

        # deconstruct page from offset
        if offset <= 255:
            page = 0
            page_offset = offset
        else:
            page = (offset//128) - 1
            page_offset = (offset%128) + 128

        if (num_bytes == 0):
            if (self.debug):
               logger.log_warning("read_eeprom got bad request for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(self.index, offset, num_bytes, page, page_offset))
            raw = bytes(0)
            return bytearray(raw)
            # return None

        self.smart_cache(page, page_offset, num_bytes)
        cached_page = self.get_cached_page(page)
        if (cached_page is not None) and (self.cache_override_disable is not True):
            if (self.override_cache(page,page_offset,num_bytes) is True):
                if (self.debug):
                    logger.log_warning("read_eeprom for SFP{} hit cached_page {}, but overriding due to offset {} num_bytes {}".format(self.index, page, page_offset, num_bytes))
                cached_page = None

        if (cached_page is None):
            ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_READ, self.index, page, page_offset, num_bytes)

            if (ret != MDIPC_RSP_SUCCESS):
                logger.log_info("read_eeprom failed with {} for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(ret, self.index, offset, num_bytes, page, page_offset))
                raw = bytes(0)
                # return bytearray(raw)
                return None
            elif (data is None):
                logger.log_info("read_eeprom failed (response data None) for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(self.index, offset, num_bytes, page, page_offset))
                raw = bytes(0)
                # return bytearray(raw)
                return None

            raw = data

            if (self.debug):
               logger.log_warning("### read_eeprom for SFP{} with offset {} and num_bytes {} : computed page {} offset {} raw.len {}".format(self.index, offset, num_bytes, page, page_offset, len(raw)))
               logger.log_warning("        raw bytes {}".format(bytes(raw)))
        else:
            if (page == 0):
                cache_offset = page_offset
                max_offset = 256
            else:
                cache_offset = page_offset - 128
                max_offset = 128

            if ((cache_offset + num_bytes) > max_offset):
                logger.log_error("read_eeprom cache hit for SFP{} with offset {} and num_bytes {} : computed page {} offset {} cache_offset {} cached_page.len {}".format(self.index, offset, num_bytes, page, page_offset, cache_offset, len(cached_page)))
                num_bytes = max_offset - cache_offset
                logger.log_error("        truncating num_bytes returned to {} to prevent page overrun".format(num_bytes))

            raw = bytes(cached_page[cache_offset:(cache_offset + num_bytes)])
            
            if (self.debug):
                logger.log_warning("read_eeprom cache hit for SFP{} with offset {} and num_bytes {} : computed page {} offset {} cache_offset {} cached_page.len {} raw.len {}".format(self.index, offset, num_bytes, page, page_offset, cache_offset, len(cached_page), len(raw)))
                logger.log_warning("        raw bytes {}".format(bytes(raw)))

        return bytearray(raw)

    def write_eeprom(self, offset, num_bytes, write_buffer):
        """
        write eeprom specfic bytes beginning from a random offset with size as num_bytes
        and write_buffer as the required bytes

        Args:
             offset :
                     Integer, the offset from which the read transaction will start
             num_bytes:
                     Integer, the number of bytes to be written
             write_buffer:
                     bytearray, raw bytes buffer which is to be written beginning at the offset

        Returns:
            a Boolean, true if the write succeeded and false if it did not succeed.
        """

        # deconstruct page from offset
        if offset <= 255:
            page = 0
            page_offset = offset
        else:
            page = (offset//128) - 1
            page_offset = (offset%128) + 128

        if (((page == 160) and (num_bytes == 128)) or ((page >= 161) and (page <= 175) and (num_bytes <= 128)) and (page_offset == 128)):
            if (page == 160) or (page == self.last_write_page + 1):

               delta_page = page - 160
               dst_offset = delta_page * 128
               self.write_cache[dst_offset:dst_offset + num_bytes] = write_buffer

               if (num_bytes < 128) or (page == 175):
                  total_bytes = (delta_page * 128) + num_bytes
                  if (self.debug or Sfp.debug):
                     l = len(write_buffer)
                     logger.log_warning("releasing CDB EPL chain for SFP{} at computed page {} num_bytes {} : l {} total_bytes {}".format(self.index, page, num_bytes, l, total_bytes))
                  self.cdb_chain_pending = False
                  self.last_write_page = 0
                  self.cdb_chain_stamp = 0

                  ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_WRITE_CDB_CHAIN, self.index, 160, 128, total_bytes, bytes(self.write_cache))
                  if (ret != MDIPC_RSP_SUCCESS):
                     logger.log_error("write_eeprom for CDB EPL chain failed with {} for SFP{} with page 160 offset 128 and total_bytes {} : computed page {} offset {} num_bytes {}".format(ret, self.index, total_bytes, page, page_offset, num_bytes))
                     return False

                  return True
               else:
                  if (self.debug or Sfp.debug):
                     logger.log_warning("caching CDB EPL op for SFP{} at computed page {} num_bytes {} : dst_offset {}".format(self.index, page, num_bytes, dst_offset))
                  self.cdb_chain_pending = True
                  self.last_write_page = page
                  self.cdb_chain_stamp = int(time.monotonic_ns() / 1000)
                  return True

        if (self.cdb_chain_pending):
           if (page != 159):
              now = int(time.monotonic_ns() / 1000)
              delta_time = now - self.cdb_chain_stamp
              if (delta_time > 2000000) or ((page >= 160) and (page <= 175)):
                  logger.log_error("write_eeprom timeout: discarding pending CDB EPL chain for SFP{} : offset {} and num_bytes {} : computed page {} offset {} : pending {}ms".format(self.index, offset, num_bytes, page, page_offset, delta_time))
                  self.cdb_chain_pending = False
                  self.last_write_page = 0
                  self.cdb_chain_stamp = 0
           else:
              delta_page = self.last_write_page - 160
              total_bytes = (delta_page * 128) + 128
              ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_WRITE_CDB_CHAIN, self.index, 160, 128, total_bytes, bytes(self.write_cache))
              if (ret != MDIPC_RSP_SUCCESS):
                    logger.log_error("write_eeprom deferred CDB EPL chain failed with {} for SFP{} and total_bytes {} : current op is computed page {} offset {} num_bytes {}".format(ret, self.index, total_bytes, page, page_offset, num_bytes))
              else:
                    if (self.debug or Sfp.debug):
                        logger.log_warning("write_eeprom deferred CDB EPL chain succeeded with {} for SFP{} and total_bytes {} : current op is computed page {} offset {} num_bytes {}".format(ret, self.index, total_bytes, page, page_offset, num_bytes))
              self.cdb_chain_pending = False
              self.last_write_page = 0
              self.cdb_chain_stamp = 0


        ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_WRITE, self.index, page, page_offset, num_bytes, bytes(write_buffer))

        if (ret != MDIPC_RSP_SUCCESS):
            if (self.debug or Sfp.debug):
               logger.log_warning("write_eeprom failed with {} for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(ret, self.index, offset, num_bytes, page, page_offset))
            return False

        if (self.debug or Sfp.debug):
            logger.log_warning("write_eeprom (status {}) for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(ret, self.index, offset, num_bytes, page, page_offset))
            if (page == 159):
                logger.log_warning("     data:    {}".format(bytes(write_buffer)))

        self.page_cache_flush()

        return True
