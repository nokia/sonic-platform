#!/usr/bin/env python

import struct
from os import *
from mmap import *

RESOURCE = "/sys/bus/pci/devices/0000:02:00.0/resource0"
REG1 = [0x2100, 0x2120, 0x2140, 0x2160, 0x2180, 0x21A0, 0x21C0, 0x21E0, 0x2200, 0x2220, 0x2240, 0x2260, 0x2280, 0x22A0, 0x22C0, 0x22E0]
REG2 = [0x2300, 0x2320, 0x2340, 0x2360, 0x2380, 0x23A0, 0x23C0, 0x23E0, 0x2400, 0x2420, 0x2440, 0x2460, 0x2480, 0x24A0, 0x24C0, 0x24E0, 0x2500, 0x2520]

def pci_set_data8(resource, val, offset):
    fd = open(resource, O_RDWR)
    mm = mmap(fd, 0)
    mm.seek(offset)
    mm.write(struct.pack('B', val))
    mm.close()
    close(fd)

def main():
    for p in range(5):
        pci_set_data8(RESOURCE, p+1, 0x2f00)
        #print(f"SPI_MUX: set mux to {(p+1)}:\n\n")
        for i in range(len(REG1)):
            pci_set_data8(RESOURCE, 0x80, REG1[i]+8)
            pci_set_data8(RESOURCE, 0x0B, REG1[i])
            pci_set_data8(RESOURCE, 0x0, REG1[i]+4)
        if p == 0:
            for i in range(len(REG2)):
                pci_set_data8(RESOURCE, 0x80, REG2[i]+8)
                pci_set_data8(RESOURCE, 0x0B, REG2[i])
                pci_set_data8(RESOURCE, 0x0, REG2[i]+4)

    return


if __name__ == '__main__':
    main()
    