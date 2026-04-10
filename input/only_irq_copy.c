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
#include <linux/input.h>

struct my_key
{
    int key1_gpio;
    int key2_gpio;
    int irq1;
    int irq2;
    struct input_dev *test_dev; 
};

struct my_key my_keys;

// 修改点1：添加 Input 上报逻辑
static irqreturn_t key1_irq_handler(int irq, void *dev_id)
{
    // 上报按键事件
    input_report_key(my_keys.test_dev, KEY_1, 0);
    input_sync(my_keys.test_dev); // 

    
    gpio_set_value(my_keys.key1_gpio, 0); 
    printk("KEY1 中断上报: %d\n", 0);
    
    return IRQ_HANDLED;
}

// 修改点2：添加 Input 上报逻辑
static irqreturn_t key2_irq_handler(int irq, void *dev_id)
{
    input_report_key(my_keys.test_dev, KEY_2, 1);
    input_sync(my_keys.test_dev);
    printk("KEY2 中断上报: %d\n", 1);

    return IRQ_HANDLED;
}

int my_dev_driver_probe(struct platform_device *pdev)
{
    int ret;
    struct device_node *key_node, *child;

    key_node = pdev->dev.of_node; 
    if (!key_node) return -ENODEV;

    int index = 0;
    for_each_child_of_node(key_node, child) {
        if (index == 0)
            my_keys.key1_gpio = of_get_named_gpio(child, "gpios", 0);
        if (index == 1)
            my_keys.key2_gpio = of_get_named_gpio(child, "gpios", 0);
        index++;
    }

    my_keys.irq1 = gpio_to_irq(my_keys.key1_gpio);
    my_keys.irq2 = gpio_to_irq(my_keys.key2_gpio);

    ret = devm_request_irq(&pdev->dev, my_keys.irq1, key1_irq_handler,
                           IRQF_TRIGGER_FALLING | IRQF_SHARED,
                           "key1_irq", &my_keys);

    ret = devm_request_irq(&pdev->dev, my_keys.irq2, key2_irq_handler,
                           IRQF_TRIGGER_FALLING | IRQF_SHARED,
                           "key2_irq", &my_keys);

    printk("按键中断注册成功！KEY1_IRQ=%d, KEY2_IRQ=%d\n", my_keys.irq1, my_keys.irq2);

    my_keys.test_dev = input_allocate_device();
    if (!my_keys.test_dev) return -ENOMEM;  

    my_keys.test_dev->name = "gpio_keys";

    set_bit(EV_KEY, my_keys.test_dev->evbit);
    set_bit(EV_SYN, my_keys.test_dev->evbit); 
    set_bit(KEY_1, my_keys.test_dev->keybit);
    set_bit(KEY_2, my_keys.test_dev->keybit);

    ret = input_register_device(my_keys.test_dev);
    if(ret < 0)
    {
        printk("input_register_device is error \n");
        input_free_device(my_keys.test_dev); 
        return ret;
    }
    return 0;
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