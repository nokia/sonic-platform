/**********************************************************************************************************************
 * Copyright (c) 2025 Nokia
 ***********************************************************************************************************************/
#pragma once

extern "C" {
typedef enum
{
    CTL_FPGA_CPUCTL = 0,
    CTL_FPGA_IOCTL,
    CTL_FPGA_MDA_FIRST,
    CTL_FPGA_MDA_1 = CTL_FPGA_MDA_FIRST,
    CTL_FPGA_MDA_2,
    CTL_FPGA_MDA_3,
    CTL_FPGA_MDA_4,
    CTL_FPGA_MDA_5,
    CTL_FPGA_MDA_6,
    CTL_FPGA_MDA_LAST = CTL_FPGA_MDA_6,
    CTL_FPGA_H4_SMB,
    CTL_FPGA_H4_UDB,
    CTL_FPGA_H4_LDB,
    CTL_FPGA_H4_32D,
    CTL_FPGA_H5,
    CTL_FPGA_MAX,
    CTL_FPGA_DEFAULT,
    CTL_FPGA_ALL,
    CTL_FPGA_NONE = 0xff,
} CtlFpgaId;
CtlFpgaId ctl_fpga_id_default(void);
const char *ctl_fpga_name(CtlFpgaId fpga_id);
}
