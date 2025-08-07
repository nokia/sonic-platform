/*
 * Driver for watchdog device controlled through GPIO-line
 *
 * Copyright: 2019, Nokia Networks, Inc 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/acpi.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/kernel.h>
#include <linux/panic_notifier.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/sizes.h>
#include <linux/kprobes.h>

static struct kprobe kp =
{
    .symbol_name = "kallsyms_lookup_name"
};
typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
typedef bool (*nokia_pci_aer_enabled_t)(void);

static nokia_pci_aer_enabled_t nokia_pci_aer_enabled = NULL;

struct nokia_gpio_wdt_priv {
	struct watchdog_device wdd;
};

static int nokia_gpio_wdt_ping(struct watchdog_device *wdd)
{
	acpi_evaluate_object(ACPI_HANDLE(wdd->parent), "KICK", NULL, NULL);

	dev_dbg(wdd->parent, "Watchdog kick\n");
	return 0;
}

static int nokia_gpio_wdt_start(struct watchdog_device *wdd)
{
	set_bit(WDOG_HW_RUNNING, &wdd->status);

	return nokia_gpio_wdt_ping(wdd);
}

static int nokia_gpio_wdt_stop(struct watchdog_device *wdd)
{
	set_bit(WDOG_HW_RUNNING, &wdd->status);

	return 0;
}


void *BAR = NULL;

static int nokia_gpio_wdt_notify_sys(struct notifier_block *this, unsigned long code, void *unused)
{
    if (BAR)
    {
        void *base = (void *) (((unsigned long) BAR) & ~0xF) + 0x807D40;
        void __iomem *io = ioremap((unsigned long) base, SZ_64);
        unsigned int *my_reg = (unsigned int *) io;
        unsigned int read_val, write_val, read_val_again, qdir, qdata;

        // stuff any/all NIF port transceiver modules into reset on typical shutdown, but not on panic/oops (ungraceful shutdown)
        if (code)
        {
            read_val = ioread32be(my_reg);
            iowrite32be(0x0, my_reg);
            read_val_again = ioread32be(my_reg);
            pr_warn("*** shutdown hook with code %lu operating on addr 0x%lx : read 0x%x 0x%x", code, (long unsigned int) my_reg, read_val, read_val_again);

            my_reg = io + 4;
            read_val = ioread32be(my_reg);
            iowrite32be(0x0, my_reg);
            read_val_again = ioread32be(my_reg);
            pr_warn("*** shutdown hook with code %lu operating on addr 0x%lx : read 0x%x 0x%x", code, (long unsigned int) my_reg, read_val, read_val_again);

            my_reg = io + 0x20;
            read_val = ioread32be(my_reg);
            iowrite32be(0xffffffff, my_reg);
            read_val_again = ioread32be(my_reg);
            pr_warn("*** shutdown hook with code %lu operating on addr 0x%lx : read 0x%x 0x%x", code, (long unsigned int) my_reg, read_val, read_val_again);

            my_reg = io + 0x24;
            read_val = ioread32be(my_reg);
            iowrite32be(0xffffffff, my_reg);
            read_val_again = ioread32be(my_reg);
            pr_warn("*** shutdown hook with code %lu operating on addr 0x%lx : read 0x%x 0x%x", code, (long unsigned int) my_reg, read_val, read_val_again);
            iounmap(io);
        }
        else
            pr_warn("*** shutdown hook [skipping FP ports reset] : code is %lu", code);

        base = (void *) (((unsigned long) BAR) & ~0xF) + 0x2700000;
        io = ioremap((unsigned long) base, SZ_128);
        my_reg = (unsigned int *) io;
        read_val = ioread32be(my_reg);
        write_val = read_val & ~0x1;
        iowrite32be(write_val, my_reg);
        read_val_again = ioread32be(my_reg);
        pr_warn("*** shutdown hook operating on addr 0x%lx : read 0x%x 0x%x", (long unsigned int) my_reg, read_val, read_val_again);

        my_reg = io + 0x54;
        read_val = ioread32be(my_reg);
        read_val |= 0x20000000;
        iowrite32be(read_val, my_reg);
        qdir = ioread32be(my_reg);
        pr_warn("*** shutdown hook operating on addr 0x%lx : read 0x%x qdir 0x%x", (long unsigned int) my_reg, read_val, qdir);

        my_reg = io + 0x50;
        read_val = ioread32be(my_reg);

        // determine dynamically whether global kernel AER is in play and stuff QFPGA into reset if safe to do
        if ((nokia_pci_aer_enabled) && (!nokia_pci_aer_enabled()))
        {
           qdata = read_val & ~0x20000000;
           iowrite32be(qdata, my_reg);
           read_val_again = ioread32be(my_reg);
           pr_warn("*** kernel PCIe AER is disabled - shutdown hook operating on addr 0x%lx : orig_read 0x%x qdata 0x%x read_again 0x%x", (long unsigned int) my_reg, read_val, qdata, read_val_again);
        }
        else
        {
           pr_warn("*** kernel PCIe AER is enabled - shutdown hook [skipping QFPGA reset] addr 0x%lx : orig_read 0x%x", (long unsigned int) my_reg, read_val);
        }
        iounmap(io);
    }
    else
    {
        pr_info("Not IMM!\n");
    }
    return NOTIFY_DONE;
}

static struct notifier_block nokia_gpio_wdt_notifier = {
	.notifier_call = nokia_gpio_wdt_notify_sys,
};

static const struct watchdog_info nokia_gpio_wdt_info = {
	.identity = "Nokia GPIO Watchdog",
};

static const struct watchdog_ops nokia_gpio_wdt_ops = {
	.owner = THIS_MODULE,
	.start = nokia_gpio_wdt_start,
	.stop  = nokia_gpio_wdt_stop,
	.ping  = nokia_gpio_wdt_ping,
};

static int nokia_gpio_wdt_probe(struct platform_device *pdev)
{
	struct nokia_gpio_wdt_priv *priv;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

	priv->wdd.parent = &pdev->dev;
	priv->wdd.info   = &nokia_gpio_wdt_info;
	priv->wdd.ops    = &nokia_gpio_wdt_ops;

    watchdog_set_drvdata(&priv->wdd, priv);
    watchdog_set_nowayout(&priv->wdd, WATCHDOG_NOWAYOUT);

	ret = watchdog_register_device(&priv->wdd);
	if (ret)
		return ret;

	dev_info(&pdev->dev, "Watchdog enabled\n");
	return 0;
}

static void nokia_gpio_wdt_remove(struct platform_device *pdev)
{
	struct nokia_gpio_wdt_priv *priv = platform_get_drvdata(pdev);

	watchdog_unregister_device(&priv->wdd);

}

static const struct acpi_device_id nokia_gpio_wdt_acpi_match[] = {
	{ "WDOG0001", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, nokia_gpio_wdt_acpi_match);

static struct platform_driver nokia_gpio_wdt_driver = {
	.driver = {
		.name             = "nokia_gpio_wdt",
		.acpi_match_table = ACPI_PTR(nokia_gpio_wdt_acpi_match),
	},
	.probe  = nokia_gpio_wdt_probe,
	.remove = nokia_gpio_wdt_remove,
};

static int __init nokia_gpio_wdt_init_driver(void)
{
    int ret;
    struct pci_dev *pdev = NULL;

    ret = register_reboot_notifier(&nokia_gpio_wdt_notifier);
    if (ret)
        pr_err("cannot register reboot notifier (err=%d)\n", ret);
    atomic_notifier_chain_register(&panic_notifier_list, &nokia_gpio_wdt_notifier);
    pdev = pci_get_device(0x1064, 0x001a, NULL);
    if (pdev)
    {
       BAR = (void *) pdev->resource[0].start;
       pr_warn("IOCTL BAR is 0x%lx\n", (unsigned long) BAR);
    }
    else
    {
       pr_warn("cannot locate IOCTL device!\n");
    }

    kallsyms_lookup_name_t kallsyms_lookup_name;
    register_kprobe(&kp);
    kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
    unregister_kprobe(&kp);
    nokia_pci_aer_enabled = (nokia_pci_aer_enabled_t) kallsyms_lookup_name("pci_aer_available");

    if (nokia_pci_aer_enabled)
       pr_info("nokia_gpio_wdt:  pci_aer_available() found at %p\n", (void *) nokia_pci_aer_enabled);
    else
       pr_warn("nokia_gpio_wdt:  could not locate pci_aer_available()\n");

    // crash_kexec_post_notifiers = true;
    return platform_driver_register(&nokia_gpio_wdt_driver);
}
module_init(nokia_gpio_wdt_init_driver);

static void __exit nokia_gpio_wdt_exit_driver(void)
{
    unregister_reboot_notifier(&nokia_gpio_wdt_notifier);
    atomic_notifier_chain_unregister(&panic_notifier_list, &nokia_gpio_wdt_notifier);
    platform_driver_unregister(&nokia_gpio_wdt_driver);
}


module_exit(nokia_gpio_wdt_exit_driver);

MODULE_AUTHOR("SR Sonic <sr-sonic@nokia.com>");
MODULE_DESCRIPTION("Nokia GPIO Watchdog");
MODULE_LICENSE("GPL");
