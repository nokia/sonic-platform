#include <linux/device.h>
#include <linux/delay.h>

#include "cpuctl.h"

#define A32_CP_SETS_SELECT                  0x00800474
#define   B_SETS_SELECT_ENABLE_SM            0x10000000
#define   B_SETS_SELECT_FORCE_MASTER         0x08000000

void ctl_clk_reset(CTLDEV * pdev)
{
    u32 val;
    val = ctl_reg_read(pdev,A32_CP_MISCIO1_DATA);
    ctl_reg_write(pdev, A32_CP_MISCIO1_DATA, val &= ~MISCIO1_CP_VERM_SETS_RST_BIT);

    msleep(10);

    ctl_reg_write(pdev,A32_CP_MISCIO1_DATA, val |= MISCIO1_CP_VERM_SETS_RST_BIT);
    ctl_reg_read(pdev,A32_CP_MISCIO1_DATA);

    msleep(2000);

    dev_info (&pdev->pcidev->dev,"SETS ssm init (master)");

    val = ctl_reg_read(pdev,A32_CP_SETS_SELECT);
    val |= B_SETS_SELECT_FORCE_MASTER | B_SETS_SELECT_ENABLE_SM;
    ctl_reg_write(pdev, A32_CP_SETS_SELECT, val );
}


