
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>

struct input_dev *test_dev; //定义一个输入设备test_dev

static irqreturn_t key1_irq_handler(int irq, void *dev_id)
{
    gpio_set_value(myled.led1_gpios, 0);
    printk("KEY1 中断：LED ON\n");
    return IRQ_HANDLED;
}

static irqreturn_t key2_irq_handler(int irq, void *dev_id)
{
    gpio_set_value(myled.led1_gpios, 1);
    printk("KEY2 中断：LED OFF\n");
    return IRQ_HANDLED;
}

int my_dev_driver_probe(struct platform_device *pdev)
{
    int ret;
    struct device_node *key_node, *child;


    key_node = of_find_node_by_path("/gpio_keys");
    if (!key_node) return -ENODEV;

    int index = 0;
    for_each_child_of_node(key_node, child) {
        if (index == 0)
            myled.key1_gpio = of_get_named_gpio(child, "gpios", 0);
        if (index == 1)
            myled.key2_gpio = of_get_named_gpio(child, "gpios", 0);
        index++;
    }


    myled.irq1 = gpio_to_irq(myled.key1_gpio);
    myled.irq2 = gpio_to_irq(myled.key2_gpio);

    ret = devm_request_irq(&pdev->dev, myled.irq1, key1_irq_handler,
                           IRQF_TRIGGER_FALLING | IRQF_SHARED,
                           "key1_irq", &myled);

    ret = devm_request_irq(&pdev->dev, myled.irq2, key2_irq_handler,
                           IRQF_TRIGGER_FALLING | IRQF_SHARED,
                           "key2_irq", &myled);

    printk("按键中断注册成功！KEY1_IRQ=%d, KEY2_IRQ=%d\n", myled.irq1, myled.irq2);

    test_dev = input_allocate_device();
    test_dev->name = "gpio_keys"

    set_bit(EV_KEY, test_dev->evbit);
    set_bit(KEY_1, test_dev->keybit);

    ret = input_register_device(test_dev);
    if(ret < 0)
    {
        printf("input_register_device is error \n");
    }
    return 0;
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    // devm_ 自动释放中断，无需手动 free_irq

    gpio_free(myled.led1_gpios);
    unregister_chrdev_region(myled.devid, 1);
    kfree(myled.c_dev);

    return 0;
}

// 匹配表不变
struct of_device_id of_node_match_table[] = {
    {.compatible = "imx6ull,led"},
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
