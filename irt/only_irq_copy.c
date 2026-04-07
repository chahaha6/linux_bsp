/**
 * 只使用中断子系统需要在设备树中，把gpio-keys的status状态设置为“disable”。
*/
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

struct MyLED {
    struct cdev *c_dev;
    struct class *class;
    struct device *dev;
    dev_t devid;
    uint32_t led1_gpios;

    uint32_t key1_gpio;
    uint32_t key2_gpio;
    int irq1;        // key1 中断号
    int irq2;        // key2 中断号
};

struct MyLED myled = {0};

// ================ 中断服务函数 ISR =================
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

// ================ 字符设备接口 =================
int myled_open(struct inode *inode, struct file *file)
{
    return 0;
}

ssize_t myled_write(struct file *file, const char __user *usrbuf, size_t size, loff_t *offset)
{
    char k_buf[32] = {0};
    copy_from_user(k_buf, usrbuf, size > 31 ? 31 : size);

    if(k_buf[0] == '1')
        gpio_set_value(myled.led1_gpios, 1);
    else
        gpio_set_value(myled.led1_gpios, 0);

    return size;
}

int myled_close(struct inode *inode, struct file *file)
{
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = myled_open,
    .write = myled_write,
    .release = myled_close,
};

// ================ platform probe =================
int my_dev_driver_probe(struct platform_device *pdev)
{
    int ret;
    struct device_node *key_node, *child;

    // 1. 字符设备初始化
    myled.c_dev = cdev_alloc();
    cdev_init(myled.c_dev, &fops);
    ret = alloc_chrdev_region(&myled.devid, 0, 1, "myled");
    cdev_add(myled.c_dev, myled.devid, 1);

    myled.class = class_create(THIS_MODULE, "MYLED");
    myled.dev = device_create(myled.class, NULL, myled.devid, NULL, pdev->name);

    // 2. 初始化 LED GPIO
    myled.led1_gpios = of_get_named_gpio(pdev->dev.of_node, "gpio", 0);
    gpio_request(myled.led1_gpios, "led1");
    gpio_direction_output(myled.led1_gpios, 0);

    // 3. 读取按键 gpio-keys 节点
    key_node = of_find_node_by_path("/gpio-keys");
    if (!key_node) return -ENODEV;

    int index = 0;
    for_each_child_of_node(key_node, child) {
        if (index == 0)
            myled.key1_gpio = of_get_named_gpio(child, "gpios", 0);
        if (index == 1)
            myled.key2_gpio = of_get_named_gpio(child, "gpios", 0);
        index++;
    }

    // 4. 申请 GPIO 为输入
    gpio_direction_input(myled.key1_gpio);
    gpio_direction_input(myled.key2_gpio);

    // 5. 获取中断号
    myled.irq1 = gpio_to_irq(myled.key1_gpio);
    myled.irq2 = gpio_to_irq(myled.key2_gpio);

    // 6. 注册中断（下降沿触发，按键按下低电平）
    ret = devm_request_irq(&pdev->dev, myled.irq1, key1_irq_handler,
                           IRQF_TRIGGER_FALLING | IRQF_SHARED,
                           "key1_irq", &myled);

    ret = devm_request_irq(&pdev->dev, myled.irq2, key2_irq_handler,
                           IRQF_TRIGGER_FALLING | IRQF_SHARED,
                           "key2_irq", &myled);

    printk("按键中断注册成功！KEY1_IRQ=%d, KEY2_IRQ=%d\n", myled.irq1, myled.irq2);
    return 0;
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    // devm_ 自动释放中断，无需手动 free_irq

    gpio_free(myled.led1_gpios);
    device_destroy(myled.class, myled.devid);
    class_destroy(myled.class);
    cdev_del(myled.c_dev);
    unregister_chrdev_region(myled.devid, 1);
    kfree(myled.c_dev);

    return 0;
}

// 匹配表不变
struct of_device_id of_node_match_table[] = {
    {.compatible = "imx6ull,led"},
    {.compatible = "imx6ull,led1"},
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
