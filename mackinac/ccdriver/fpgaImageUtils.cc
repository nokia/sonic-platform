/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <linux/swab.h>
#include <fcntl.h>
#include <time.h>
#include "fpgaImageUtils.h"
#include "tmFlash.h"
#include "std_types.h"
using namespace srlinux;
using namespace srlinux::platform;
extern "C" {
static int readAll(int fd, UCHAR* p, UINT total)
{
    UINT cnt = 0;
    UINT left;
    while ((left = (total - cnt)) != 0)
    {
        int len = read(fd, (char*)p, left);
        if (len < 0)
            return (-1);
        else if (len == 0)
            break;
        p += len;
        cnt += len;
    }
    return cnt;
}
SrlStatus getBitfile( char *filename,
                         const char *fpganame,
                         char **bitfile,
                         tPromHeader *pHeader,
                         uint32_t *pVersion)
{
    char * fpgabits;
    int fpgaVersion = -1;
    uint32_t fpgaSize;
    uint8_t offset = 0;
    uint16_t pgsize = 256;
    if (pHeader)
        offset = (sizeof(tPromHeader));
    if (pVersion)
        *pVersion = (uint32_t) fpgaVersion;
    {
        int fd;
        printf("Looking for file %s ...\n", filename);
        fd = open(filename, O_RDONLY);
        if (fd == (-1))
        {
            printf("Can't find file\n");
            return((-1));
        }
        fpgabits = (char *)malloc((42 * 1024 * 1024)+pgsize);
        if (fpgabits == NULL)
        {
            printf("ERROR: Could not allocate %u bytes for .bit file.\n", (42 * 1024 * 1024));
            close(fd);
            return (-1);
        }
        printf("Reading file ... \n");
        fpgaSize = readAll(fd, (UCHAR*)(fpgabits+offset), (42 * 1024 * 1024));
        close(fd);
        if ((fpgaSize < 1024) || ((fpgaSize+offset) >= (42 * 1024 * 1024)))
        {
            printf("ERROR: could not read file or bad file (len=%u)\n", fpgaSize);
            free(fpgabits);
            return (-1);
        }
        printf("%d bytes read", fpgaSize);
    }
    printf("New FPGA image:       %s\n", fpganame);
    printf("New FPGA size:        0x%02X\n", fpgaSize);
    if (fpgaVersion >= 0)
        printf("New FPGA version:     0x%02X\n", fpgaVersion);
    *bitfile = fpgabits;
    if (pHeader)
    {
        pHeader->size = fpgaSize;
        pHeader->u.data32 = (uint32_t) fpgaVersion;
    }
    if (pVersion)
        *pVersion = (uint32_t) fpgaVersion;
    return 0;
}
}
