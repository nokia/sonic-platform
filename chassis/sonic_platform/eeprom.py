#############################################################################
# Nokia
#
# Module contains an implementation of SONiC Platform Base API and
# provides the eeprom information which are available in the platform
#
#############################################################################
import os
import struct
import binascii

from sonic_py_common.logger import Logger
from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
try:
    from sonic_platform_base.sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger()

# ONIE TlvInfo constants
TLV_INFO_HEADER = b"TlvInfo"
TLV_INFO_VERSION = 0x01
TLV_CRC32_TYPE = 0xFE

#
# CACHE_XXX stuffs are supposted to be moved to the base classes
# since they are common for all vendors
# they are defined in decode-syseeprom which might be removed in the future
# currently we just copy them here
#
CACHE_ROOT = '/var/cache/sonic/decode-syseeprom'
CACHE_FILE = 'syseeprom_cache'


class Eeprom(eeprom_tlvinfo.TlvInfoDecoder):
    def __init__(self):
        cache_file = os.path.join(CACHE_ROOT, CACHE_FILE)
        if not os.path.exists(CACHE_ROOT):
            try:
                os.makedirs(CACHE_ROOT)
            except Exception as e:
                # if there is a permission issue to create directory,
                # use /tmp to instead
                cache_file = os.path.join("/tmp", CACHE_FILE)
                pass

        if not os.path.exists(cache_file) or not self.validate_eeprom_cache(cache_file):
            channel, stub = nokia_common.channel_setup(nokia_common.NOKIA_GRPC_EEPROM_SERVICE)
            if not channel or not stub:
                raise RuntimeError(str(e) + "Unable to fetch eeprom info from platform-ndk")
                return
            response = stub.GetCardEepromAllTlvs(platform_ndk_pb2.ReqEepromInfoPb())
            nokia_common.channel_shutdown(channel)

            eeprom_filearray = bytearray(response.data)
            eeprom_file = open(cache_file, "wb")
            eeprom_file.write(eeprom_filearray)
            eeprom_file.close()

        if not os.path.exists(cache_file):
            logger.log_error("Nowhere to read syseeprom from! File not exists or cache file found")
            return

        self.eeprom_path = cache_file
        super(Eeprom, self).__init__(self.eeprom_path, 0, '', True)

        try:
            self.eeprom_data = self.read_eeprom()
        except Exception as e:
            self.eeprom_data = "N/A"
            raise RuntimeError(str(e) + "Eeprom is not Programmed")
        else:
            eeprom = self.eeprom_data

            if not self.is_valid_tlvinfo_header(eeprom):
                return

            self._eeprom_tlv_dict = dict()
            total_length = (eeprom[9] << 8) | eeprom[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % tlv[0]

                if tlv[0] == self._TLV_CODE_VENDOR_EXT:
                    value = str((tlv[2] << 24) | (tlv[3] << 16) |
                                (tlv[4] << 8) | tlv[5])
                    value += tlv[6:6 + tlv[1]].decode('ascii')
                else:
                    name, value = self.decoder(None, tlv)

                self._eeprom_tlv_dict[code] = value
                if eeprom[tlv_index] == self._TLV_CODE_CRC_32:
                    break

                tlv_index += eeprom[tlv_index+1] + 2

    def validate_eeprom_cache(self, file_path):
        if not os.path.exists(file_path):
            return False
        try:
            with open(file_path, "rb") as f:
                data = f.read()
        except IOError as e:
            logger.log_error("Unable to read file {}".format(file_path))
            return False

        # Minimum size check: header (8) + version (1) + length (2) + CRC TLV (6)
        if len(data) < 17:
            logger.log_warning("File {} too short to be a valid ONIE EEPROM".format(file_path))
            return False

        # Check magic header
        if data[0:7] != TLV_INFO_HEADER:
            logger.log_warning("Missing 'TlvInfo' header")
            return False

        # Check version
        version = data[8]
        if version != TLV_INFO_VERSION:
            logger.log_warning("Unsupported TLV version {}. Expected {}".format(version, TLV_INFO_VERSION))
            return False

        # Read total length (big-endian)
        total_len = struct.unpack(">H", data[9:11])[0]
        if total_len + 11 != len(data):
            logger.log_warning("Length mismatch!")
            return False

        # Parse TLVs
        offset = 11
        crc_expected = None
        tlv_count = 0

        while offset < len(data):
            if offset + 2 > len(data):
                logger.log_warning("TLV entry truncated.")
                return False

            tlv_type = data[offset]
            tlv_len = data[offset + 1]
            offset += 2

            if offset + tlv_len > len(data):
                logger.log_warning("TLV value exceeds file size.")
                return False

            tlv_value = data[offset:offset + tlv_len]
            offset += tlv_len
            tlv_count += 1
            if tlv_type == TLV_CRC32_TYPE:
                if tlv_len != 4:
                    logger.log_warning("CRC32 TLV length must be 4.")
                    return False
                crc_expected = struct.unpack(">I", tlv_value)[0]

        # CRC check
        if crc_expected is None:
            logger.log_warning("Missing CRC32 TLV.")
            return False

        crc_calc = binascii.crc32(data[0:-4]) & 0xFFFFFFFF
        if crc_calc != crc_expected:
            logger.log_warning("CRC32 mismatch: expected 0x{}, got 0x{}".format(hex(crc_expected), hex(crc_calc)))
            return False
        return True

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        (is_valid, t) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_MAC_BASE)
        if not is_valid or t[1] != 6:
            return super(eeprom_tlvinfo.TlvInfoDecoder, self).switchaddrstr(e)
        return ":".join(["{:02x}".format(T) for T in t[2]]).upper()

    def get_serial_number(self):
        """
        Retrieves the hardware serial number for the chassis

        Returns:
            A string containing the hardware serial number for this chassis.
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_SERIAL_NUMBER)
        if not is_valid:
            return "N/A"
        return results[2].decode('ascii')

    def get_product_name(self):
        """
        Retrieves the hardware product name for the chassis

        Returns:
            A string containing the hardware product name for this chassis.
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_PRODUCT_NAME)
        if not is_valid:
            return "N/A"
        return results[2].decode('ascii')

    def get_part_number(self):
        """
        Retrieves the hardware part number for the chassis

        Returns:
            A string containing the hardware part number for this chassis.
        """
        (is_valid, results) = self.get_tlv_field(self.eeprom_data, self._TLV_CODE_PART_NUMBER)
        if not is_valid:
            return "N/A"
        return results[2].decode('ascii')

    def get_system_eeprom_info(self):
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
        """
        return self._eeprom_tlv_dict

    def reset(self):
        """Remove the syseeprom.bin file to trigger the GRPC to get the file again
        """
        cache_file = os.path.join(CACHE_ROOT, CACHE_FILE)
        if os.path.exists(cache_file):
            os.remove(cache_file)
