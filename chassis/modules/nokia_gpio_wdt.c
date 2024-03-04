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

static int nokia_gpio_wdt_remove(struct platform_device *pdev)
{
	struct nokia_gpio_wdt_priv *priv = platform_get_drvdata(pdev);

	watchdog_unregister_device(&priv->wdd);

	return 0;
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
	return platform_driver_register(&nokia_gpio_wdt_driver);
}
module_init(nokia_gpio_wdt_init_driver);

static void __exit nokia_gpio_wdt_exit_driver(void)
{
	platform_driver_unregister(&nokia_gpio_wdt_driver);
}
module_exit(nokia_gpio_wdt_exit_driver);

MODULE_AUTHOR("SR Sonic <sr-sonic@nokia.com>");
MODULE_DESCRIPTION("Nokia GPIO Watchdog");
MODULE_LICENSE("GPL");
