// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for I2C connected Hynitron CST353X Touchscreen
 *
 * Based on the driver supplied by GL.iNet.
 */
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/touchscreen.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/unaligned.h>

#define CST353X_FRAME_REG	0xd0070000
#define CST353X_END_REG		0xd00002ab

#define CST353X_FRAME_LEN	9
#define CST353X_CHECKSUM_SEED	0x55

#define CST353X_NATIVE_MAX_X	240
#define CST353X_NATIVE_MAX_Y	320

struct cst353x_priv {
	struct device *dev;
	struct i2c_client *client;
	struct gpio_desc *reset;
	struct input_dev *input;
	struct touchscreen_properties prop;

	bool touched;
	unsigned int abs_x;
	unsigned int abs_y;

	u8 rxtx[CST353X_FRAME_LEN];
};

static int cst353x_register_read(struct cst353x_priv *priv, u32 reg)
{
	struct i2c_client *client = priv->client;
	struct i2c_msg xfer[2];
	u8 reg_buf[4];
	int rc;

	put_unaligned_be32(reg, reg_buf);

	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = sizeof(reg_buf);
	xfer[0].buf = reg_buf;

	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = sizeof(priv->rxtx);
	xfer[1].buf = priv->rxtx;

	rc = i2c_transfer(client->adapter, xfer, ARRAY_SIZE(xfer));
	if (rc != ARRAY_SIZE(xfer)) {
		rc = rc < 0 ? rc : -EIO;
		dev_err(&client->dev, "i2c rx err: %d\n", rc);
		return rc;
	}

	return 0;
}

static int cst353x_register_write(struct cst353x_priv *priv, u32 reg)
{
	struct i2c_client *client = priv->client;
	struct i2c_msg xfer;
	u8 reg_buf[4];
	int rc;

	put_unaligned_be32(reg, reg_buf);

	xfer.addr = client->addr;
	xfer.flags = 0;
	xfer.len = sizeof(reg_buf);
	xfer.buf = reg_buf;

	rc = i2c_transfer(client->adapter, &xfer, 1);
	if (rc != 1) {
		rc = rc < 0 ? rc : -EIO;
		dev_err(&client->dev, "i2c tx err: %d\n", rc);
		return rc;
	}

	return 0;
}

static int cst353x_frame_read(struct cst353x_priv *priv)
{
	u16 checksum;
	u8 *raw;
	int rc;

	rc = cst353x_register_read(priv, CST353X_FRAME_REG);
	if (rc)
		return rc;

	raw = priv->rxtx;
	checksum = CST353X_CHECKSUM_SEED + raw[4] + raw[5] + raw[6] + raw[7] + raw[8];
	if ((raw[0] | (raw[1] << 8)) != checksum)
		return -EBADMSG;

	priv->touched = (raw[8] >> 4) != 0;
	priv->abs_x = CST353X_NATIVE_MAX_X - (raw[4] + ((raw[7] & 0x0f) << 8));
	priv->abs_y = raw[5] + ((raw[7] & 0xf0) << 4);

	return 0;
}

static irqreturn_t cst353x_irq_cb(int irq, void *cookie)
{
	struct cst353x_priv *priv = cookie;

	if (!cst353x_frame_read(priv)) {
		if (priv->touched)
			touchscreen_report_pos(priv->input, &priv->prop,
					       priv->abs_x, priv->abs_y, false);

		input_report_key(priv->input, BTN_TOUCH, priv->touched);
		input_sync(priv->input);
	}

	cst353x_register_write(priv, CST353X_END_REG);

	return IRQ_HANDLED;
}

static int cst353x_register_input(struct cst353x_priv *priv)
{
	priv->input = devm_input_allocate_device(priv->dev);
	if (!priv->input)
		return -ENOMEM;

	priv->input->name = "Hynitron CST353X Touchscreen";
	priv->input->phys = "input/ts";
	priv->input->id.bustype = BUS_I2C;
	input_set_drvdata(priv->input, priv);

	input_set_capability(priv->input, EV_KEY, BTN_TOUCH);
	input_set_abs_params(priv->input, ABS_X, 0, CST353X_NATIVE_MAX_X - 1, 0, 0);
	input_set_abs_params(priv->input, ABS_Y, 0, CST353X_NATIVE_MAX_Y - 1, 0, 0);

	touchscreen_parse_properties(priv->input, false, &priv->prop);

	return input_register_device(priv->input);
}

static void cst353x_reset(struct cst353x_priv *priv)
{
	gpiod_set_value_cansleep(priv->reset, 1);
	msleep(50);
	gpiod_set_value_cansleep(priv->reset, 0);
	msleep(100);
}

static int cst353x_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct cst353x_priv *priv;
	int rc;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->client = client;

	priv->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(priv->reset))
		return dev_err_probe(dev, PTR_ERR(priv->reset),
				     "failed to request reset gpio\n");

	cst353x_reset(priv);

	rc = cst353x_register_input(priv);
	if (rc)
		return dev_err_probe(dev, rc, "failed to register input device\n");

	rc = devm_request_threaded_irq(dev, client->irq, NULL, cst353x_irq_cb,
				       IRQF_ONESHOT, dev->driver->name, priv);
	if (rc)
		return dev_err_probe(dev, rc, "failed to request irq\n");

	return 0;
}

static const struct i2c_device_id cst353x_id[] = {
	{ "cst3530", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cst353x_id);

static const struct of_device_id cst353x_of_match[] = {
	{ .compatible = "hynitron,cst3530", },
	{ }
};
MODULE_DEVICE_TABLE(of, cst353x_of_match);

static struct i2c_driver cst353x_driver = {
	.driver = {
		.name = "cst353x",
		.of_match_table = cst353x_of_match,
	},
	.id_table = cst353x_id,
	.probe = cst353x_probe,
};

module_i2c_driver(cst353x_driver);

MODULE_DESCRIPTION("Hynitron CST353X Touchscreen Driver");
MODULE_LICENSE("GPL");
