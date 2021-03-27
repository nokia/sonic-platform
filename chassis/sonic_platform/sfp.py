# Name: sfp.py, version: 1.0
#
# Description: Module contains the definitions of SFP related APIs
# for Nokia IXR 7250 platform.
#
# Copyright (c) 2019, Nokia
# All rights reserved.
#

try:
    from sonic_platform_base.sfp_base import SfpBase
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436InterfaceId
    from sonic_platform_base.sonic_sfp.sff8436 import sff8436Dom
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_InterfaceId
    from sonic_platform_base.sonic_sfp.qsfp_dd import qsfp_dd_Dom
    from platform_ndk import nokia_common
    from platform_ndk import platform_ndk_pb2
    from sonic_py_common.logger import Logger

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


ALL_PAGES_TYPE = 99

STATUS_CONTROL_OFFSET = 110
STATUS_CONTROL_WIDTH = 1
TX_DISABLE_HARD_BIT = 7
TX_DISABLE_SOFT_BIT = 6

"""
INFO_OFFSET = 128
DOM_OFFSET = 0
DOM_OFFSET1 = 384
"""

cable_length_tup = ('LengthSMFkm-UnitsOfKm', 'LengthSMF(UnitsOf100m)',
                    'Length50um(UnitsOf10m)', 'Length62.5um(UnitsOfm)',
                    'LengthCable(UnitsOfm)', 'LengthOM3(UnitsOf10m)')

compliance_code_tup = ('10GEthernetComplianceCode', 'InfinibandComplianceCode',
                       'ESCONComplianceCodes', 'SONETComplianceCodes',
                       'EthernetComplianceCodes', 'FibreChannelLinkLength',
                       'FibreChannelTechnology', 'SFP+CableTechnology',
                       'FibreChannelTransmissionMedia', 'FibreChannelSpeed')

qsfp_cable_length_tup = ('Length(km)', 'Length OM3(2m)', 'Length OM2(m)',
                         'Length OM1(m)', 'Length Cable Assembly(m)')

qsfp_compliance_code_tup = (
    '10/40G Ethernet Compliance Code',
    'SONET Compliance codes',
    'SAS/SATA compliance codes',
    'Gigabit Ethernet Compliant codes',
    'Fibre Channel link length/Transmitter Technology',
    'Fibre Channel transmission media',
    'Fibre Channel Speed')


info_dict_keys = ['type', 'hardwarerev', 'serialnum',
                  'manufacturename', 'modelname', 'Connector',
                  'vendor_date', 'vendor_oui', 'power', 'cable_length',
                  'revision_compliance', 'flatmem', 'fw_revision',
                  'app_advertising', 'supported_link_length', 'type_abbrv_name']

dom_dict_keys = ['rx_los',       'tx_fault',   'reset_status',
                 'power_lpmode', 'tx_disable', 'tx_disable_channel',
                 'temperature',  'voltage',    'rx1power',
                 'rx2power',     'rx3power',   'rx4power',
                 'tx1bias',      'tx2bias',    'tx3bias',
                 'tx4bias',      'tx1power',   'tx2power',
                 'tx3power',     'tx4power']

threshold_dict_keys = ['temphighalarm',    'temphighwarning',
                       'templowalarm',     'templowwarning',
                       'vcchighalarm',     'vcchighwarning',
                       'vcclowalarm',      'vcclowwarning',
                       'rxpowerhighalarm', 'rxpowerhighwarning',
                       'rxpowerlowalarm',  'rxpowerlowwarning',
                       'txpowerhighalarm', 'txpowerhighwarning',
                       'txpowerlowalarm',  'txpowerlowwarning',
                       'txbiashighalarm',  'txbiashighwarning',
                       'txbiaslowalarm',   'txbiaslowwarning']

QSFP_INFO_STRUCT = 1
QSFP_DOM_STRUCT = 2

FUNC_ABSENT = ''

sff8436_parser = {
    'compliance':       [CTRL_TYPE,   1,  2, FUNC_ABSENT],
    'reset_status':     [CTRL_TYPE,   2,  1, 'parse_dom_status_indicator'],
    'reset_status_new': [CTRL_TYPE,   0,  9, FUNC_ABSENT],
    'rx_los':           [CTRL_TYPE,   3,  1, FUNC_ABSENT],
    'tx_fault':         [CTRL_TYPE,   4,  1, FUNC_ABSENT],
    'tx_disable':       [CTRL_TYPE,  86,  1, FUNC_ABSENT],
    'power_lpmode':     [CTRL_TYPE,  93,  1, 'parse_dom_power_control'],
    'power_override':   [CTRL_TYPE,  93,  1, 'parse_dom_power_control'],
    'Temperature':      [CTRL_TYPE,  22,  2, 'parse_temperature'],
    'dom_bulk_data':    [CTRL_TYPE,  22, 36, FUNC_ABSENT],
    'Voltage':          [CTRL_TYPE,  26,  2, 'parse_voltage'],
    'ChannelMonitor':   [CTRL_TYPE,  34, 16, 'parse_channel_monitor_params'],
    'ChannelMonitorwtxpwr': [CTRL_TYPE,  34, 24, 'parse_channel_monitor_params_with_tx_power'],
    'cable_type':       [INFO_TYPE, -1, -1, 'parse_sfp_info_bulk'],
    'cable_length':     [INFO_TYPE, -1, -1, 'parse_sfp_info_bulk'],
    'Connector':        [INFO_TYPE,  0, 20, 'parse_sfp_info_bulk'],
    'type':             [INFO_TYPE,  0, 20, 'parse_sfp_info_bulk'],
    'encoding':         [INFO_TYPE,  0, 20, 'parse_sfp_info_bulk'],
    'ext_identifier':   [INFO_TYPE,  0, 20, 'parse_sfp_info_bulk'],
    'ext_rateselect_compliance': [INFO_TYPE,  0, 20, 'parse_sfp_info_bulk'],
    'nominal_bit_rate': [INFO_TYPE,  0, 20, 'parse_sfp_info_bulk'],
    'specification_compliance': [INFO_TYPE,  0, 20, 'parse_sfp_info_bulk'],
    'type_abbrv_name':  [INFO_TYPE,  0,  20, 'parse_sfp_info_bulk'],
    'manufacturename':  [INFO_TYPE, 20, 16, 'parse_vendor_name'],
    'vendor_oui':       [INFO_TYPE,  37, 3, 'parse_vendor_oui'],
    'modelname':        [INFO_TYPE, 40, 16, 'parse_vendor_pn'],
    'hardwarerev':      [INFO_TYPE, 56,  2, 'parse_vendor_rev'],
    'options':          [INFO_TYPE, 64,  4, 'parse_option_params', QSFP_DOM_STRUCT],
    'ext_spec_compliance': [INFO_TYPE, 64,  1, 'parse_ext_specification_compliance'],
    'serialnum':        [INFO_TYPE, 68, 16, 'parse_vendor_sn'],
    'vendor_date':      [INFO_TYPE, 84,  8, 'parse_vendor_date'],
    'dom_monitor_type': [INFO_TYPE, 92,  2, 'parse_qsfp_dom_capability'],
    'ModuleThreshold':  [EXT_CTRL_TYPE, 0, 24, 'parse_module_threshold_values'],
    'ChannelThreshold': [EXT_CTRL_TYPE, 48, 24, 'parse_channel_threshold_values'],
}


DD_INFO_STRUCT = 1
DD_DOM_STRUCT = 2

qsfpdd_parser = {
    'RevisionCompliance': [CTRL_TYPE,  1, 1, 'parse_rev_compliance', DD_INFO_STRUCT],
    'summary byte2':      [CTRL_TYPE,  2, 1, 'parse_summary_byte_2', DD_INFO_STRUCT],
    'dom_monitor_type':   [CTRL_TYPE,  2, 1, 'parse_qsfp_dom_capability', DD_INFO_STRUCT],
    'FW version':         [CTRL_TYPE,  39, 2, 'parse_FW_rev', DD_INFO_STRUCT],
    'temperature':        [CTRL_TYPE,  14,  2, 'parse_temperature', DD_DOM_STRUCT],
    'voltage':            [CTRL_TYPE,  16,  2, 'parse_voltage', DD_DOM_STRUCT],
    'reset_status':       [CTRL_TYPE,  26,  1, 'parse_dom_channel_status', DD_DOM_STRUCT],
    'media_type':         [CTRL_TYPE,  85,  1, 'parse_media_type', DD_INFO_STRUCT],

    'first_app_list':     [CTRL_TYPE,  86, 32, FUNC_ABSENT, DD_INFO_STRUCT],

    'ChannelMonitor':     [CTRL_TYPE,  100, 6, 'parse_channel_monitor_params', DD_DOM_STRUCT],
    'App advertising':    [CTRL_TYPE,  85, 33, 'parse_app_advertising', DD_INFO_STRUCT],

    'type':               [INFO_TYPE,  0, 1, 'parse_sfp_type', DD_INFO_STRUCT],
    'ext_iden':           [INFO_TYPE,  72, 2, 'parse_ext_iden', DD_INFO_STRUCT],
    'cable_length':       [INFO_TYPE,  74, 1, 'parse_cable_len', DD_INFO_STRUCT],
    'Connector':          [INFO_TYPE,  75, 1, 'parse_connector', DD_INFO_STRUCT],

    'manufacturename':    [INFO_TYPE,  1, 16, 'parse_vendor_name', DD_INFO_STRUCT],
    'vendor_oui':         [INFO_TYPE, 17,  3, 'parse_vendor_oui', DD_INFO_STRUCT],
    'modelname':          [INFO_TYPE, 20, 16, 'parse_vendor_pn', DD_INFO_STRUCT],
    'hardwarerev':        [INFO_TYPE, 36,  2, 'parse_vendor_rev', DD_INFO_STRUCT],
    'serialnum':          [INFO_TYPE, 38, 16, 'parse_vendor_sn', DD_INFO_STRUCT],
    'vendor_date':        [INFO_TYPE, 54,  8, 'parse_vendor_date', DD_INFO_STRUCT],
    'MIT':                [INFO_TYPE, 84,  1, 'parse_MIT', DD_INFO_STRUCT],
    'Supported Link Length': [UPPER_PAGE_TYPE,  4,  6, 'parse_supported_link_lengths', DD_INFO_STRUCT, 1],
    'Module_chars_adv':   [UPPER_PAGE_TYPE, 17, 10, FUNC_ABSENT, DD_INFO_STRUCT, 1],
    'Impl_controls_adv':  [UPPER_PAGE_TYPE, 27,  2, FUNC_ABSENT, DD_INFO_STRUCT, 1],
    'Impl_monitors_adv':  [UPPER_PAGE_TYPE, 31,  2, FUNC_ABSENT, DD_INFO_STRUCT, 1],
    'second_app_list':    [UPPER_PAGE_TYPE, 95, 28, FUNC_ABSENT, DD_INFO_STRUCT, 1],
    'tx_disable':         [UPPER_PAGE_TYPE,  2, 1, FUNC_ABSENT, DD_INFO_STRUCT, 16],
    'rx_los':             [UPPER_PAGE_TYPE, 19, 1, FUNC_ABSENT, DD_DOM_STRUCT, 17],
    'tx_fault':           [UPPER_PAGE_TYPE,  7, 1, FUNC_ABSENT, DD_DOM_STRUCT, 17],
    'channel_state':      [UPPER_PAGE_TYPE, 26, 48, 'parse_channel_monitor_params', DD_DOM_STRUCT, 17],
    'tx_pwr_state':       [UPPER_PAGE_TYPE, 26, 16, 'parse_dom_tx_power', DD_DOM_STRUCT, 17],
    'tx_bias_state':      [UPPER_PAGE_TYPE, 42, 16, 'parse_dom_tx_bias', DD_DOM_STRUCT, 17],
    'rx_pwr_state':       [UPPER_PAGE_TYPE, 58, 16, 'parse_dom_rx_power', DD_DOM_STRUCT, 17],

    'ModuleThreshold':    [EXT_CTRL_TYPE,  0, 72, 'parse_module_threshold_values', DD_DOM_STRUCT],
}


