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
    from multiprocessing import Process, Lock, RawValue
    import mmap
    import os
    import sys


except ImportError as e:
    raise ImportError(str(e) + "- required module not found")
logger = Logger("sfp")

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
MDIPC_NUM_CHANNELS = 4

MDIPC_OWN_NDK = 0x5A5A3C3C
MDIPC_OWN_NOS = 0X43211234
MDIPC_READ  = 0
MDIPC_WRITE = 1
MDIPC_PRESENCE = 2
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
        self.stat_min_rsp_wait   = -1
        self.stat_max_rsp_wait   = 0
        self.stat_num_timeouts   = 0
        
        try:
           self.fd = os.open(self.name, os.O_RDWR | os.O_NOFOLLOW | os.O_CLOEXEC)
           self.mm = mmap.mmap(self.fd, 0)
           logger.log_warning("MDIPC_CHAN: file {} size {} fd {} successfully opened".format(self.name, self.mm.size(), self.fd))
        except os.error:
           logger.log_error("MDIPC_CHAN: file error for {}".format(self.name))

class MDIPC():
    channels = []
    initialized = False
    def __init__(self):
        pid = os.getpid()
        if MDIPC.initialized is True:
            logger.log_warning("MDIPC ({}): {} channels already initialized".format(pid, MDIPC_NUM_CHANNELS)) 
            return
        for x in range(0, MDIPC_NUM_CHANNELS):
            chan = MDIPC_CHAN(x)
            MDIPC.channels.append(chan)
        MDIPC.initialized = True
        self.stat_no_channel_avail = 0
        self.mutex = Lock()
        logger.log_warning("MDIPC ({}): {} channels initialized".format(pid, MDIPC_NUM_CHANNELS))        
    
    def dump_stats(self):
        pid = os.getpid()
        for chan in MDIPC.channels:
            logger.log_warning("MDIPC ({}) channel {} local: msgs {} success {} fail {} notpresent {} unknown {} minrspwait {} maxrspwait {} timeouts {} already_in_use {}".format(pid, chan.index, chan.stat_num_msgs,
                chan.stat_num_success, chan.stat_num_fail, chan.stat_num_notpresent, chan.stat_num_unknown, chan.stat_min_rsp_wait, chan.stat_max_rsp_wait, chan.stat_num_timeouts, chan.stat_already_in_use))
        logger.log_warning("     no_channel_avail {}".format(self.stat_no_channel_avail))

    def obtain_channel(self):
        pid = os.getpid()
        index = None
        self.mutex.acquire()
        for chan in MDIPC.channels:
            own = int.from_bytes(chan.mm[0:4],sys.byteorder)
            if (own == MDIPC_OWN_NOS):
                # process protection
                ownerID = int.from_bytes(chan.mm[8:12],sys.byteorder)
                if (ownerID == 0):
                   chan.mm[8:12] = pid.to_bytes(4, sys.byteorder)
                   index = chan.index
                   break
                else:
                   msgID = int.from_bytes(chan.mm[4:8],sys.byteorder) 
                   logger.log_error("obtain_channel({}): got free channel {} with ownerID {} and chan.in_use {}!  own {} msgID {}".format(pid, chan.index, ownerID, chan.in_use, own, msgID))
                   if (ownerID == pid):         # another thread already has it
                      chan.stat_already_in_use += 1
                      if (chan.in_use == False):
                         chan.in_use = True
                         # index = chan.index
                         # break
                         #
                         # keep looking for an available channel for this thread
                      else:
                         logger.log_error("obtain_channel({}): got free channel {} with matching pid {} already in use {}!  own {} msgID {}".format(pid, chan.index, ownerID, chan.in_use, own, msgID))
            else:
                msgID = int.from_bytes(chan.mm[4:8],sys.byteorder)
                logger.log_debug("obtain_channel({}) {} unavailable : own {} ownerID {} msgID {}".format(pid, chan.index, hex(own), int.from_bytes(chan.mm[8:12],sys.byteorder), msgID))
        self.mutex.release()
        return index

    def free_channel(self, index, stat = None):
        if (stat != None):
            # todo:  update client stats kept within the shared channel
           pass
        ownerID = 0
        MDIPC.channels[index].mm[8:12] = ownerID.to_bytes(4, sys.byteorder)           # ownerID: up for process grabs again
        MDIPC.channels[index].in_use = False               # no local threads using it either

    def msg_send(self, op, hw_port_id, page, offset, num_bytes, data=None):

        start_time = int(time.monotonic_ns() / 1000)
        index = self.obtain_channel()
        if (index is None):
            self.stat_no_channel_avail += 1
            logger.log_error("msg_send ({}): no free channel available!".format(os.getpid()))
            return MDIPC_RSP_FAIL, None

        msg = memoryview(MDIPC.channels[index].mm)
        msgID = int.from_bytes(msg[4:8],sys.byteorder) + 1
        msg[4:8] = msgID.to_bytes(4, sys.byteorder)
        pid = os.getpid()
        msg[8:12] = pid.to_bytes(4, sys.byteorder)
        msg[12:16] = hw_port_id.to_bytes(4, sys.byteorder)
        msg[16:20] = op.to_bytes(4, sys.byteorder)
        msg[20:24] = page.to_bytes(4, sys.byteorder)
        msg[24:28] = offset.to_bytes(4, sys.byteorder)
        if (num_bytes > 128):        
            logger.log_error("msg_send ({},{}): op {} requested with num_bytes {} truncated to 128!".format(index, pid, num_bytes))
            logger.log_error("          op {} msgID {} index {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
            num_bytes = 128
        msg[28:32] = num_bytes.to_bytes(4, sys.byteorder)
        msg[32:36] = msgID.to_bytes(4, sys.byteorder)       # status validation check

        if (op == MDIPC_WRITE):
            if (data is None):
               logger.log_error("msg_send ({},{}): MDIPC_WRITE op requested but no data provided!".format(index, pid))
               logger.log_error("          op {} msgID {} index {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
               self.free_channel(index)
               return MDIPC_RSP_FAIL, None
            msg[36:(36+num_bytes)] = data
        elif (op == MDIPC_READ):
            pass
        elif (op == MDIPC_PRESENCE):
            pass
        else:
            logger.log_error("msg_send ({},{}): unknown op {} requested!".format(index, pid, op))
            logger.log_error("          op {} msgID {} index {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
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
            if (msg[0:4] == MDIPC_OWN_NOS.to_bytes(4, sys.byteorder)):
               break
            sleep_iters+=1
            if (sleep_iters > 2500):
               timed_out = True       

        done_time = int(time.monotonic_ns() / 1000)
        delta_time = done_time - handoff_time
        if (timed_out == True):
            MDIPC.channels[index].stat_num_timeouts += 1
            logger.log_error("msg_send ({},{}): timeout ({}/{})! own {} : starttime {} handofftime {} endtime {} rsptime(us) {}".format(index, pid, sleep_iters, MDIPC.channels[index].stat_num_timeouts, hex(int.from_bytes(msg[0:4],sys.byteorder)), start_time, handoff_time, done_time, delta_time))
            logger.log_error("          op {} msgID {} index {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
            self.free_channel(index)
            return MDIPC_RSP_FAIL, None
        else:
            status = int.from_bytes(msg[32:36],sys.byteorder)
            if (delta_time >=  200000):
               logger.log_warning("msg_send ({},{}): op {} msgID {} : index {} pg {} offset {} num_bytes {} : rsp status {} starttime {} handofftime {} endtime {} rsptime(us) {}".format(index, pid, op, msgID, hw_port_id, page, offset, num_bytes, status, start_time, handoff_time, done_time, delta_time))
            
            if (status == MDIPC_RSP_SUCCESS):
               MDIPC.channels[index].stat_num_success += 1
               if (op == MDIPC_READ):
                   # logger.log_debug("          op {} msgID {} index {} pg {} offset {} num_bytes {} ret_data {}".format(op, msgID, hw_port_id, page, offset, num_bytes, bytearray(msg[36:(36+num_bytes)])))
                   pass
               else:
                   # logger.log_debug("          op {} msgID {} index {} pg {} offset {} num_bytes {}".format(op, msgID, hw_port_id, page, offset, num_bytes))
                   pass
            elif (status == MDIPC_RSP_FAIL):
               MDIPC.channels[index].stat_num_fail += 1
            elif (status == MDIPC_RSP_NOTPRESENT):
               MDIPC.channels[index].stat_num_notpresent += 1
            else:
               MDIPC.channels[index].stat_num_unknown += 1
            if (delta_time < MDIPC.channels[index].stat_min_rsp_wait):
               MDIPC.channels[index].stat_min_rsp_wait = delta_time
               logger.log_warning("**** msg_send ({},{}): msgID {} new minrspwait {}".format(index, pid, msgID, delta_time))
            if (delta_time > MDIPC.channels[index].stat_max_rsp_wait):
               MDIPC.channels[index].stat_max_rsp_wait = delta_time
               logger.log_warning("**** msg_send ({},{}): msgID {} new maxrspwait {}".format(index, pid, msgID, delta_time))

        if (op == MDIPC_READ):
             # copy data to prevent continued peering directly into mmap window
            ret_data = bytearray(msg[36:(36+num_bytes)])
        else:
            ret_data = None
        self.free_channel(index)
        return status, ret_data


# caching modes
CACHE_NORMAL  = 0
CACHE_FRESHEN = 1

class CachePage():
    def __init__(self, sfp_index, page, size, expiration = 5):
        self.sfp_index          = sfp_index
        self.page_num           = page
        self.cache_page_data    = bytes([0] * size)
        self.cache_page_valid   = False
        self.cache_page_ts      = 0
        self.expiration         = expiration

    def cache_page_fresh(self):
        if (self.cache_page_valid is True):
            current_time = time.time()
            if (self.cache_page_ts != 0) and ((current_time - self.cache_page_ts) < self.expiration):
                return True
        self.cache_page_valid = False
        return False

    def cache_page(self, mode=CACHE_NORMAL):
        if (self.cache_page_fresh() == True):
            if (mode == CACHE_FRESHEN):
                self.cache_page_ts = time.time()
                logger.log_debug("*** SFP{} freshened cached page {} new TS {}".format(self.sfp_index, self.page_num, self.cache_page_ts))
            return True

        offset = 128
        num_bytes = 128

        if (self.page_num == 0):
            offset = 0

        ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_READ, self.sfp_index, self.page_num, offset, num_bytes)       

        if (self.page_num == 0):
            if (ret != MDIPC_RSP_SUCCESS) or (data is None):
                logger.log_error("*** SFP{} cache page0 failed 1st chunk for offset {} num_bytes {} ret {}".format(self.sfp_index, offset, num_bytes, ret))
                return False
            temp_cache = data
            offset = 128

            ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_READ, self.sfp_index, self.page_num, offset, num_bytes)

            if (ret != MDIPC_RSP_SUCCESS) or (data is None):
                logger.log_error("*** SFP{} cache page0 failed 2nd chunk for offset {} num_bytes {} ret {}".format(self.sfp_index, offset, num_bytes, ret))
                return False

            self.cache_page_data = temp_cache + data

        if (ret != MDIPC_RSP_SUCCESS) or (data is None):
            logger.log_error("*** SFP{} cache page {} failed for offset {} num_bytes {} ret {}".format(self.sfp_index, self.page_num, offset, num_bytes, ret))
            return False

        self.cache_page_valid = True
        self.cache_page_ts = time.time()

        if (self.page_num != 0):
            self.cache_page_data = data
        # logger.log_error("*** SFP{} cached page {} num_bytes {} cache_page.len {}".format(self.sfp_index, self.page_num, num_bytes, len(self.cache_page_data)))

        return True

    def page_flush(self):
        self.cache_page_valid = False


class Sfp(SfpOptoeBase):
    """
    Nokia IXR-7250 Platform-specific Sfp class
    """

    instances = []
    MDIPC_hdl = None
    MDIPC_Initialized = False

    # used by sfp_event to synchronize presence info
    @staticmethod
    def SfpHasBeenTransitioned(port, status):
        for inst in Sfp.instances:
            if (inst.index == port):
                inst.page_cache_flush()
                lastPresence = inst.lastPresence.value
                if (status == '0'):
                    inst.lastPresence.value = False
                else:
                    inst.lastPresence.value = True
                logger.log_warning(
                    "SfpHasBeenTransitioned({}): invalidating_page_cache for Sfp index{} : status is {} lastPresence {}:{}".format(os.getpid(), inst.index, status, lastPresence, inst.lastPresence.value))
                return
        logger.log_warning("SfpHasBeenTransitioned({}): no match for port index{}".format(os.getpid(), port))

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
        self.sfp_type = None

        self.index = index
        self._version_info = device_info.get_sonic_version_info()
        self.lastPresence = RawValue('i', False)

        if Sfp.MDIPC_hdl is None:
            Sfp.MDIPC_hdl = MDIPC()
            Sfp.MDIPC_Initialized = True

        self.page_cache = []
        self.cache_page0 = CachePage(index, 0, 256)
        self.page_cache.append(self.cache_page0)
        self.cache_page1 = CachePage(index, 1, 128)
        self.page_cache.append(self.cache_page1)
        self.cache_page2 = CachePage(index, 2, 128)
        self.page_cache.append(self.cache_page2)
        self.cache_page16 = CachePage(index, 16, 128)
        self.page_cache.append(self.cache_page16)
        self.cache_page17 = CachePage(index, 17, 128)
        self.page_cache.append(self.cache_page17)
        self.cache_override_disable = False

        self.stub = stub

        # set name
        self.name = self.SfpTypeToString(sfp_type) + '_' + str(index)
        logger.log_debug("Sfp __init__ index {} setting name to {}".format(index, self.name))
        Sfp.instances.append(self)
        self.get_presence(True)


    def get_cached_page(self, page):
        for inst in self.page_cache:
            if (inst.page_num == page):
                if (inst.cache_page_fresh() == True):
                   return inst.cache_page_data
        # logger.log_debug("*** SFP{}: no page {} available in cache".format(self.index, page))
        return None

    def page_cache_flush(self, page = -1):
        for inst in self.page_cache:
            if (page == -1) or (inst.page_num == page):
                inst.cache_page_valid = False
                # logger.log_debug("*** SFP{}: flushing page {} from cache".format(self.index, page))

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
        self.page_cache_flush()

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
        self.page_cache_flush()

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

    def get_presence(self, first_time=False):
        """
        Retrieves the presence of the sfp
        """

        if (first_time != True):
            # wait for SFP event subsystem notification of status change after initial get
            return self.lastPresence.value
        else:
            logger.log_warning("({}) getting MDIPC presence for SFP{}".format(os.getpid(), self.index))
        
        """
        # NDK presence
        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_REQ_PRESENCE

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.GetSfpPresence,
                                              platform_ndk_pb2.ReqSfpOpsPb(type=op_type, hw_port_id_begin=self.index))
        nokia_common.channel_shutdown(channel)
        
        # logger.log_warning("get_presence status for SFP{} is {}".format(self.index, response.sfp_status.status))

        if ret is False:
            return False
        status = response.sfp_status.status
        
        if (self.lastPresence != status_msg):
            logger.log_info("get_presence status changed for SFP {} to {}".format(self.index, status_msg))
            self.lastPresence = status_msg
            # really only need to do this on transition to present, but since it's not an expensive operation...
            self.invalidate_page_cache(ALL_PAGES_TYPE)

        return status_msg.status
        """

        status, data = Sfp.MDIPC_hdl.msg_send(MDIPC_PRESENCE, self.index, 0, 0, 0)
        lastPresence = self.lastPresence.value

        if (lastPresence != status):
            logger.log_warning("({}) get_presence status changed for SFP{} from {} to {}".format(os.getpid(), self.index, lastPresence, status))
            self.lastPresence.value = status
            
            self.page_cache_flush()
            if (status):
                logger.log_warning("caching page0 for SFP{} due to lastPresence {}".format(self.index, status))
                self.cache_page0.cache_page()

        if (status):
            return True
        else:
            return False

    """
    Direct control (optoe) SFP refactored api_factory compliance below
    """

    def get_transceiver_info(self):
        # logger.log_debug("get_transceiver_info INVOKED for SFP{}!".format(self.index))
        self.cache_page0.cache_page()
        self.cache_page1.cache_page()
        self.cache_override_disable = True
        transceiver_info_dict = super().get_transceiver_info()
        self.cache_override_disable = False
        # logger.log_debug("get_transceiver_info DONE for SFP{}".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_info_dict))
        return transceiver_info_dict

    def get_transceiver_bulk_status(self):
        # logger.log_debug("get_transceiver_bulk_status INVOKED for SFP{}!".format(self.index))
        self.cache_page0.cache_page()
        self.cache_page1.cache_page()
        self.cache_page2.cache_page()
        self.cache_page17.cache_page()
        self.cache_override_disable = True
        transceiver_bulk_dict = super().get_transceiver_bulk_status()
        self.cache_override_disable = False
        # logger.log_debug("get_transceiver_bulk_status DONE for SFP{}".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_bulk_dict))
        return transceiver_bulk_dict

    def get_transceiver_threshold_info(self):
        # logger.log_debug("get_transceiver_threshold_info INVOKED for SFP{}!".format(self.index))
        self.cache_page0.cache_page()
        self.cache_page1.cache_page()
        self.cache_page2.cache_page()
        self.cache_override_disable = True
        transceiver_threshold_dict = super().get_transceiver_threshold_info()
        self.page_cache_flush()
        self.cache_override_disable = False
        # logger.log_debug("get_transceiver_threshold_info DONE for SFP{}".format(self.index))
        # logger.log_debug("      SFP{} returned {}".format(self.index, transceiver_threshold_dict))
        return transceiver_threshold_dict

    def smart_cache(self, page, offset):
        hit = False
        if (page == 0) and ((offset == 85) or (offset == 86) or (offset == 2)):
           self.cache_page0.cache_page()
           hit = True
        if (page == 1) and (offset == 176):
           self.cache_page1.cache_page()
           hit = True           
        if (page == 16) and (offset == 145):
           self.cache_page16.cache_page()
           hit = True
        if (hit is True):
           # logger.log_debug("   SFP{} smart_cache cached page {} due to offset {}".format(self.index, page, offset))
           pass

    def override_cache(self, page, offset):
        if (page == 0) and ((offset >= 3) and (offset <= 84)):
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

        self.smart_cache(page, page_offset)
        cached_page = self.get_cached_page(page)
        if (cached_page is not None) and (self.cache_override_disable is not True):
            if (self.override_cache(page,page_offset) is True):
                # logger.log_warning("read_eeprom for SFP{} hit cached_page {}, but overriding due to offset {} num_bytes {}".format(self.index, page, page_offset, num_bytes))
                cached_page = None            

        if (cached_page is None):
            ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_READ, self.index, page, page_offset, num_bytes)

            if (ret != MDIPC_RSP_SUCCESS):
                logger.log_error("read_eeprom failed with {} for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(ret, self.index, offset, num_bytes, page, page_offset))
                return None

            if (data is None):
                logger.log_error("read_eeprom failed (response data None) for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(self.index, offset, num_bytes, page, page_offset))
                return None

            # raw = bytearray(response.data)
            raw = data

            # logger.log_debug("read_eeprom for SFP{} with offset {} and num_bytes {} : computed page {} offset {} raw.len {}".format(self.index, offset, num_bytes, page, page_offset, len(raw)))
            # logger.log_debug("        raw bytes {}".format(raw))
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
            
            # logger.log_debug("read_eeprom cache hit for SFP{} with offset {} and num_bytes {} : computed page {} offset {} cache_offset {} cached_page.len {} raw.len {}".format(self.index, offset, num_bytes, page, page_offset, cache_offset, len(cached_page), len(raw)))
            # logger.log_debug("        raw bytes {}".format(raw))

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

        ret, data = Sfp.MDIPC_hdl.msg_send(MDIPC_WRITE, self.index, page, page_offset, num_bytes, bytes(write_buffer))

        if (ret != MDIPC_RSP_SUCCESS):
            logger.log_error("Failed write_eeprom with {} for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(ret, self.index, offset, num_bytes, page, page_offset))
            return False

        self.page_cache_flush(page)
        # logger.log_debug("write_eeprom (status {}) for SFP{} with offset {} and num_bytes {} : computed page {} offset {}".format(ret, self.index, offset, num_bytes, page, page_offset))
        return True
