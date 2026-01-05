#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

enum i2c_regs {
  OSPAD_REG_STATUS   = 1,
  OSPAD_REG_REQ_DATA = 2
};

enum status {
  STATUS_OK,
  STATUS_RESERVED,
  STATUS_REQ_JOYS_COUNT,
  STATUS_REQ_BTN_COUNT,
  STATUS_REQ_JOYS_START,
  STATUS_REQ_BTN_START,
  STATUS_REQ_CALIBRATE,
  STATUS_CALIBRATING,
};

#define OSPAD_JOYS_REG_SIZE 2
#define OSPAD_JOYS_MAX_VAL  1023
#define OSPAD_JOYS_FUZZ     8
#define OSPAD_JOYS_FLAT     32
#define OSPAD_POLL_INTERVAL 16
#define OSPAD_POLL_MIN      8
#define OSPAD_POLL_MAX      32


struct padinfo {
	char phys[32];
	struct input_dev *dev;
	struct i2c_client *client;
	u8  joys_size;
	u8  joys_count;
	u8  joys_start;
	u16 btn_count;
	u8  btn_start;
};

static struct i2c_client *ospad_client;

static int ospad_probe(struct i2c_client *client);

static struct of_device_id ospad_ids[] = {
	{ .compatible = "p560,ospad" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ospad_ids);

static struct i2c_device_id ospad[] = { { KBUILD_MODNAME },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, ospad);


static struct i2c_driver ospad_driver = {
	.probe = ospad_probe,
	.id_table = ospad,
	.driver = {
		.name = "ospad",
		.of_match_table = of_match_ptr(ospad_ids),
	},
};


static inline s32 ospad_read_word(struct i2c_client *client, u8 reg)
{
	s32 ret = i2c_smbus_read_word_data(client, reg);
	if (ret < 0)
		return ret;
	return (s32)be16_to_cpu(ret);
}

static inline s32 ospad_read_byte(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline s32 ospad_read_req_data(struct i2c_client *client)
{
	return ospad_read_word(client, OSPAD_REG_REQ_DATA);
}

static inline void ospad_send_data_req(struct i2c_client *client, u8 cmd)
{
	i2c_smbus_write_byte_data(client, OSPAD_REG_STATUS, cmd);
}


static void ospad_poll(struct input_dev *input)
{
	u8 i, j;
	s32 ret;
	struct padinfo *info = input_get_drvdata(input);

	for (i = 0; i < info->joys_count; i++) {
		ret = ospad_read_word(info->client, info->joys_start +
			i * info->joys_size * 2);
		if (ret >= 0)
			input_report_abs(input, ABS_X, ret);

		ret = ospad_read_word(info->client, info->joys_start +
			i * info->joys_size * 2 + info->joys_size);
		if (ret >= 0)
			input_report_abs(input, ABS_Y, ret);
	}

	const u8 btn_regs_count = (info->btn_count + 7) / 8;
	for (i = 0; i < btn_regs_count ; i++) {
		ret = ospad_read_byte(info->client, info->btn_start + i);
		for (j = 0; j < 8; j++) {
			if (ret >= 0 && info->btn_count > (u16)i * 8 + j) {
				input_report_key(input, BTN_LEFT,
				                 !(ret & (1UL << j)));
			}
		}
	}

	input_sync(input);
}


static int ospad_probe(struct i2c_client *client) {
	pr_info("Probing ospad\n");

	s32 ret;
	ospad_client = client;

	struct padinfo *info = devm_kzalloc(&client->dev,
	                                   sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;


	info->joys_size = 2;
	ospad_send_data_req(client, STATUS_REQ_JOYS_COUNT);
	ret = ospad_read_req_data(client);
	if (ret < 0)
		return ret;
	info->joys_count = ret;
		
	ospad_send_data_req(client, STATUS_REQ_BTN_COUNT);
	ret = ospad_read_req_data(client);
	if (ret < 0)
		return ret;
	info->btn_count = ret;

	ospad_send_data_req(client, STATUS_REQ_JOYS_START);
	ret = ospad_read_req_data(client);
	if (ret < 0)
		return ret;
	info->joys_start = ret;

	ospad_send_data_req(client, STATUS_REQ_BTN_START);
	ret = ospad_read_req_data(client);
	if (ret < 0)
		return ret;
	info->btn_start = ret;


	info->client = client;
	snprintf(info->phys, sizeof(info->phys),
	         "i2c/%s", dev_name(&client->dev));
	i2c_set_clientdata(client, info);

	info->dev = devm_input_allocate_device(&client->dev);
	if (!info->dev)
		return -ENOMEM;

	info->dev->id.bustype = BUS_I2C;
	info->dev->name = "ospad";
	info->dev->phys = info->phys;
	input_set_drvdata(info->dev, info);

	input_set_abs_params(info->dev, ABS_X, 0, OSPAD_JOYS_MAX_VAL,
	                     OSPAD_JOYS_FUZZ, OSPAD_JOYS_FLAT);
	input_set_abs_params(info->dev, ABS_Y, 0, OSPAD_JOYS_MAX_VAL,
	                     OSPAD_JOYS_FUZZ, OSPAD_JOYS_FLAT);
	input_set_capability(info->dev, EV_KEY, BTN_LEFT);

	ret = input_setup_polling(info->dev, ospad_poll);
	if (ret) {
		dev_err(&client->dev, "Failed to setup polling: %d\n", ret);
		return ret;
	}

	input_set_poll_interval(info->dev, OSPAD_POLL_INTERVAL);
	input_set_min_poll_interval(info->dev, OSPAD_POLL_MIN);
	input_set_max_poll_interval(info->dev, OSPAD_POLL_MAX);

	ret = input_register_device(info->dev);
	if (ret)
		dev_err(&client->dev, "Failed to register device: %d\n", ret);

	return ret;
}


module_i2c_driver(ospad_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jakub Jankowski <jj.jankowski01@gmail.com>");
MODULE_DESCRIPTION("A driver for the pads for a homebrew console");