QSFP_TYPE = "QSFP"
QSFPDD_TYPE = "QSFP_DD"

QSFP_DOM_BULK_DATA_START = 22
QSFP_DOM_BULK_DATA_SIZE = 36
QSFP_TEMPE_OFFSET = 22
QSFP_TEMPE_WIDTH = 2
QSFP_VOLT_OFFSET = 26
QSFP_VOLT_WIDTH = 2
QSFP_CHANNL_MON_OFFSET = 34
QSFP_CHANNL_MON_WIDTH = 16
QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH = 24

"""
# definitions of the offset for values in OSFP info eeprom. These are used relative to upper memory page 0
QSFPDD_UPPER_PAGE0_OFFSET = 128
QSFPDD_TYPE_OFFSET = 128
QSFPDD_VENDOR_NAME_OFFSET = 129
QSFPDD_VENDOR_OUI_OFFSET = 145
QSFPDD_VENDOR_PN_OFFSET = 148
QSFPDD_VENDOR_REV_OFFSET = 164
QSFPDD_VENDOR_SN_OFFSET = 166
QSFPDD_VENDOR_DATE_OFFSET = 182
QSFPDD_MIT_OFFSET = 212
QSFPDD_MEDIA_TYPE_ENCODING_OFFSET = 85  # lower page 0
"""


