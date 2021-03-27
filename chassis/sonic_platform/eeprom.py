#############################################################################
# Nokia
#
# Module contains an implementation of SONiC Platform Base API and
# provides the eeprom information which are available in the platform
#
#############################################################################
import os

from sonic_py_common.logger import Logger
from platform_ndk import nokia_common
from platform_ndk import platform_ndk_pb2
try:
    from sonic_platform_base.sonic_eeprom import eeprom_tlvinfo
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = Logger()

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
                pass
        cache_file = os.path.join(CACHE_ROOT, CACHE_FILE)
        if not os.path.exists(cache_file):
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
