"""
    Nokia 7220 IXR H5-64D

    Module contains platform specific implementation of SONiC Platform
    Base API and provides the EEPROMs' information.

    The different EEPROMs available are as follows:
      - System EEPROM : Contains Serial number, Service tag, Base MA
                         address, etc. in ONIE TlvInfo EEPROM format.
"""

try:
    from sonic_platform_base.sonic_eeprom.eeprom_tlvinfo import TlvInfoDecoder
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError(str(e) + ' - required module not found') from e

sonic_logger = logger.Logger('eeprom')

class Eeprom(TlvInfoDecoder):
    """Nokia platform-specific EEPROM class"""

    I2C_DIR = "/sys/bus/i2c/devices/"

    def __init__(self, is_psu, psu_index, is_fan, drawer_index):
        self.is_psu_eeprom = is_psu
        self.is_fan_eeprom = is_fan
        self.is_sys_eeprom = not (is_psu | is_fan)
        self.service_tag = 'NA'
        self.part_number = 'NA'

        if self.is_sys_eeprom:
            self.start_offset = 0
            self.eeprom_path = self.I2C_DIR + "4-0054/eeprom"
            # System EEPROM is in ONIE TlvInfo EEPROM format
            super(Eeprom, self).__init__(self.eeprom_path, self.start_offset, '', True)
            self.base_mac = ''
            self.serial_number = ''
            self.part_number = ''
            self.model_str = ''
            self.service_tag = ''
        else:
            self.serial_number = 'N/A'
            self.part_number = 'N/A'
            self.model_str = 'N/A'
            self.service_tag = 'N/A'

    def _load_system_eeprom(self):
        """
        Reads the system EEPROM and retrieves the values corresponding
        to the codes defined as per ONIE TlvInfo EEPROM format and fills
        them in a dictionary.
        """
        try:
            # Read System EEPROM as per ONIE TlvInfo EEPROM format.
            self.eeprom_data = self.read_eeprom()
        except Exception as e:
            sonic_logger.log_warning("Unable to read system eeprom")
            self.base_mac = 'NA'
            self.serial_number = 'NA'
            self.part_number = 'NA'
            self.model_str = 'NA'
            self.service_tag = 'NA'
            self.eeprom_tlv_dict = dict()
        else:
            eeprom = self.eeprom_data
            self.eeprom_tlv_dict = dict()

            if not self.is_valid_tlvinfo_header(eeprom):
                sonic_logger.log_warning("Invalid system eeprom TLV header")
                self.base_mac = 'NA'
                self.serial_number = 'NA'
                self.part_number = 'NA'
                self.model_str = 'NA'
                self.service_tag = 'NA'
                return

            total_length = (eeprom[9] << 8) | eeprom[10]
            tlv_index = self._TLV_INFO_HDR_LEN
            tlv_end = self._TLV_INFO_HDR_LEN + total_length

            while (tlv_index + 2) < len(eeprom) and tlv_index < tlv_end:
                if not self.is_valid_tlv(eeprom[tlv_index:]):
                    break

                tlv = eeprom[tlv_index:tlv_index + 2
                             + eeprom[tlv_index + 1]]
                code = "0x%02X" % (tlv[0])

                name, value = self.decoder(None, tlv)

                self.eeprom_tlv_dict[code] = value
                if eeprom[tlv_index] == self._TLV_CODE_CRC_32:
                    break

                tlv_index += eeprom[tlv_index+1] + 2

            self.base_mac = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_MAC_BASE), 'NA')
            self.serial_number = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_SERIAL_NUMBER), 'NA')
            self.part_number = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_PART_NUMBER), 'NA')
            self.model_str = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_PRODUCT_NAME), 'NA')
            self.service_tag = self.eeprom_tlv_dict.get(
                "0x%X" % (self._TLV_CODE_SERVICE_TAG), 'NA')

    def _get_eeprom_field(self, field_name):
        """
        For a field name specified in the EEPROM format, returns the
        presence of the field and the value for the same.
        """
        field_start = 0
        for field in self.format:
            field_end = field_start + field[2]
            if field[0] == field_name:
                return (True, self.eeprom_data[field_start:field_end])
            field_start = field_end

        return (False, None)

    def serial_number_str(self):
        """
        Returns the serial number.
        """
        if not self.serial_number:
            self._load_system_eeprom()

        return self.serial_number

    def part_number_str(self):
        """
        Returns the part number.
        """
        if not self.part_number:
            self._load_system_eeprom()

        return self.part_number

    def airflow_fan_type(self):
        """
        Returns the airflow fan type.
        """
        if self.is_psu_eeprom:
            return int(self.psu_type.encode('hex'), 16)
        if self.is_fan_eeprom:
            return int(self.fan_type.encode('hex'), 16)

    def base_mac_addr(self):
        """
        Returns the base MAC address found in the system EEPROM.
        """
        if not self.base_mac:
            self._load_system_eeprom()

        return self.base_mac

    def modelstr(self):
        """
        Returns the Model name.
        """
        if not self.model_str:
            self._load_system_eeprom()

        return self.model_str

    def service_tag_str(self):
        """
        Returns the servicetag number.
        """
        return self.service_tag

    def system_eeprom_info(self):
        """
        Returns a dictionary, where keys are the type code defined in
        ONIE EEPROM format and values are their corresponding values
        found in the system EEPROM.
        """
        return self.eeprom_tlv_dict
