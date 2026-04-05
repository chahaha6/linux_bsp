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

int myled_open(struct inode *inode, struct file *file)
{
    printk("myled_open 执行了\n");
    return 0;
}
ssize_t myled_write(struct file *file, const char *usrbuf, size_t size, loff_t *offset)
{
    printk("myled_write 执行了\n");

    return size;
}
ssize_t myled_read(struct file *file, char *usrbuf, size_t size, loff_t *offset)
{
    printk("myled_read 执行了\n");
    return size;
}

int myled_close(struct inode *inode, struct file *file)
{
    printk("myled_close 执行了\n");
    return 0;
}

int my_dev_driver_probe(struct platform_device *pdev)
{

    printk("my_dev_driver_probe probe函数执行\n");
    printk("设备节点的名称 = %s\n",pdev->name);
    printk("资源1 = %#x \n",pdev->resource[0].start);
    printk("资源2 = %#x \n",pdev->resource[1].start);
    printk("资源3 = %#x \n",pdev->resource[2].start);
    return 0;
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    printk("my_dev_driver_remover函数执行了\n");
    return 0;
}

struct platform_device_id id_table_match[] = {
    [0] = {"WX,my_device_01", 1},
    [1] = {"WX,my_device_02", 2},
    [2] = {/*最后一个一定要给一个空元素，代表结束*/}
};

//设备树匹配方式：
struct of_device_id of_node_match_table[] = {
    [0] = {.compatible = "imx6ull,led"},
    [1] = {.compatible = "imx6ull,led2"},
    [2] = {/*最后一个一定要给一个空元素，代表结束*/}
};

// 1.定义一个平台驱动对象：
struct platform_driver my_platform_driver = {
    .probe = my_dev_driver_probe,
    .remove = my_dev_driver_remove,
    .driver = {
        .name = "WX,my_device_driver",
        //设备树的匹配方式：
        .of_match_table = of_node_match_table,
    },
    .id_table = id_table_match,
};

// 入口函数：
int __init my_test_module_init(void)
{
    int ret = 0;
    ret = platform_driver_register(&my_platform_driver);
    if (ret < 0)
    {
        return -1;
    }
    return 0;
}

// 出口函数：
void __exit my_test_module_exit(void)
{
    platform_driver_unregister(&my_platform_driver);
}

// 指定许可：
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gaowanxi, email:gaonetcom@163.com");
// 指定入口及出口函数：
module_init(my_test_module_init);
module_exit(my_test_module_exit);