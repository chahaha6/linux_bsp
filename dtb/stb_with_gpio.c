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
#include <linux/of_platform.h>
#include <linux/uaccess.h>

struct MyLED{
    struct cdev* c_dev;
    struct class* class;
    struct device* dev;
    dev_t devid;        // 修复：独立设备号
    uint32_t led1_gpios;
};

struct MyLED myled = {0};

int myled_open(struct inode *inode, struct file *file)
{
    printk("myled_open 执行了\n");
    return 0;
}

ssize_t myled_write(struct file *file, const char __user *usrbuf, size_t size, loff_t *offset)
{
    char k_buf[32] = {0};
    printk("myled_write 执行了\n");

    copy_from_user(k_buf, usrbuf, size > 31 ? 31 : size);

    if(k_buf[0] == '1')
    {
        gpio_set_value(myled.led1_gpios, 1);
    }
    else
    {
        gpio_set_value(myled.led1_gpios, 0);
    }

    return size;
}

ssize_t myled_read(struct file *file, char __user *usrbuf, size_t size, loff_t *offset)
{
    printk("myled_read 执行了\n");
    return size;
}

int myled_close(struct inode *inode, struct file *file)
{
    printk("myled_close 执行了\n");
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = myled_open,
    .read = myled_read,
    .write = myled_write,
    .release = myled_close,
};

int my_dev_driver_probe(struct platform_device *pdev)
{
    int ret;

    printk("my_dev_driver_probe 执行\n");

    // 1. cdev 初始化
    myled.c_dev = cdev_alloc();
    cdev_init(myled.c_dev, &fops);

    // 2. 申请设备号
    ret = alloc_chrdev_region(&myled.devid, 0, 1, "myled");
    if(ret < 0){
        printk("设备号申请失败\n");
        return ret;
    }

    cdev_add(myled.c_dev, myled.devid, 1);

    // 3. 创建设备节点
    myled.class = class_create(THIS_MODULE, "MYLED");
    myled.dev = device_create(myled.class, NULL, myled.devid, NULL, pdev->name);
    printk("设备节点名：%s\n",pdev->name);

    myled.led1_gpios = of_get_named_gpio(pdev->dev.of_node, "gpio", 0);
    printk("获取到的GPIO号 = %d\n", myled.led1_gpios); // 加日志

    // 申请GPIO
    gpio_request(myled.led1_gpios, "led1_gpios");
    gpio_direction_output(myled.led1_gpios, 0);
    printk("LED 驱动 probe 成功！\n");
    return 0;
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    printk("my_dev_driver_remove 执行\n");

    gpio_free(myled.led1_gpios);
    device_destroy(myled.class, myled.devid);
    class_destroy(myled.class);
    cdev_del(myled.c_dev);
    unregister_chrdev_region(myled.devid, 1);
    kfree(myled.c_dev);

    return 0;
}

// 匹配表保持你原来的不变
struct platform_device_id id_table_match[] = {
    [0] = {"imx6ull,led01", 1},
    [1] = {"imx6ull,led02", 2},
    [2] = {}
};

struct of_device_id of_node_match_table[] = {
    [0] = {.compatible = "imx6ull,led"},
    [1] = {.compatible = "imx6ull,led1"},
    [2] = {}
};

struct platform_driver my_platform_driver = {
    .probe = my_dev_driver_probe,
    .remove = my_dev_driver_remove,
    .driver = {
        .name = "imx6ull,led00",
        .of_match_table = of_node_match_table,
    },
    .id_table = id_table_match,
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

MODULE_LICENSE("GPL");
module_init(my_test_module_init);
module_exit(my_test_module_exit);