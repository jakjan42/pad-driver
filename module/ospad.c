#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/i2c.h>

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Jakub Jankowski <jj.jankowski01@gmail.com>");
MODULE_DESCRIPTION("A driver for the pads for a homebrew console");


#define I2C_REGS_COUNT 6
#define I2C_REGS \
  X(REG_STATUS,    1) \
  X(REG_JOYS1XMSB, 2) \
  X(REG_JOYS1XLSB, 3) \
  X(REG_JOYS1YMSB, 4) \
  X(REG_JOYS1YLSB, 5) \
  X(REG_BUTTONS1, 10)


static const char *i2c_reg_names[] = {
  #define X(reg, num) #reg ,
    I2C_REGS
  #undef X
};

enum pad_regs {
  #define X(reg, num) reg = num ,
    I2C_REGS
  #undef X
  REG_MAX,
};

static struct i2c_client *ospad_client;

static int ospad_probe(struct i2c_client *client);
static void ospad_remove(struct i2c_client *client);

static struct of_device_id ospad_ids[] = {
	{ .compatible = "p560,ospad" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, ospad_ids);

static struct i2c_device_id ospad[] = {
	{"ospad_ids", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(i2c, ospad);


static struct i2c_driver ospad_driver = {
	.probe = ospad_probe,
	.remove = ospad_remove,
	.id_table = ospad,
	.driver = {
		.name = "ospad",
		.of_match_table = ospad_ids,
	},
};


static struct proc_dir_entry *proc_file;

static ssize_t ospad_write(struct file *File, const char *user_buffer,
                           size_t count, loff_t *offs)
{
	long val;
	if(kstrtol(user_buffer, 0, &val) == 0) 
		i2c_smbus_write_byte_data(ospad_client, REG_STATUS, (u8) val);
	return count;
}

static ssize_t ospad_read(struct file *File, char *user_buffer,
                          size_t count, loff_t *offs)
{
	s32 ret;
	u8 status = 0, buttons1 = 0;
	u16 joys1x = 0, joys1y = 0;

	ret = i2c_smbus_read_byte_data(ospad_client, REG_STATUS);
	if (ret >= 0)
		status = (u8)ret;
	ret = i2c_smbus_read_byte_data(ospad_client, REG_BUTTONS1);
	if (ret >= 0)
		buttons1 = (u8)ret;
	ret = i2c_smbus_read_byte_data(ospad_client, REG_JOYS1XLSB);
	if (ret >= 0)
		joys1x = (u16)ret;
	ret = i2c_smbus_read_byte_data(ospad_client, REG_JOYS1XMSB);
	if (ret >= 0)
		joys1x = ((u16)ret << 8) | joys1x;
	ret = i2c_smbus_read_byte_data(ospad_client, REG_JOYS1YLSB);
	if (ret >= 0)
		joys1y = (u16)ret;
	ret = i2c_smbus_read_byte_data(ospad_client, REG_JOYS1YMSB);
	if (ret >= 0)
		joys1y = ((u16)ret << 8) | joys1y;

	return snprintf(user_buffer, count,
	                "%s: %d\n%s: %d\n%s: %d\n%s: %d\n",
	                "status", status,
	                "joys1x", joys1x,
	                "joys1y", joys1y,
	                "buttons1", buttons1
	                );
}


static struct proc_ops fops = {
	.proc_write = ospad_write,
	.proc_read = ospad_read,
};

static int ospad_probe(struct i2c_client *client) {
	printk("Probing ospad\n");

	ospad_client = client;
		
	/* Creating procfs file */
	proc_file = proc_create("ospad", 0666, NULL, &fops);
	if(proc_file == NULL) {
		printk("ospad - Error creating /proc/ospad\n");
		return -ENOMEM;
	}

	return 0;
}

static void ospad_remove(struct i2c_client *client) {
	printk("Removing ospad\n");
	proc_remove(proc_file);
}

module_i2c_driver(ospad_driver);

