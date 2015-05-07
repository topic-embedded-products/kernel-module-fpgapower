/*
 * Copyright (C) 2014 Topic Embedded Products
 * Author: Mike Looijmans <mike.looijmans@topic.nl>
 *
 * Licensed under the GPL-2.
 *
 * Dummy power supply that just provides a supply that becomes active
 * when this module is loaded. This allows built-in devices (e.g. an
 * SDIO Wifi card) to postpone probing until the FPGA logic has been
 * activated.
 *
 * Based on "fixed.c" regulator.
 */

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>

struct fpga_voltage_data {
	struct regulator_desc desc;
	struct regulator_dev *dev;
};

static struct regulator_ops fpga_voltage_ops = {
};

static int reg_fpga_voltage_probe(struct platform_device *pdev)
{
	struct fpga_voltage_data *drvdata;
	struct regulator_config cfg = { };
	struct regulator_init_data *init_data;
	int ret;

	drvdata = devm_kzalloc(&pdev->dev, sizeof(struct fpga_voltage_data),
			       GFP_KERNEL);
	if (drvdata == NULL) {
		dev_err(&pdev->dev, "Failed to allocate device data\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.type = REGULATOR_VOLTAGE;
	drvdata->desc.owner = THIS_MODULE;
	drvdata->desc.ops = &fpga_voltage_ops;

	init_data = of_get_regulator_init_data(&pdev->dev,
		pdev->dev.of_node, &drvdata->desc);
	if (!init_data) {
		dev_err(&pdev->dev, "Failed to get init_data\n");
		ret = -EINVAL;
		goto err;
	}

	drvdata->desc.name = kstrdup(init_data->constraints.name, GFP_KERNEL);
	if (drvdata->desc.name == NULL) {
		dev_err(&pdev->dev, "Failed to allocate supply name\n");
		ret = -ENOMEM;
		goto err;
	}

	drvdata->desc.fixed_uV = init_data->constraints.min_uV;

	cfg.dev = &pdev->dev;
	cfg.init_data = init_data;
	cfg.driver_data = drvdata;
	cfg.of_node = pdev->dev.of_node;

	drvdata->dev = regulator_register(&drvdata->desc, &cfg);
	if (IS_ERR(drvdata->dev)) {
		ret = PTR_ERR(drvdata->dev);
		dev_err(&pdev->dev, "Failed to register regulator: %d\n", ret);
		goto err_name;
	}

	platform_set_drvdata(pdev, drvdata);

	dev_dbg(&pdev->dev, "%s supplying %duV\n", drvdata->desc.name,
		drvdata->desc.fixed_uV);

	return 0;

err_name:
	kfree(drvdata->desc.name);
err:
	return ret;
}

static int reg_fpga_voltage_remove(struct platform_device *pdev)
{
	struct fpga_voltage_data *drvdata = platform_get_drvdata(pdev);

	regulator_unregister(drvdata->dev);
	kfree(drvdata->desc.name);

	return 0;
}

#if defined(CONFIG_OF)
static const struct of_device_id fpga_of_match[] = {
	{ .compatible = "regulator-fpga", },
	{},
};
MODULE_DEVICE_TABLE(of, fpga_of_match);
#endif

static struct platform_driver regulator_fpga_voltage_driver = {
	.probe		= reg_fpga_voltage_probe,
	.remove		= reg_fpga_voltage_remove,
	.driver		= {
		.name		= "reg-fpga-voltage",
		.owner		= THIS_MODULE,
		.of_match_table = of_match_ptr(fpga_of_match),
	},
};

static int __init regulator_fpga_voltage_init(void)
{
	return platform_driver_register(&regulator_fpga_voltage_driver);
}
subsys_initcall(regulator_fpga_voltage_init);

static void __exit regulator_fpga_voltage_exit(void)
{
	platform_driver_unregister(&regulator_fpga_voltage_driver);
}
module_exit(regulator_fpga_voltage_exit);

MODULE_AUTHOR("Mike Looijmans <mike.looijmans@topic.nl>");
MODULE_DESCRIPTION("Indicates that the FPGA is up and running");
MODULE_LICENSE("GPL v2");
