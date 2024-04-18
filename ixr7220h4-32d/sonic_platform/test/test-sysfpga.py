#!/usr/bin/env python

try:
    import struct
    from os import *
    from mmap import *
    import sonic_platform.platform
    import sonic_platform.chassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

RESOURCE = "/sys/bus/pci/devices/0000:02:00.0/resource0"
REG_SCRATCH = 0x0000
REG_CODE_REV0 = 0x0004
REG_CODE_REV1 = 0x0008
REG_BOARD_TYPE = 0x000C
TEST_VAL = 0x5A5AA5A5

def pci_set_value(resource, data, offset):
    fd = open(resource, O_RDWR)
    mm = mmap(fd, 0)
    mm.seek(offset)
    mm.write(struct.pack('I', data))
    mm.close()
    close(fd)

def pci_get_value(resource, offset):
    fd = open(resource, O_RDWR)
    mm = mmap(fd, 0)
    mm.seek(offset)
    read_data_stream = mm.read(4)
    reg_val = struct.unpack('I', read_data_stream)
    mm.close()
    close(fd)
    return reg_val

def main():
    print("-----------------------------")
    print("Get sysFPGA info and r/w test")
    print("-----------------------------")

#   pci_set_value(RESOURCE, val, offset)

#   pci_get_value(RESOURCE, offset)
    
    val = pci_get_value(RESOURCE, REG_CODE_REV1)
    code_year = (val[0] & 0xFF000000) >> 24
    code_month = (val[0] & 0xFF0000) >> 16
    code_day = (val[0] & 0xFF00) >> 8
    print("  sysFPA realse date: {}-{}-{}\n".format(code_year, code_month, code_day))

    val = pci_get_value(RESOURCE, REG_CODE_REV0)    
    code_rev = val[0] & 0xFF 
    print("  Code Revision: {}\n".format(hex(code_rev)))

    val = pci_get_value(RESOURCE, REG_BOARD_TYPE)    
    platform_type = (val[0] & 0xF0) >> 4
    board_type = val[0] & 0xF
    print("  Platform type: {}\n".format(hex(platform_type)))
    print("  Board type: {}\n".format(hex(board_type)))

    val = pci_get_value(RESOURCE, REG_SCRATCH)    
    scratch = val[0] 
    print("  Scratch register: {}\n".format(hex(scratch)))
    pci_set_value(RESOURCE, int(TEST_VAL, 16), REG_SCRATCH)
    print("  Writing {} into Scratch register...\n".format(hex(TEST_VAL)))
    val = pci_get_value(RESOURCE, REG_SCRATCH)    
    scratch = val[0] 
    print("  Scratch register: {}\n".format(hex(scratch)))



    return


if __name__ == '__main__':
    main()
