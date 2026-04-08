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

#include <linux/workqueue.h>

struct MyLED{
    struct cdev* c_dev;
    struct class* class;
    struct device* dev;
    dev_t devid;
    uint32_t led1_gpios;

    // 只新增这部分
    uint32_t key1_gpio;
    uint32_t key2_gpio;
    struct delayed_work key_work;
};

struct MyLED myled = {0};

// 按键轮询（消抖）
static void key_work_func(struct work_struct *work)
{
    int val1 = gpio_get_value_cansleep(myled.key1_gpio);
    int val2 = gpio_get_value_cansleep(myled.key2_gpio);

    if(val1 == 0){
        gpio_set_value(myled.led1_gpios, 0);
        printk("KEY1: LED ON\n");
    }

    if(val2 == 0){
        gpio_set_value(myled.led1_gpios, 1);
        printk("KEY2: LED OFF\n");
    }

    schedule_delayed_work(&myled.key_work, msecs_to_jiffies(30));
}


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
    struct device_node *key_node, *child;

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
    printk("获取到的GPIO号 = %d\n", myled.led1_gpios);

    gpio_request(myled.led1_gpios, "led1_gpios");
    gpio_direction_output(myled.led1_gpios, 0);

    // ===================== 【修复】正确读取 gpio-keys 子节点 =====================
    key_node = of_find_node_by_path("/gpio-keys");
    if (!key_node) {
        printk("找不到 gpio-keys 节点\n");
        return -ENODEV;
    }

    // 遍历子节点 user1 / user2
    child = NULL;
    int index = 0;
    for_each_child_of_node(key_node, child) {
        if (index == 0)
            myled.key1_gpio = of_get_named_gpio(child, "gpios", 0);
        if (index == 1)
            myled.key2_gpio = of_get_named_gpio(child, "gpios", 0);
        index++;
    }

    printk("KEY1 GPIO: %d, KEY2 GPIO: %d\n", myled.key1_gpio, myled.key2_gpio);

    // 校验 GPIO 是否有效
    if (!gpio_is_valid(myled.key1_gpio) || !gpio_is_valid(myled.key2_gpio)) {
        printk("按键 GPIO 无效\n");
        return -EINVAL;
    }

    // 申请按键
    ret = devm_gpio_request(&pdev->dev, myled.key1_gpio, "key1");
    ret = devm_gpio_request(&pdev->dev, myled.key2_gpio, "key2");

    gpio_direction_input(myled.key1_gpio);
    gpio_direction_input(myled.key2_gpio);

    // 启动轮询
    INIT_DELAYED_WORK(&myled.key_work, key_work_func);
    schedule_delayed_work(&myled.key_work, msecs_to_jiffies(30));
    // ============================================================================

    printk("LED 驱动 probe 成功！\n");
    return 0;
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    printk("my_dev_driver_remove 执行\n");

    // 安全销毁工作队列
    cancel_delayed_work_sync(&myled.key_work);

    gpio_free(myled.led1_gpios);
    device_destroy(myled.class, myled.devid);
    class_destroy(myled.class);
    cdev_del(myled.c_dev);
    unregister_chrdev_region(myled.devid, 1);
    kfree(myled.c_dev);

    return 0;
}

// 你的匹配表 一字不动
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