class Sfp(SfpBase):
    """
    Nokia IXR-7250 Platform-specific Sfp class
    """

    instances = []

    # used by sfp_event to synchronize presence info
    @staticmethod
    def SfpHasBeenTransitioned(port, status):
        for inst in Sfp.instances:
            if (inst.index == port):
                logger.log_error(
                    "SfpHasBeenTransitioned: invalidating_page_cache for Sfp index{} : status is {}".format(
                        inst.index, status))
                inst.invalidate_page_cache(ALL_PAGES_TYPE)
                inst.dom_capability_established = False
                return
        logger.log_error("SfpHasBeenTransitioned: no match for port index{}".format(port))

    def __init__(self, index, sfp_type, stub):
        SfpBase.__init__(self)
        self.sfp_type = sfp_type
        self.index = index
        self.sfpi_obj = None
        self.sfpd_obj = None
        self.dom_capability_established = False
        self.flatmem = None

        """
        self.qsfpDDInfo = qsfp_dd_InterfaceId()
        self.qsfpDDDomInfo = qsfp_dd_Dom(calibration_type=1)
        self.qsfpInfo = sff8436InterfaceId()
        self.qsfpDomInfo = sff8436Dom()
        """

        self.lastPresence = False

        self.CTRL_TYPE_page = bytearray([0] * 128)
        self.INFO_TYPE_page = bytearray([0] * 128)
        self.EXT_CTRL_TYPE_page = bytearray([0] * 128)
        self.page1_page = bytearray([0] * 128)
        self.CTRL_TYPE_page_cached = False
        self.INFO_TYPE_page_cached = False
        self.EXT_CTRL_TYPE_page_cached = False
        self.page1_page_cached = False
        self.stub = stub
        Sfp.instances.append(self)

    def invalidate_page_cache(self, page_type):
        # more logic is needed for DD extended pages/banks since there are many more
        if (page_type == CTRL_TYPE):
            self.CTRL_TYPE_page = None
            self.CTRL_TYPE_page_cached = False
        elif (page_type == INFO_TYPE):
            self.INFO_TYPE_page = None
            self.INFO_TYPE_page_cached = False
        elif (page_type == EXT_CTRL_TYPE):
            self.EXT_CTRL_TYPE_page = None
            self.EXT_CTRL_TYPE_page_cached = False
        elif (page_type == UPPER_PAGE_TYPE):
            # will support more here in the future, but just DD advertising page for now
            self.page1_page = None
            self.page1_page_cached = False
        elif (page_type == ALL_PAGES_TYPE):
            self.CTRL_TYPE_page = None
            self.CTRL_TYPE_page_cached = False
            self.INFO_TYPE_page = None
            self.INFO_TYPE_page_cached = False
            self.EXT_CTRL_TYPE_page = None
            self.EXT_CTRL_TYPE_page_cached = False
            self.page1_page = None
            self.page1_page_cached = False
        else:
            logger.log_error("invalidate_page_cache for Ethernet{} unknown page_type{}".format(self.index, page_type))

    def debug_eeprom_ops(self, etype, offset, num_bytes):

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return None
        ret, response = nokia_common.try_grpc(stub.GetSfpEepromInfo,
                                              platform_ndk_pb2.ReqSfpEepromPb(hw_port_id=self.index,
                                                                              etype=etype,
                                                                              offset=offset,
                                                                              num_bytes=num_bytes))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return None
        if response.data is None:
            return None

    def _read_eeprom_bytes(self, etype, offset, num_bytes, *more):
        cached_page = None
        page_num = 0
        # bank_num = 0

        if (etype == CTRL_TYPE):
            if self.CTRL_TYPE_page_cached is True:
                cached_page = self.CTRL_TYPE_page
        elif (etype == INFO_TYPE):
            if self.INFO_TYPE_page_cached is True:
                cached_page = self.INFO_TYPE_page
        elif (etype == EXT_CTRL_TYPE):
            if (self.EXT_CTRL_TYPE_page_cached is True):
                cached_page = self.EXT_CTRL_TYPE_page
        elif (etype == UPPER_PAGE_TYPE):
            page_num = more[0]
            # only supportind DD advertising page here for now
            if (page_num == 1) and (self.page1_page_cached is True):
                cached_page = self.page1_page
            # logger.log_debug("_read_eeprom_bytes: UPPER_PAGE {} with etype{} bytes{} requested for Ethernet{}".format(
            #    page_num, etype, num_bytes, self.index))
        elif (etype == UPPER_BANK_PAGE_TYPE):
            page_num = more[0]
            # bank_num = more[1]
            # logger.log_debug("_read_eeprom_bytes for Ethernet{} with UPPER_BANK_PAGE_TYPE : page {} bank {}".format(
            #    self.index, page_num, bank_num))
        else:
            logger.log_error("_read_eeprom_bytes for Ethernet{} with BAD etype{} offset{} and num_bytes{}".format(
                self.index, etype, offset, num_bytes))
            return None

        if (cached_page is None):
            logger.log_debug("_read_eeprom_bytes for Ethernet{} with etype{} offset{} num_bytes{} "
                             "and page_num{}".format(self.index, etype, offset, num_bytes, page_num))

            channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
            if not channel or not stub:
                return None
            ret, response = nokia_common.try_grpc(stub.GetSfpEepromInfo,
                                                  platform_ndk_pb2.ReqSfpEepromPb(hw_port_id=self.index,
                                                                                  etype=etype,
                                                                                  offset=offset,
                                                                                  num_bytes=num_bytes,
                                                                                  page=page_num))
            nokia_common.channel_shutdown(channel)

            if ret is False:
                return None

            if response.data is None:
                logger.log_error("response.data is None! _read_eeprom_bytes for Ethernet{} with etype{} "
                                 "offset{} num_bytes{} and page_num{}".format(self.index, etype, offset,
                                                                              num_bytes, page_num))
                return None

            raw = bytearray(response.data)

            # cache the page
            # more logic is needed for DD extended pages/banks since there are many more
            if (etype == CTRL_TYPE):
                self.CTRL_TYPE_page = raw
                self.CTRL_TYPE_page_cached = True
            elif (etype == INFO_TYPE):
                self.INFO_TYPE_page = raw
                self.INFO_TYPE_page_cached = True
            elif (etype == EXT_CTRL_TYPE):
                self.EXT_CTRL_TYPE_page = raw
                self.EXT_CTRL_TYPE_page_cached = True
            elif (etype == UPPER_PAGE_TYPE):
                if (page_num == 1):
                    self.page1_page = raw
                    self.page1_page_cached = True
            # else:
            #    logger.log_error("_read_eeprom_bytes for Ethernet{} with BAD etype{} offset{} and "
            #                     "num_bytes".format(self.index, etype, offset, num_bytes))
            #    return None

        else:
            logger.log_debug("_read_eeprom_bytes CACHE HIT for Ethernet{} with etype{} offset{} and "
                             "num_bytes {}".format(self.index, etype, offset, num_bytes))
            # logger.log_error("     cached_page {}".format(cached_page))
            raw = cached_page

        eeprom_raw = [0] * num_bytes

        try:
            """
            for n in range(0, num_bytes):  # Since we are getting entire 128 bytes every single time
                # eeprom_raw[n] = hex(raw[offset+n])[2:].zfill(2)
                eeprom_raw[n] = hex(raw[offset+n]).lstrip("0x").zfill(2)
            """
            # in case raw is bytes (python3 is used) raw[n] will return int,
            # and in case raw is str(python2 is used) raw[n] will return str,
            # so for python3 there is no need to call ord to convert str to int.
            # TODO: Remove this check once we no longer support Python 2
            """
            if type(raw) == bytearray:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(raw[offset+n])[2:].zfill(2)
            else:
                for n in range(0, num_bytes):
                    eeprom_raw[n] = hex(ord(raw[offset+n]))[2:].zfill(2)
            """
            for n in range(0, num_bytes):
                eeprom_raw[n] = hex(raw[offset+n])[2:].zfill(2)

        except BaseException:
            logger.log_debug("BaseException in _read_eeprom_bytes for Ethernet{} with etype{} offset{} "
                             "num_bytes {} and page {}".format(self.index, etype, offset, num_bytes, page_num))
            return None

        return eeprom_raw

    def _get_eeprom_dd_dom_data(self, eeprom_key):
        eeprom_data = None

        if (self.sfp_type == QSFPDD_TYPE):
            # logger.log_debug("Getting eeprom size for SFP {} type {} num-bytes"
            #                  " {}".format(self.index, eeprom_key, qsfpdd_parser[eeprom_key][KEY_WIDTH]))
            eeprom_data_raw = self._read_eeprom_bytes(
                qsfpdd_parser[eeprom_key][READ_TYPE],
                qsfpdd_parser[eeprom_key][KEY_OFFSET],
                qsfpdd_parser[eeprom_key][KEY_WIDTH])

            if (eeprom_data_raw is not None):
                eeprom_data = getattr(
                    self.sfpd_obj, qsfpdd_parser[eeprom_key][FUNC_NAME])(
                    eeprom_data_raw, 0)
        else:
            logger.log_error(
                "_get_eeprom_dom_data called with non-DOM-data for SFP {} eeprom_key {}".format(self.index, eeprom_key))

        return eeprom_data

    def _get_eeprom_data(self, eeprom_key):
        eeprom_data = None

        if (self.sfp_type == QSFPDD_TYPE):
            struct_type = qsfpdd_parser[eeprom_key][STRUCT_TYPE]

            # if PAGE_NUMBER parameter present
            if (len(qsfpdd_parser[eeprom_key]) > PAGE_NUMBER):
                eeprom_data_raw = self._read_eeprom_bytes(
                    qsfpdd_parser[eeprom_key][READ_TYPE],
                    qsfpdd_parser[eeprom_key][KEY_OFFSET],
                    qsfpdd_parser[eeprom_key][KEY_WIDTH],
                    qsfpdd_parser[eeprom_key][PAGE_NUMBER])
            else:
                eeprom_data_raw = self._read_eeprom_bytes(
                    qsfpdd_parser[eeprom_key][READ_TYPE],
                    qsfpdd_parser[eeprom_key][KEY_OFFSET],
                    qsfpdd_parser[eeprom_key][KEY_WIDTH])
            """
            logger.log_debug("** _get_eeprom_data for SFP {} type {} and eeprom_key {} data {}".format(
                    self.index, self.sfp_type, eeprom_key, eeprom_data_raw))
            """

            if (eeprom_data_raw is not None):
                if (qsfpdd_parser[eeprom_key][FUNC_NAME] == FUNC_ABSENT):
                    return eeprom_data_raw
                if (struct_type == DD_INFO_STRUCT):
                    eeprom_data = getattr(
                        self.sfpi_obj, qsfpdd_parser[eeprom_key][FUNC_NAME])(
                        eeprom_data_raw, 0)
                elif (struct_type == DD_DOM_STRUCT):
                    eeprom_data = getattr(
                        self.sfpd_obj, qsfpdd_parser[eeprom_key][FUNC_NAME])(
                        eeprom_data_raw, 0)
                else:
                    logger.log_error("_get_eeprom_data bad struct_type {} for SFP {} and eeprom_key {}".format(
                        struct_type, self.index, eeprom_key))
                    return None
        else:
            op_type = sff8436_parser[eeprom_key][READ_TYPE]
            eeprom_data_raw = self._read_eeprom_bytes(
                sff8436_parser[eeprom_key][READ_TYPE],
                sff8436_parser[eeprom_key][KEY_OFFSET],
                sff8436_parser[eeprom_key][KEY_WIDTH])

            # if struct_type parameter present
            if (len(sff8436_parser[eeprom_key]) > STRUCT_TYPE):
                struct_type = sff8436_parser[eeprom_key][STRUCT_TYPE]
            else:
                struct_type = None

            if (eeprom_data_raw is not None):
                if (sff8436_parser[eeprom_key][FUNC_NAME] == FUNC_ABSENT):
                    return eeprom_data_raw
                # INFO_TYPE is used to retrieve sff8436InterfaceId Info
                # CTRL_TYPE is used to retrieve sff8436Dom Info
                # if struct_type present then override operation for DOM
                if (struct_type == QSFP_DOM_STRUCT):
                    op_type = CTRL_TYPE

                if (op_type == INFO_TYPE):
                    if (self.sfpi_obj is None):
                        return None
                    eeprom_data = getattr(
                        self.sfpi_obj, sff8436_parser[eeprom_key][FUNC_NAME])(
                        eeprom_data_raw, 0)
                else:
                    if (self.sfpd_obj is None):
                        return None
                    eeprom_data = getattr(
                        self.sfpd_obj, sff8436_parser[eeprom_key][FUNC_NAME])(
                        eeprom_data_raw, 0)
            else:
                logger.log_debug("_get_eeprom_data eeprom_data_raw is None for SFP {} sfp_type {} and "
                                 "eeprom_key {}".format(self.index, self.sfp_type, eeprom_key))

        return eeprom_data

    def get_transceiver_info(self):
        """
        Retrieves transceiver info of this SFP
        """
        transceiver_info_dict = {}
        compliance_code_dict = {}
        # transceiver_info_dict = dict.fromkeys(info_dict_keys, 'N/A')

        self.invalidate_page_cache(ALL_PAGES_TYPE)

        # get upper page 0 Administrative data directly
        # eeprom_raw = self._get_eeprom_data_raw()
        # id = int(eeprom_raw[0], 16)

        # pull the INFO page into cache so we can dynamically grab the ID
        eeprom_data_raw = self._read_eeprom_bytes(INFO_TYPE, 0, 128)
        if eeprom_data_raw is None:
            logger.log_info("_read_eeprom_bytes returned None for Ethernet{}".format(self.index))
            self.sfp_type = None
            return None

        id = int(eeprom_data_raw[0], 16)

        # logger.log_debug("GTI for Ethernet{} id {} raw {}".format(self.index, id, eeprom_data_raw))

        # we should set type correctly ad hoc, immediately after module insertion has been detected.
        # Do it here for now...
        if (id == 24):
            self.sfp_type = QSFPDD_TYPE
            self.sfpi_obj = qsfp_dd_InterfaceId()
        elif (id == 17) or (id == 13):
            self.sfp_type = QSFP_TYPE
            self.sfpi_obj = sff8436InterfaceId()
        else:
            logger.log_error("Unknown module ID {} for Ethernet{}".format(id, self.index))
            self.sfp_type = None
            return None

        # now parse up the cached page
        iface_data = self._get_eeprom_data('type')

        if(self.sfp_type == QSFPDD_TYPE):
            if self.sfpi_obj is None:
                logger.log_error(
                    "qsfp_dd_InterfaceId obj is None in get_transceiver_info for Ethernet{}".format(self.index))
                return None

            """
            if (iface_data is not None):
                connector = iface_data['data']['Connector']['value']
                length = iface_data['data']['CableAssemblyLength']['value']
                power = iface_data['data']['ModulePower']['value']
                identifier = iface_data['data']['type']['value']
                type_abbrv_name = iface_data['data']['type_abbrv_name']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None
            """

            # type
            data_type = 'type'
            type_data_raw = self._get_eeprom_data(data_type)
            if (type_data_raw is not None):
                type_data = type_data_raw['data']['type']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # connector
            data_type = 'Connector'
            connector_data = self._get_eeprom_data(data_type)
            if (connector_data is not None):
                connector = connector_data['data']['Connector']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # length
            data_type = 'cable_length'
            cable_length_data = self._get_eeprom_data(data_type)
            if (cable_length_data is not None):
                cable_length = str(cable_length_data['data']['Length Cable Assembly(m)']['value'])
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # ext id
            data_type = 'ext_iden'
            ext_id_data = self._get_eeprom_data(data_type)
            if (ext_id_data is not None):
                ext_id = ext_id_data['data']['Extended Identifier']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Vendor Date
            data_type = 'vendor_date'
            vendor_date_data = self._get_eeprom_data(data_type)
            if (vendor_date_data is not None):
                vendor_date = vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Vendor Name
            data_type = 'manufacturename'
            vendor_name_data = self._get_eeprom_data(data_type)
            if (vendor_name_data is not None):
                vendor_name = vendor_name_data['data']['Vendor Name']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Vendor OUI
            data_type = 'vendor_oui'
            vendor_oui_data = self._get_eeprom_data(data_type)
            if (vendor_oui_data is not None):
                vendor_oui = vendor_oui_data['data']['Vendor OUI']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Vendor PN
            data_type = 'modelname'
            vendor_pn_data = self._get_eeprom_data(data_type)
            if (vendor_pn_data is not None):
                vendor_pn = vendor_pn_data['data']['Vendor PN']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Vendor Revision
            data_type = 'hardwarerev'
            vendor_rev_data = self._get_eeprom_data(data_type)
            if (vendor_rev_data is not None):
                vendor_rev = vendor_rev_data['data']['Vendor Rev']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Vendor Serial Number
            data_type = 'serialnum'
            vendor_sn_data = self._get_eeprom_data(data_type)
            if (vendor_sn_data is not None):
                vendor_sn = vendor_sn_data['data']['Vendor SN']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Media type
            data_type = 'media_type'
            media_type_dict = self._get_eeprom_data(data_type)
            if (media_type_dict is None):
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            host_media_list = ""
            data_type = 'first_app_list'
            application_list_first = self._get_eeprom_data(data_type)
            if application_list_first is not None:
                max_application_count = 8
                application_list = application_list_first
            else:
                logger.log_error("Failed to get any application list in get_transceiver_info for "
                                 "Ethernet{} type {}".format(self.index, self.sfp_type))
                return None

            if not self.flatmem:
                data_type = 'second_app_list'
                application_list_second = self._get_eeprom_data(data_type)
                if application_list_second is not None:
                    max_application_count = 15
                    application_list += application_list_second

            # logger.log_error("appl list for Ethernet{} type {} : {} {} {}".format(
            #    self.index, self.sfp_type, max_application_count, application_list_first, application_list_second))

            for i in range(0, max_application_count):
                if application_list[i * 4] == 'ff':
                    break
                host_electrical, media_interface = self.sfpi_obj.parse_application(
                    media_type_dict, application_list[i * 4], application_list[i * 4 + 1])
                host_media_list = host_media_list + host_electrical + ' - ' + media_interface + '\n\t\t\t\t   '

            """
            # Revision Compliance
            data_type = 'RevisionCompliance'
            rev_compliance_data = self._get_eeprom_data(data_type)
            if (rev_compliance_data is not None):
                rev_compliance = rev_compliance_data['data']['Revision Compliance']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # Flat Mem
            data_type = 'summary byte2'
            summary_byte2_data = self._get_eeprom_data(data_type)
            if (summary_byte2_data is not None):
                summary_byte2 = summary_byte2_data['data']['Summary Byte2']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index ))
                return None

            # FW version
            data_type = 'FW version'
            FW_rev_data = self._get_eeprom_data(data_type)
            if (FW_rev_data is not None):
                FW_rev = FW_rev_data['data']['FW Version']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # MIT
            data_type = 'MIT'
            MIT_data = self._get_eeprom_data(data_type)
            if (MIT_data is not None):
                MIT = MIT_data['data']['MIT']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index ))
                return None

            # advertising
            data_type = 'App advertising'
            app_adv_data = self._get_eeprom_data(data_type)
            if (app_adv_data is not None):
                app_adv = app_adv_data['data']['App Advertising']['value']
            else:
                logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                    data_type, self.index))
                return None

            # supported link lengths
            if (self.qsfpDDInfo.Flat_mem == False):
               data_type = 'Supported Link Length'
               SLL_data = self._get_eeprom_data(data_type)
               if (SLL_data is not None):
                   SLL = SLL_data['data']['Supported Link Length']['value']
               else:
                   logger.log_error("Failed to get {} for get_transceiver_info for Ethernet{}".format(
                       data_type, self.index ))
                   SLL = "N/A"
            else:
               SLL = "N/A"
            """

            # logger.log_error("  type {} name {} pn {} rev {} sn {} oui {} for Ethernet{}".format(
            #     sfp_type_data, sfp_vendor_name_data, sfp_vendor_pn_data, sfp_vendor_rev_data,
            #     sfp_vendor_sn_data, sfp_vendor_oui_data, sfp_vendor_data_data, self.index))

            # Fill The Dictionary and return
            transceiver_info_dict['type'] = type_data
            transceiver_info_dict['manufacturer'] = vendor_name
            transceiver_info_dict['model'] = vendor_pn
            transceiver_info_dict['hardware_rev'] = vendor_rev
            transceiver_info_dict['serial'] = vendor_sn
            transceiver_info_dict['vendor_oui'] = vendor_oui
            transceiver_info_dict['vendor_date'] = vendor_date
            transceiver_info_dict['connector'] = connector
            transceiver_info_dict['encoding'] = "N/A"
            transceiver_info_dict['ext_identifier'] = ext_id
            transceiver_info_dict['ext_rateselect_compliance'] = "N/A"
            transceiver_info_dict['specification_compliance'] = "N/A"
            transceiver_info_dict['cable_type'] = "Length Cable Assembly(m)"
            transceiver_info_dict['cable_length'] = cable_length
            transceiver_info_dict['nominal_bit_rate'] = "N/A"
            transceiver_info_dict['application_advertisement'] = host_media_list

            """
            transceiver_info_dict['revision_compliance'] = str(rev_compliance)
            transceiver_info_dict['power'] = power
            transceiver_info_dict['summary_byte2'] = summary_byte2
            transceiver_info_dict['fw_revision'] = str(FW_rev)
            transceiver_info_dict['app_advertising'] = app_adv
            transceiver_info_dict['type_abbrv_name']=type_abbrv_name
            transceiver_info_dict['supported_link_length'] = SLL
            """

            return transceiver_info_dict

        # we can also do check here less dynamically via
        # if (self.sfp_type == 'QSFP28'):
        # but dynamic is better so long as we already have module ID returned
        elif (self.sfp_type == QSFP_TYPE):
            if self.sfpi_obj is None:
                logger.log_error("sff8436InterfaceId obj is None for QSFP28 in get_transceiver_info "
                                 "for Ethernet{}".format(self.index))
                return None

            connector = iface_data['data']['Connector']['value']
            encoding = iface_data['data']['EncodingCodes']['value']
            ext_id = iface_data['data']['Extended Identifier']['value']
            rate_identifier = iface_data['data']['RateIdentifier']['value']
            identifier = iface_data['data']['type']['value']
            type_abbrv_name = iface_data['data']['type_abbrv_name']['value']
            bit_rate = str(iface_data['data']['Nominal Bit Rate(100Mbs)']['value'])

            for key in qsfp_cable_length_tup:
                if key in iface_data['data']:
                    cable_type = key
                    cable_length = str(iface_data['data'][key]['value'])

            for key in qsfp_compliance_code_tup:
                if key in iface_data['data']['Specification compliance']['value']:
                    compliance_code_dict[key] = iface_data['data']['Specification compliance']['value'][key]['value']

            ext_spec_comp = self._get_eeprom_data('ext_spec_compliance')
            if (ext_spec_comp is not None):
                if (ext_spec_comp['data']['Extended Specification compliance']['value'] != "Unspecified"):
                    comp_value = ext_spec_comp['data']['Extended Specification compliance']['value']
                    compliance_code_dict['Extended Specification compliance'] = comp_value

            # Vendor Date
            vendor_date_data = self._get_eeprom_data('vendor_date')
            if (vendor_date_data is not None):
                vendor_date = vendor_date_data['data']['VendorDataCode(YYYY-MM-DD Lot)']['value']
            else:
                return transceiver_info_dict

            # Vendor Name
            vendor_name_data = self._get_eeprom_data('manufacturename')
            if (vendor_name_data is not None):
                vendor_name = vendor_name_data['data']['Vendor Name']['value']
            else:
                return transceiver_info_dict

            # Vendor OUI
            vendor_oui_data = self._get_eeprom_data('vendor_oui')
            if (vendor_oui_data is not None):
                vendor_oui = vendor_oui_data['data']['Vendor OUI']['value']
            else:
                return transceiver_info_dict

            # Vendor PN
            vendor_pn_data = self._get_eeprom_data('modelname')
            if (vendor_pn_data is not None):
                vendor_pn = vendor_pn_data['data']['Vendor PN']['value']
            else:
                return transceiver_info_dict

            # Vendor Revision
            vendor_rev_data = self._get_eeprom_data('hardwarerev')
            if (vendor_rev_data is not None):
                vendor_rev = vendor_rev_data['data']['Vendor Rev']['value']
            else:
                return transceiver_info_dict

            # Vendor Serial Number
            vendor_sn_data = self._get_eeprom_data('serialnum')
            if (vendor_sn_data is not None):
                vendor_sn = vendor_sn_data['data']['Vendor SN']['value']
            else:
                return transceiver_info_dict

            # Fill The Dictionary and return
            transceiver_info_dict['type'] = identifier
            transceiver_info_dict['manufacturer'] = vendor_name
            transceiver_info_dict['model'] = vendor_pn
            transceiver_info_dict['hardware_rev'] = vendor_rev
            transceiver_info_dict['serial'] = vendor_sn
            transceiver_info_dict['vendor_oui'] = vendor_oui
            transceiver_info_dict['vendor_date'] = vendor_date
            transceiver_info_dict['connector'] = connector
            transceiver_info_dict['encoding'] = encoding
            transceiver_info_dict['ext_identifier'] = ext_id
            transceiver_info_dict['ext_rateselect_compliance'] = rate_identifier
            transceiver_info_dict['cable_type'] = cable_type
            transceiver_info_dict['cable_length'] = cable_length
            transceiver_info_dict['nominal_bit_rate'] = bit_rate
            transceiver_info_dict['specification_compliance'] = str(compliance_code_dict)
            transceiver_info_dict['type_abbrv_name'] = type_abbrv_name
            transceiver_info_dict['application_advertisement'] = 'N/A'

            return transceiver_info_dict

        else:
            logger.log_error("unrecognizedID is {} in get_transceiver_info for Ethernet{}".format(id, self.index))
            return None

    def _dom_capability_detect(self):
        if not self.get_presence():
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_bias_power_supported = False
            self.dom_tx_power_supported = False
            self.calibration = 0
            self.dom_capability_established = False
            self.dom_tx_disable_supported = False
            self.dom_tx_disable_per_lane_supported = False
            return

        if self.sfpi_obj is None:
            self.dom_supported = False
            self.dom_capability_established = False
            # wait until self.sfpi_obj has been set to start with DOM ops
            return

        self.invalidate_page_cache(ALL_PAGES_TYPE)
        self.dom_capability_established = True
        qsfp_version_compliance = "unestablished"

        if self.sfp_type == QSFP_TYPE:
            self.calibration = 1
            self.sfpd_obj = sff8436Dom()
            if self.sfpd_obj is None:
                self.dom_supported = False
            # self.sfpi_obj should already be set
            if self.sfpi_obj is None:
                self.dom_supported = False
                logger.log_error(
                    "self.sfpi_obj is None in dom_capability_detect for QSFP type Ethernet{}".format(self.index))
                return
            dom_capability = self._get_eeprom_data('dom_monitor_type')
            if (dom_capability is not None):
                qsfp_version_compliance_raw = self._get_eeprom_data('compliance')
                qsfp_version_compliance = int(qsfp_version_compliance_raw[0], 16)
                if qsfp_version_compliance >= 0x08:
                    self.dom_temp_supported = dom_capability['data']['Temp_support']['value'] == 'On'
                    self.dom_volt_supported = dom_capability['data']['Voltage_support']['value'] == 'On'
                    self.dom_rx_power_supported = dom_capability['data']['Rx_power_support']['value'] == 'On'
                    self.dom_tx_power_supported = dom_capability['data']['Tx_power_support']['value'] == 'On'
                else:
                    self.dom_temp_supported = True
                    self.dom_volt_supported = True
                    self.dom_rx_power_supported = dom_capability['data']['Rx_power_support']['value'] == 'On'
                    self.dom_tx_power_supported = True
                self.dom_supported = True
                self.calibration = 1
                self.dom_tx_disable_supported = False
                optional_capability = self._get_eeprom_data('options')
                if optional_capability is not None:
                    self.dom_tx_disable_supported = optional_capability['data']['TxDisable']['value'] == 'On'
                dom_status_indicator = self.sfpd_obj.parse_dom_status_indicator(qsfp_version_compliance_raw, 1)

                self.flatmem = dom_status_indicator['data']['FlatMem']['value'] == 'On'
            else:
                self.dom_supported = False
                self.dom_temp_supported = False
                self.dom_volt_supported = False
                self.dom_rx_power_supported = False
                self.dom_tx_power_supported = False
                self.calibration = 0
                self.flatmem = True
                self.dom_tx_disable_supported = False
            logger.log_info("dom_capability established for QSFP type Ethernet{} version_compliance{} "
                            "dom_supported{} temp{} volt{} rx_pwr{} tx_pwr{} tx_dis{} flatmem{}".format(
                                self.index, qsfp_version_compliance, self.dom_supported, self.dom_temp_supported,
                                self.dom_volt_supported, self.dom_rx_power_supported, self.dom_tx_power_supported,
                                self.dom_tx_disable_supported, self.flatmem))
        elif self.sfp_type == QSFPDD_TYPE:
            self.sfpd_obj = qsfp_dd_Dom()
            # self.sfpi_obj should already be set
            if self.sfpi_obj is None:
                self.dom_supported = False
                logger.log_error(
                    "self.sfpi_obj is None in dom_capability_detect for QSFP_DD type Ethernet{}".format(self.index))
                return
            dom_capability = self._get_eeprom_data('dom_monitor_type')
            if (dom_capability is not None):
                if dom_capability['data']['Flat_MEM']['value'] == 'Off':
                    self.flatmem = False
                    self.dom_supported = True
                    self.dom_rx_power_supported = True
                    self.dom_tx_power_supported = True
                    self.dom_tx_bias_power_supported = True
                    self.dom_thresholds_supported = True
                    self.dom_rx_tx_power_bias_supported = True

                    module_chars_raw = self._get_eeprom_data('Module_chars_adv')
                    byte151_data = int(module_chars_raw[6], 16)
                    if (byte151_data & 0x01 != 0):
                        self.dom_tx_disable_per_lane_supported = False
                    else:
                        self.dom_tx_disable_per_lane_supported = True

                    implemented_controls_raw = self._get_eeprom_data('Impl_controls_adv')
                    byte155_data = int(implemented_controls_raw[0], 16)
                    if (byte155_data & 0x02 != 0):
                        self.dom_tx_disable_supported = True
                    else:
                        self.dom_tx_disable_supported = False

                    implemented_monitors_raw = self._get_eeprom_data('Impl_monitors_adv')
                    byte159_data = int(implemented_monitors_raw[0], 16)
                    if (byte159_data & 0x01 != 0):
                        self.dom_temp_supported = True
                    else:
                        self.dom_temp_supported = False

                    if (byte159_data & 0x02 != 0):
                        self.dom_volt_supported = True
                    else:
                        self.dom_volt_supported = False
                else:
                    self.flatmem = True
                    self.dom_supported = False
                    self.dom_temp_supported = False
                    self.dom_volt_supported = False
                    self.dom_rx_power_supported = False
                    self.dom_tx_power_supported = False
                    self.dom_tx_bias_power_supported = False
                    self.dom_thresholds_supported = False
                    self.dom_rx_tx_power_bias_supported = False
                    self.dom_tx_disable_supported = False
                    self.dom_tx_disable_per_lane_supported = False
            else:
                self.flatmem = True
                self.dom_supported = False
                self.dom_temp_supported = False
                self.dom_volt_supported = False
                self.dom_rx_power_supported = False
                self.dom_tx_power_supported = False
                self.dom_tx_bias_power_supported = False
                self.dom_thresholds_supported = False
                self.dom_rx_tx_power_bias_supported = False
                self.dom_tx_disable_supported = False
                self.dom_tx_disable_per_lane_supported = False

            logger.log_info("dom_capability established for QSFP-DD type Ethernet{} dom_supported{} temp{} volt{} "
                            "rx_pwr{} tx_pwr{} tx_bias{} thresholds{} rx_tx_power_bias{} tx_disable{}"
                            "tx_disable_per_lane{} flatmem{}".format(
                                self.index, self.dom_supported, self.dom_temp_supported, self.dom_volt_supported,
                                self.dom_rx_power_supported, self.dom_tx_power_supported,
                                self.dom_tx_bias_power_supported, self.dom_thresholds_supported,
                                self.dom_rx_tx_power_bias_supported, self.dom_tx_disable_supported,
                                self.dom_tx_disable_per_lane_supported, self.flatmem))
        else:
            self.dom_supported = False
            self.dom_temp_supported = False
            self.dom_volt_supported = False
            self.dom_rx_power_supported = False
            self.dom_tx_power_supported = False

    def is_dom_supported(self):
        return self.dom_supported

    def is_dom_tx_disable_supported(self):
        if self.dom_supported:
            return self.dom_tx_disable_supported
        else:
            return False

    def _convert_string_to_num(self, value_str):
        if "-inf" in value_str:
            return 'N/A'
        elif "Unknown" in value_str:
            return 'N/A'
        elif 'dBm' in value_str:
            t_str = value_str.rstrip('dBm')
            return float(t_str)
        elif 'mA' in value_str:
            t_str = value_str.rstrip('mA')
            return float(t_str)
        elif 'C' in value_str:
            t_str = value_str.rstrip('C')
            return float(t_str)
        elif 'Volts' in value_str:
            t_str = value_str.rstrip('Volts')
            return float(t_str)
        else:
            return 'N/A'

    def get_transceiver_bulk_status(self):

        transceiver_dom_info_dict = {}

        dom_info_dict_keys = ['temperature',    'voltage',
                              'rx1power',       'rx2power',
                              'rx3power',       'rx4power',
                              'rx5power',       'rx6power',
                              'rx7power',       'rx8power',
                              'tx1bias',        'tx2bias',
                              'tx3bias',        'tx4bias',
                              'tx5bias',        'tx6bias',
                              'tx7bias',        'tx8bias',
                              'tx1power',       'tx2power',
                              'tx3power',       'tx4power',
                              'tx5power',       'tx6power',
                              'tx7power',       'tx8power'
                              ]
        transceiver_dom_info_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')

        if not self.dom_capability_established:
            self._dom_capability_detect()

        # flush cache for all germane pages where dynamically updated data exists
        # self.invalidate_page_cache(CTRL_TYPE)
        # self.invalidate_page_cache(EXT_CTRL_TYPE)
        self.invalidate_page_cache(ALL_PAGES_TYPE)

        if self.sfp_type == QSFP_TYPE:
            if not self.dom_supported:
                return transceiver_dom_info_dict

            if self.sfpd_obj is None:
                return transceiver_dom_info_dict

            dom_data_raw = self._get_eeprom_data('dom_bulk_data')
            if dom_data_raw is None:
                return transceiver_dom_info_dict

            # logger.log_debug("  get_xcvr_bulk_status working for QSFP type Ethernet{} type{} sfpd_obj{}:"
            #                 " dom_data_raw {}".format(self.index, self.sfp_type, self.sfpd_obj, dom_data_raw))
            if self.dom_temp_supported:
                start = QSFP_TEMPE_OFFSET - QSFP_DOM_BULK_DATA_START
                end = start + QSFP_TEMPE_WIDTH
                dom_temperature_data = self.sfpd_obj.parse_temperature(dom_data_raw[start: end], 0)
                temp = self._convert_string_to_num(dom_temperature_data['data']['Temperature']['value'])
                if temp is not None:
                    transceiver_dom_info_dict['temperature'] = temp

            if self.dom_volt_supported:
                start = QSFP_VOLT_OFFSET - QSFP_DOM_BULK_DATA_START
                end = start + QSFP_VOLT_WIDTH
                dom_voltage_data = self.sfpd_obj.parse_voltage(dom_data_raw[start: end], 0)
                volt = self._convert_string_to_num(dom_voltage_data['data']['Vcc']['value'])
                if volt is not None:
                    transceiver_dom_info_dict['voltage'] = volt

            start = QSFP_CHANNL_MON_OFFSET - QSFP_DOM_BULK_DATA_START
            end = start + QSFP_CHANNL_MON_WITH_TX_POWER_WIDTH
            dom_channel_monitor_data = self.sfpd_obj.parse_channel_monitor_params_with_tx_power(
                dom_data_raw[start: end], 0)

            if self.dom_tx_power_supported:
                # logger.log_debug("    ** dom_channel_monitor_data {}".format(dom_channel_monitor_data))
                transceiver_dom_info_dict['tx1power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX1Power']['value'])
                transceiver_dom_info_dict['tx2power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX2Power']['value'])
                transceiver_dom_info_dict['tx3power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX3Power']['value'])
                transceiver_dom_info_dict['tx4power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['TX4Power']['value'])

            if self.dom_rx_power_supported:
                transceiver_dom_info_dict['rx1power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX1Power']['value'])
                transceiver_dom_info_dict['rx2power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX2Power']['value'])
                transceiver_dom_info_dict['rx3power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX3Power']['value'])
                transceiver_dom_info_dict['rx4power'] = self._convert_string_to_num(
                    dom_channel_monitor_data['data']['RX4Power']['value'])

            transceiver_dom_info_dict['tx1bias'] = dom_channel_monitor_data['data']['TX1Bias']['value']
            transceiver_dom_info_dict['tx2bias'] = dom_channel_monitor_data['data']['TX2Bias']['value']
            transceiver_dom_info_dict['tx3bias'] = dom_channel_monitor_data['data']['TX3Bias']['value']
            transceiver_dom_info_dict['tx4bias'] = dom_channel_monitor_data['data']['TX4Bias']['value']
            # logger.log_debug("**get_xcvr_bulk_status done for QSFP type Ethernet{} type{}"
            #                 " sfpd_obj{}: info_dict {}".format(self.index, self.sfp_type, self.sfpd_obj,
            #                                                    transceiver_dom_info_dict))

        elif self.sfp_type == QSFPDD_TYPE:

            if self.sfpd_obj is None:
                return transceiver_dom_info_dict

            if self.dom_temp_supported:
                temp_data = self._get_eeprom_data('temperature')
                if (temp_data is not None):
                    temp = self._convert_string_to_num(temp_data['data']['Temperature']['value'])
                    if temp is not None:
                        transceiver_dom_info_dict['temperature'] = temp

            if self.dom_volt_supported:
                volt_data = self._get_eeprom_data('voltage')
                if (volt_data is not None):
                    volt = self._convert_string_to_num(volt_data['data']['Vcc']['value'])
                    if volt is not None:
                        transceiver_dom_info_dict['voltage'] = volt

            if not self.flatmem:
                dom_channel_monitor_data = self._get_eeprom_data('channel_state')
                # logger.log_debug("       dom_channel_monitor_data {}".format(dom_channel_monitor_data))
                if dom_channel_monitor_data is None:
                    return transceiver_dom_info_dict

                # logger.log_debug("**get_xcvr_bulk_status QSFP-DD type Ethernet{} type{}  sfpd_obj{}: "
                #                 "channel_data {}".format(self.index, self.sfp_type, self.sfpd_obj, channel_data))

                if self.dom_tx_power_supported:
                    transceiver_dom_info_dict['tx1power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX1Power']['value']))
                    transceiver_dom_info_dict['tx2power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX2Power']['value']))
                    transceiver_dom_info_dict['tx3power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX3Power']['value']))
                    transceiver_dom_info_dict['tx4power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX4Power']['value']))
                    transceiver_dom_info_dict['tx5power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX5Power']['value']))
                    transceiver_dom_info_dict['tx6power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX6Power']['value']))
                    transceiver_dom_info_dict['tx7power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX7Power']['value']))
                    transceiver_dom_info_dict['tx8power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['TX8Power']['value']))

                if self.dom_rx_power_supported:
                    transceiver_dom_info_dict['rx1power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX1Power']['value']))
                    transceiver_dom_info_dict['rx2power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX2Power']['value']))
                    transceiver_dom_info_dict['rx3power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX3Power']['value']))
                    transceiver_dom_info_dict['rx4power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX4Power']['value']))
                    transceiver_dom_info_dict['rx5power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX5Power']['value']))
                    transceiver_dom_info_dict['rx6power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX6Power']['value']))
                    transceiver_dom_info_dict['rx7power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX7Power']['value']))
                    transceiver_dom_info_dict['rx8power'] = str(self._convert_string_to_num(
                        dom_channel_monitor_data['data']['RX8Power']['value']))

                if self.dom_tx_bias_power_supported:
                    transceiver_dom_info_dict['tx1bias'] = str(dom_channel_monitor_data['data']['TX1Bias']['value'])
                    transceiver_dom_info_dict['tx2bias'] = str(dom_channel_monitor_data['data']['TX2Bias']['value'])
                    transceiver_dom_info_dict['tx3bias'] = str(dom_channel_monitor_data['data']['TX3Bias']['value'])
                    transceiver_dom_info_dict['tx4bias'] = str(dom_channel_monitor_data['data']['TX4Bias']['value'])
                    transceiver_dom_info_dict['tx5bias'] = str(dom_channel_monitor_data['data']['TX5Bias']['value'])
                    transceiver_dom_info_dict['tx6bias'] = str(dom_channel_monitor_data['data']['TX6Bias']['value'])
                    transceiver_dom_info_dict['tx7bias'] = str(dom_channel_monitor_data['data']['TX7Bias']['value'])
                    transceiver_dom_info_dict['tx8bias'] = str(dom_channel_monitor_data['data']['TX8Bias']['value'])
        else:
            logger.log_error("get_transceiver_bulk_status: Unknown module type {} for Ethernet{}".format(
                self.sfp_type, self.index))

        return transceiver_dom_info_dict

    def get_transceiver_threshold_info(self):

        xcvr_dom_thresh_dict = {}

        dom_info_dict_keys = ['temphighalarm',    'temphighwarning',
                              'templowalarm',     'templowwarning',
                              'vcchighalarm',     'vcchighwarning',
                              'vcclowalarm',      'vcclowwarning',
                              'rxpowerhighalarm', 'rxpowerhighwarning',
                              'rxpowerlowalarm',  'rxpowerlowwarning',
                              'txpowerhighalarm', 'txpowerhighwarning',
                              'txpowerlowalarm',  'txpowerlowwarning',
                              'txbiashighalarm',  'txbiashighwarning',
                              'txbiaslowalarm',   'txbiaslowwarning'
                              ]
        xcvr_dom_thresh_dict = dict.fromkeys(dom_info_dict_keys, 'N/A')

        if not self.dom_capability_established:
            self._dom_capability_detect()

        # flush cache for all germane pages where dynamically updated data exists
        # self.invalidate_page_cache(CTRL_TYPE)
        # self.invalidate_page_cache(EXT_CTRL_TYPE)
        self.invalidate_page_cache(ALL_PAGES_TYPE)

        if self.sfp_type == QSFP_TYPE:
            if self.sfpd_obj is None or not self.dom_supported or self.flatmem:
                return xcvr_dom_thresh_dict

            logger.log_debug("  get_xcvr_threshold_info working for QSFP type Ethernet{}".format(self.index))

            module_threshold_data = self._get_eeprom_data('ModuleThreshold')
            if module_threshold_data is None:
                logger.log_debug("  get_xcvr_threshold_info module_threshold_data"
                                 " is None for QSFP type Ethernet{}".format(self.index))
                return xcvr_dom_thresh_dict

            channel_threshold_data = self._get_eeprom_data('ChannelThreshold')
            if channel_threshold_data is None:
                logger.log_debug("  get_xcvr_threshold_info channel_threshold_data is"
                                 " None for QSFP type Ethernet{}".format(self.index))
                return xcvr_dom_thresh_dict

            xcvr_dom_thresh_dict['temphighalarm'] = module_threshold_data['data']['TempHighAlarm']['value']
            xcvr_dom_thresh_dict['temphighwarning'] = module_threshold_data['data']['TempHighWarning']['value']
            xcvr_dom_thresh_dict['templowalarm'] = module_threshold_data['data']['TempLowAlarm']['value']
            xcvr_dom_thresh_dict['templowwarning'] = module_threshold_data['data']['TempLowWarning']['value']
            xcvr_dom_thresh_dict['vcchighalarm'] = module_threshold_data['data']['VccHighAlarm']['value']
            xcvr_dom_thresh_dict['vcchighwarning'] = module_threshold_data['data']['VccHighWarning']['value']
            xcvr_dom_thresh_dict['vcclowalarm'] = module_threshold_data['data']['VccLowAlarm']['value']
            xcvr_dom_thresh_dict['vcclowwarning'] = module_threshold_data['data']['VccLowWarning']['value']
            xcvr_dom_thresh_dict['rxpowerhighalarm'] = channel_threshold_data['data']['RxPowerHighAlarm']['value']
            xcvr_dom_thresh_dict['rxpowerhighwarning'] = channel_threshold_data['data']['RxPowerHighWarning']['value']
            xcvr_dom_thresh_dict['rxpowerlowalarm'] = channel_threshold_data['data']['RxPowerLowAlarm']['value']
            xcvr_dom_thresh_dict['rxpowerlowwarning'] = channel_threshold_data['data']['RxPowerLowWarning']['value']
            xcvr_dom_thresh_dict['txbiashighalarm'] = channel_threshold_data['data']['TxBiasHighAlarm']['value']
            xcvr_dom_thresh_dict['txbiashighwarning'] = channel_threshold_data['data']['TxBiasHighWarning']['value']
            xcvr_dom_thresh_dict['txbiaslowalarm'] = channel_threshold_data['data']['TxBiasLowAlarm']['value']
            xcvr_dom_thresh_dict['txbiaslowwarning'] = channel_threshold_data['data']['TxBiasLowWarning']['value']
            xcvr_dom_thresh_dict['txpowerhighalarm'] = channel_threshold_data['data']['TxPowerHighAlarm']['value']
            xcvr_dom_thresh_dict['txpowerhighwarning'] = channel_threshold_data['data']['TxPowerHighWarning']['value']
            xcvr_dom_thresh_dict['txpowerlowalarm'] = channel_threshold_data['data']['TxPowerLowAlarm']['value']
            xcvr_dom_thresh_dict['txpowerlowwarning'] = channel_threshold_data['data']['TxPowerLowWarning']['value']

        elif self.sfp_type == QSFPDD_TYPE:
            if self.sfpd_obj is None or not self.dom_supported or not self.dom_thresholds_supported:
                return xcvr_dom_thresh_dict

            module_threshold_data = self._get_eeprom_data('ModuleThreshold')
            if module_threshold_data is None:
                return xcvr_dom_thresh_dict

            # Threshold Data
            xcvr_dom_thresh_dict['temphighalarm'] = module_threshold_data['data']['TempHighAlarm']['value']
            xcvr_dom_thresh_dict['temphighwarning'] = module_threshold_data['data']['TempHighWarning']['value']
            xcvr_dom_thresh_dict['templowalarm'] = module_threshold_data['data']['TempLowAlarm']['value']
            xcvr_dom_thresh_dict['templowwarning'] = module_threshold_data['data']['TempLowWarning']['value']
            xcvr_dom_thresh_dict['vcchighalarm'] = module_threshold_data['data']['VccHighAlarm']['value']
            xcvr_dom_thresh_dict['vcchighwarning'] = module_threshold_data['data']['VccHighWarning']['value']
            xcvr_dom_thresh_dict['vcclowalarm'] = module_threshold_data['data']['VccLowAlarm']['value']
            xcvr_dom_thresh_dict['vcclowwarning'] = module_threshold_data['data']['VccLowWarning']['value']
            xcvr_dom_thresh_dict['rxpowerhighalarm'] = module_threshold_data['data']['RxPowerHighAlarm']['value']
            xcvr_dom_thresh_dict['rxpowerhighwarning'] = module_threshold_data['data']['RxPowerHighWarning']['value']
            xcvr_dom_thresh_dict['rxpowerlowalarm'] = module_threshold_data['data']['RxPowerLowAlarm']['value']
            xcvr_dom_thresh_dict['rxpowerlowwarning'] = module_threshold_data['data']['RxPowerLowWarning']['value']
            xcvr_dom_thresh_dict['txbiashighalarm'] = module_threshold_data['data']['TxBiasHighAlarm']['value']
            xcvr_dom_thresh_dict['txbiashighwarning'] = module_threshold_data['data']['TxBiasHighWarning']['value']
            xcvr_dom_thresh_dict['txbiaslowalarm'] = module_threshold_data['data']['TxBiasLowAlarm']['value']
            xcvr_dom_thresh_dict['txbiaslowwarning'] = module_threshold_data['data']['TxBiasLowWarning']['value']
            xcvr_dom_thresh_dict['txpowerhighalarm'] = module_threshold_data['data']['TxPowerHighAlarm']['value']
            xcvr_dom_thresh_dict['txpowerhighwarning'] = module_threshold_data['data']['TxPowerHighWarning']['value']
            xcvr_dom_thresh_dict['txpowerlowalarm'] = module_threshold_data['data']['TxPowerLowAlarm']['value']
            xcvr_dom_thresh_dict['txpowerlowwarning'] = module_threshold_data['data']['TxPowerLowWarning']['value']

        else:
            logger.log_error("get_transceiver_threshold_info: Unknown module type {} for Ethernet{}".format(
                self.sfp_type, self.index))

        return xcvr_dom_thresh_dict

    def get_name(self):
        """
        Retrieves the name of the sfp
        Returns : QSFP or QSFP+ or QSFP28
        """
        identifier = None
        iface_data = self._get_eeprom_data('type')
        if (iface_data is not None):
            identifier = iface_data['data']['type']['value']
        else:
            return None
        identifier += str(self.index)

        return identifier

    def get_presence(self):
        """
        Retrieves the presence of the sfp
        """
        # logger.log_debug("Get-presence start for SFP {}".format(self.index))

        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_REQ_PRESENCE

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.GetSfpPresence,
                                              platform_ndk_pb2.ReqSfpOpsPb(type=op_type, hw_port_id_begin=self.index))
        nokia_common.channel_shutdown(channel)

        if ret is False:
            return False
        status_msg = response.sfp_status

        if (self.lastPresence != status_msg):
            logger.log_info("get_presence status changed for SFP {} to {}".format(self.index, status_msg))
            self.lastPresence = status_msg
            # really only need to do this on transition to present, but since it's not an expensive operation...
            self.invalidate_page_cache(ALL_PAGES_TYPE)

        return status_msg.status

    def get_model(self):
        """
        Retrieves the model number (or part number) of the sfp
        """
        vendor_pn = None
        vendor_pn_data = self._get_eeprom_data('modelname')
        if (vendor_pn_data is not None):
            vendor_pn = vendor_pn_data['data']['Vendor PN']['value']

        return vendor_pn

    def get_serial(self):
        """
        Retrieves the serial number of the sfp
        """
        vendor_sn_data = self._get_eeprom_data('serialnum')
        if (vendor_sn_data is not None):
            vendor_sn = vendor_sn_data['data']['Vendor SN']['value']
        else:
            return None

        return vendor_sn

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
        self.invalidate_page_cache(ALL_PAGES_TYPE)
        return status_msg.status

    def get_rx_los(self):
        """
        Retrieves the RX LOS (lost-of-signal) status of SFP
        """
        if not self.dom_supported:
            return None

        rx_los_list = []

        if (self.sfp_type == QSFP_TYPE):
            rx_los_data_raw = self._get_eeprom_data('rx_los')
            if rx_los_data_raw is not None:
                rx_los_data = int(rx_los_data_raw[0], 16)
                rx_los_list.append(rx_los_data & 0x01 != 0)
                rx_los_list.append(rx_los_data & 0x02 != 0)
                rx_los_list.append(rx_los_data & 0x04 != 0)
                rx_los_list.append(rx_los_data & 0x08 != 0)
        elif (self.sfp_type == QSFPDD_TYPE):
            if not self.flatmem:
                # might check for rx los implemented previous to reading status
                rx_los_data_raw = self._get_eeprom_data('rx_los')
                if rx_los_data_raw is not None:
                    rx_los_data = int(rx_los_data_raw[0], 16)
                    rx_los_list.append(rx_los_data & 0x01 != 0)
                    rx_los_list.append(rx_los_data & 0x02 != 0)
                    rx_los_list.append(rx_los_data & 0x04 != 0)
                    rx_los_list.append(rx_los_data & 0x08 != 0)
                    rx_los_list.append(rx_los_data & 0x10 != 0)
                    rx_los_list.append(rx_los_data & 0x20 != 0)
                    rx_los_list.append(rx_los_data & 0x40 != 0)
                    rx_los_list.append(rx_los_data & 0x80 != 0)
        else:
            logger.log_error("get_rx_los for Ethernet{} found invalid sfp_type{}".format(self.index, self.sfp_type))
            return None

        return rx_los_list

    def get_tx_fault(self):
        """
        Retrieves the TX fault status of SFP
        """
        if not self.dom_supported:
            return None

        tx_fault_list = []

        if (self.sfp_type == QSFP_TYPE):
            tx_fault_data_raw = self._get_eeprom_data('tx_fault')
            if tx_fault_data_raw is not None:
                tx_fault_data = int(tx_fault_data_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x01 != 0)
                tx_fault_list.append(tx_fault_data & 0x02 != 0)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
                tx_fault_list.append(tx_fault_data & 0x08 != 0)

        elif (self.sfp_type == QSFPDD_TYPE):
            # might check for rx los implemented previous to reading status
            tx_fault_data_raw = self._get_eeprom_data('tx_fault')
            if tx_fault_data_raw is not None:
                tx_fault_data = int(tx_fault_data_raw[0], 16)
                tx_fault_list.append(tx_fault_data & 0x01 != 0)
                tx_fault_list.append(tx_fault_data & 0x02 != 0)
                tx_fault_list.append(tx_fault_data & 0x04 != 0)
                tx_fault_list.append(tx_fault_data & 0x08 != 0)
                tx_fault_list.append(tx_fault_data & 0x10 != 0)
                tx_fault_list.append(tx_fault_data & 0x20 != 0)
                tx_fault_list.append(tx_fault_data & 0x40 != 0)
                tx_fault_list.append(tx_fault_data & 0x80 != 0)
        else:
            logger.log_error("get_tx_fault for Ethernet{} found invalid sfp_type{}".format(self.index, self.sfp_type))
            tx_fault_list = None

        return tx_fault_list

    def get_tx_disable(self):
        """
        Retrieves the tx_disable status of all channels this SFP
        Returns:
            A Boolean, True if tx_disable is enabled, False if disabled
        for QSFP, the disable states of each channel which are the lower 4 bits in byte 85 page a0
        for QSFP-DD, disable states of each channel are in byte 130 page 10

        """
        if not self.dom_supported:
            return None

        if not self.dom_tx_disable_supported:
            return None

        tx_disable_list = []

        if self.sfp_type == QSFP_TYPE:
            # if (self.dom_tx_disable_supported):
            tx_disable_data_raw = self._get_eeprom_data('tx_disable')
            if (tx_disable_data_raw is not None):
                tx_disable_data = int(tx_disable_data_raw[0], 16)
                tx_disable_list.append(tx_disable_data & 0x01 != 0)
                tx_disable_list.append(tx_disable_data & 0x02 != 0)
                tx_disable_list.append(tx_disable_data & 0x04 != 0)
                tx_disable_list.append(tx_disable_data & 0x08 != 0)
        elif (self.sfp_type == QSFPDD_TYPE):
            # if (self.dom_tx_disable_supported) and (self.dom_tx_disable_per_channel_supported):
            tx_disable_data_raw = self._get_eeprom_data('tx_disable')
            if (tx_disable_data_raw is not None):
                tx_disable_data = int(tx_disable_data_raw[0], 16)
                tx_disable_list.append(tx_disable_data & 0x01 != 0)
                tx_disable_list.append(tx_disable_data & 0x02 != 0)
                tx_disable_list.append(tx_disable_data & 0x04 != 0)
                tx_disable_list.append(tx_disable_data & 0x08 != 0)
                tx_disable_list.append(tx_disable_data & 0x10 != 0)
                tx_disable_list.append(tx_disable_data & 0x20 != 0)
                tx_disable_list.append(tx_disable_data & 0x40 != 0)
                tx_disable_list.append(tx_disable_data & 0x80 != 0)
        else:
            logger.log_error("get_tx_disable for Ethernet{} found invalid sfp_type{}".format(
                self.index, self.sfp_type))

        return tx_disable_list

    def get_tx_disable_channel(self):
        """
        Retrieves the TX disabled channels in this SFP
        Returns:
            A hex of bits to represent all
            TX channels which have been disabled in this SFP.
        """
        tx_disable_list = self.get_tx_disable()
        if tx_disable_list is None:
            return 0
        tx_disabled = 0
        for i in range(len(tx_disable_list)):
            if tx_disable_list[i]:
                tx_disabled |= 1 << i
        return tx_disabled

    def get_power_override(self):
        """
        Retrieves the power-override status of this SFP

        Returns:
            A Boolean, True if power-override is enabled, False if disabled
        """
        self.invalidate_page_cache(ALL_PAGES_TYPE)

        if self.sfp_type == QSFP_TYPE:
            if not self.dom_supported or self.sfpd_obj is None:
                return None
            byte_data = self._get_eeprom_data('power_override')
            if byte_data is not None:
                return ('On' == byte_data['data']['PowerOverRide'])
        else:
            return NotImplementedError

    def get_temperature(self):
        """
        Retrieves the temperature of this SFP

        Returns:
            An integer number of current temperature in Celsius
        """
        self.invalidate_page_cache(ALL_PAGES_TYPE)
        temp = None

        if self.sfp_type == QSFP_TYPE:
            if not self.dom_supported or self.sfpd_obj is None:
                return None
            if self.dom_temp_supported:
                dom_temp_data = self._get_eeprom_data('Temperature')
                if dom_temp_data is not None:
                    temp = self._convert_string_to_num(dom_temp_data['data']['Temperature']['value'])

        elif self.sfp_type == QSFPDD_TYPE:
            if self.sfpd_obj is None:
                return None

            if self.dom_temp_supported:
                temp_data = self._get_eeprom_data('temperature')
                if temp_data is not None:
                    temp = self._convert_string_to_num(temp_data['data']['Temperature']['value'])

        return temp

    def get_voltage(self):
        """
        Retrieves the supply voltage of this SFP
        Returns:
            An integer number of supply voltage in mV
        """
        self.invalidate_page_cache(ALL_PAGES_TYPE)
        voltage = None

        if self.sfp_type == QSFP_TYPE:
            if not self.dom_supported or self.sfpd_obj is None:
                return None
            if self.dom_volt_supported:
                dom_voltage_data = self._get_eeprom_data('Voltage')
                if dom_voltage_data is not None:
                    voltage = self._convert_string_to_num(dom_voltage_data['data']['Vcc']['value'])

        elif self.sfp_type == QSFPDD_TYPE:
            if self.sfpd_obj is None:
                return None

            if self.dom_volt_supported:
                volt_data = self._get_eeprom_data('voltage')
                if volt_data is not None:
                    voltage = self._convert_string_to_num(volt_data['data']['Vcc']['value'])

        return voltage

    def get_tx_bias(self):
        """
        Retrieves the TX bias current of this SFP

        Returns:
            A list of integer numbers, representing TX bias power in mA for each channel
            Ex. for channel 0 to channel 4:  ['109.xx', '111.xx', '108.xx', '112.xx']
        """
        tx_bias_list = []
        self.invalidate_page_cache(ALL_PAGES_TYPE)

        if self.sfp_type == QSFP_TYPE:
            if not self.dom_supported or self.sfpd_obj is None:
                return None

            dom_channel_monitor_data = self._get_eeprom_data('ChannelMonitorwtxpwr')
            if dom_channel_monitor_data is not None:
                # logger.log_debug("    ** dom_channel_monitor_data {}".format(dom_channel_monitor_data))
                tx_bias_list.append(self._convert_string_to_num(dom_channel_monitor_data['data']['TX1Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(dom_channel_monitor_data['data']['TX2Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(dom_channel_monitor_data['data']['TX3Bias']['value']))
                tx_bias_list.append(self._convert_string_to_num(dom_channel_monitor_data['data']['TX4Bias']['value']))

        elif self.sfp_type == QSFPDD_TYPE:
            if self.sfpd_obj is None or self.flatmem:
                return None

            if self.dom_tx_bias_power_supported:
                dom_tx_bias_data = self._get_eeprom_data('tx_bias_state')
                if dom_tx_bias_data is not None:
                    # logger.log_debug("       dom_channel_monitor_data {}".format(dom_channel_monitor_data))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX1Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX2Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX3Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX4Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX5Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX6Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX7Bias']['value']))
                    tx_bias_list.append(self._convert_string_to_num(dom_tx_bias_data['data']['TX8Bias']['value']))
                else:
                    return None

        return tx_bias_list

    def get_rx_power(self):
        """
        Retrieves the RX power for this SFP

        Returns:
            A list of integer numbers, representing RX power in mW for each channel
            Ex. for channel 0 to channel 4:  ['1.xx', '1.xx', '1.xx', '1.xx']
        """
        rx_power_list = []
        self.invalidate_page_cache(ALL_PAGES_TYPE)

        if self.sfp_type == QSFP_TYPE:
            if not self.dom_supported or self.sfpd_obj is None:
                return None

            if self.dom_rx_power_supported:
                channel_monitor_data = self._get_eeprom_data('ChannelMonitorwtxpwr')
                if channel_monitor_data is not None:
                    # logger.log_debug("    ** channel_monitor_data {}".format(channel_monitor_data))
                    rx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['RX1Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['RX2Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['RX3Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['RX4Power']['value']))
                else:
                    return None

        elif self.sfp_type == QSFPDD_TYPE:
            if self.sfpd_obj is None or self.flatmem:
                return None

            if self.dom_rx_power_supported:
                dom_rx_power_data = self._get_eeprom_data('rx_pwr_state')
                if dom_rx_power_data is not None:
                    # logger.log_debug("       dom_channel_monitor_data {}".format(dom_channel_monitor_data))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX1Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX2Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX3Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX4Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX5Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX6Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX7Power']['value']))
                    rx_power_list.append(self._convert_string_to_num(dom_rx_power_data['data']['RX8Power']['value']))
                else:
                    return None

        return rx_power_list

    def get_tx_power(self):
        """
        Retrieves the TX power of this SFP

        Returns:
            A list of integer numbers, representing TX power in mW for each channel
            Ex. for channel 0 to channel 4:  ['1.xx', '1.xx', '1.xx', '1.xx']
        """
        tx_power_list = []
        self.invalidate_page_cache(ALL_PAGES_TYPE)

        if self.sfp_type == QSFP_TYPE:
            if not self.dom_supported or self.sfpd_obj is None:
                return None

            if self.dom_tx_power_supported:
                channel_monitor_data = self._get_eeprom_data('ChannelMonitorwtxpwr')
                if channel_monitor_data is not None:
                    # logger.log_debug("    ** channel_monitor_data {}".format(channel_monitor_data))
                    tx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['TX1Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['TX2Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['TX3Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(channel_monitor_data['data']['TX4Power']['value']))
                else:
                    return None

        elif self.sfp_type == QSFPDD_TYPE:
            if self.sfpd_obj is None or self.flatmem:
                return None

            if self.dom_tx_power_supported:
                dom_tx_power_data = self._get_eeprom_data('tx_pwr_state')
                if dom_tx_power_data is not None:
                    # logger.log_debug("       dom_channel_monitor_data {}".format(dom_channel_monitor_data))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX1Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX2Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX3Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX4Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX5Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX6Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX7Power']['value']))
                    tx_power_list.append(self._convert_string_to_num(dom_tx_power_data['data']['TX8Power']['value']))
                else:
                    return None

        return tx_power_list

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
        if ret is False:
            return False
        status_msg = response.sfp_status
        self.invalidate_page_cache(ALL_PAGES_TYPE)
        return status_msg.status

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
        if ret is False:
            return False
        status_msg = response.sfp_status
        self.invalidate_page_cache(ALL_PAGES_TYPE)
        return status_msg.status

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

    def tx_disable(self, tx_disable):
        """
        Disable SFP TX for all channels
        Args:
            tx_disable : A Boolean, True to enable tx_disable mode, False to disable
                         tx_disable mode.
        Returns:
            A boolean, True if tx_disable is set successfully, False if not
        for SFP, make use of bit 6 of byte at (offset 110, a2h (i2c addr 0x51)) to disable/enable tx
        for QSFP, set all channels to disable/enable tx
        """
        op_type = platform_ndk_pb2.ReqSfpOpsType.SFP_OPS_NORMAL

        channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_XCVR_SERVICE)
        if not channel or not stub:
            return False
        ret, response = nokia_common.try_grpc(stub.ReqSfpTxDisable,
                                              platform_ndk_pb2.ReqSfpOpsPb(type=op_type,
                                                                           hw_port_id_begin=self.index,
                                                                           val=tx_disable))
        nokia_common.channel_shutdown(channel)
        if ret is False:
            return False
        status_msg = response.sfp_status
        self.invalidate_page_cache(ALL_PAGES_TYPE)
        return status_msg.status

    def tx_disable_channel(self, channel, disable):
        """
        Sets the tx_disable for specified SFP channels
        Args:
            channel : A hex of 4 bits (bit 0 to bit 3) which represent channel 0 to 3,
                      e.g. 0x5 for channel 0 and channel 2.
            disable : A boolean, True to disable TX channels specified in channel,
                      False to enable
        Returns:
            A boolean, True if successful, False if not
        QSFP: page a0, address 86, lower 4 bits
        """

        return NotImplementedError

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

    def set_power_override(self, power_override, power_set):
        """
        Sets SFP power level using power_override and power_set

        Args:
            power_override :
                    A Boolean, True to override set_lpmode and use power_set
                    to control SFP power, False to disable SFP power control
                    through power_override/power_set and use set_lpmode
                    to control SFP power.
            power_set :
                    Only valid when power_override is True.
                    A Boolean, True to set SFP to low power mode, False to set
                    SFP to high power mode.

        Returns:
            A boolean, True if power-override and power_set are set successfully,
            False if not
        """
        return NotImplementedError

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
