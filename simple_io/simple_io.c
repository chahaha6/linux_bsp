#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>


struct s_ds18b20
{
    dev_t dev_hao;
}
int my_dev_driver_probe(struct platform_device *pdev)
{
   kzalloc()
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    free_irq(my_keys.irq1, NULL);
    free_irq(my_keys.irq2, NULL);
    input_unregister_device(my_keys.test_dev);
    printk("gooodbye! \n");
    return 0;
}

// 匹配表不变
struct of_device_id of_node_match_table[] = {
    {.compatible = "gpio_keys"},
    {}
};

struct platform_driver my_platform_driver = {
    .probe = my_dev_driver_probe,
    .remove = my_dev_driver_remove,
    .driver = {
        .name = "imx6ull_led",
        .of_match_table = of_node_match_table,
    },
};

int __init my_test_module_init(void)
{
    int ret = platform_driver_register(&my_platform_driver);
    if (ret < 0){
        printk("驱动注册失败\n");
        return ret;
    }
    printk("LED 平台驱动注册成功\n");
    return 0;
}

void __exit my_test_module_exit(void)
{
    platform_driver_unregister(&my_platform_driver);
    printk("驱动已卸载\n");
}

module_init(my_test_module_init);
module_exit(my_test_module_exit);
MODULE_LICENSE("GPL");