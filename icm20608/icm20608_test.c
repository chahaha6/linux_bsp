#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>

/* ==================== 修复 1：必须 static ==================== */
static const struct of_device_id icm20608_id[] = {
	{ .compatible = "my_icm20608" },
	{ }
};

static const struct spi_device_id icm20608_ids[] = {
	{ "my_icm20608" },
	{ }
};

static int icm20608_probe(struct spi_device *spi)
{
	printk(KERN_INFO "icm20608: PROBE 成功！！！\n");
	return 0;
}

static int icm20608_remove(struct spi_device *spi)
{
	printk(KERN_INFO "icm20608: remove\n");
	return 0;
}

struct spi_driver icm20608_driver = {
	.driver = {
		.name = "my_icm20608",
		.owner = THIS_MODULE,
		.of_match_table = icm20608_id,  
	},
	.probe = icm20608_probe,
	.remove = icm20608_remove,
	.id_table = icm20608_ids,
};

static int __init my_init(void)
{
	printk("icm20608: 驱动加载\n");
	return spi_register_driver(&icm20608_driver);
}

static void __exit my_exit(void)
{
	spi_unregister_driver(&icm20608_driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
