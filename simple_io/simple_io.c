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
    dev_t dev_num;
    struct cdev ds18b20_dev;
    struct class *ds18b20_class;
    struct device *ds18b20_device;
}
struct s_ds18b20 *ds18b20;

struct file_operations ds18b20_fops
{
    .OWNMER = THIS_MODULE;
    .open=ds18b20_open,
    .read=ds18b20_read,
    .release=ds18b20_release,
};

intds18b20_open(struct inode*inode, struct file*file)
{
    return 0;
}

ssize_tds18b20_read(struct file *file, char__user*buf, size_t size, loff_t*offs)
{
    return 0;
}

intds18b20_release(struct inode*inode, struct file*file)
{
    return 0;
}
int my_dev_driver_probe(struct platform_device *pdev)
{
    int ret;
    printf("this is probe\n");
    
    //分配设备空间
    ds18b20 = kzalloc(sizeof(*ds18b20), GFP_KERNEL);
    if (NULL == ds18b20)
    {
        printf("kzalloc is error\n");
    }
    //注册设备号
    ret = alloc_chrdev_region(&ds18b20->dev_num, 0, 1, "myds18b20");
    if(0 == ret)
    {
        printf("alloc_chrdev_region is error\n");
    }
    
    //初始化设备
    cdev_init(&ds18b20->ds18b20_cdev, &ds18b20_fops);
    //加入链表
    cdev_add(&ds18b20->ds18b20_cdev,ds18b20->dev_num,1);
    //class类
    ds18b20->ds18b20_class = class_create(THIS_MODULE, "sensors");
    if(IS_ERR(ds18b20->ds18b20_class))
    {
        printk("class_create is err\n");
    }
    //device设备
    device_create(ds18b20->ds18b20_class,NULL,ds18b20->dev_num,NULL,"my_ds18b20");
    if (IS_ERR(ds18b20->ds18b20_device))
    {
        printk("device_create is error\n");
    }


}

int my_dev_driver_remove(struct platform_device *pdev)
{

    printk("gooodbye! \n");
    return 0;
}

// 匹配表不变
struct of_device_id of_node_match_table[] = {
    {.compatible = "my_ds18b20"},
    {}
};

struct platform_driver my_platform_driver = {
    .probe = my_dev_driver_probe,
    .remove = my_dev_driver_remove,
    .driver = {
        .name = "my_ds18b20",
        .of_match_table = of_node_match_table,
    },
};

int __init my_test_module_init(void)
{
    int ret = platform_driver_register(&ds18b20_driver);
    if (ret < 0){
        printk("驱动注册失败\n");
        return ret;
    }
    printk("LED 平台驱动注册成功\n");
    return 0;
}

void __exit my_test_module_exit(void)
{
    //销毁设备
    device_destroy(ds18b20->ds18b20_class,ds18b20->dev_num);
    //销毁设备类
    class_destroy(ds18b20->ds18b20_class);
    //删除字符设备
    cdev_del(&ds18b20->ds18b20_cdev);
    //注销字符设备号
    unregister_chrdev_region(ds18b20->dev_num,1);
    //释放设备数据结构内存
    kfree(ds18b20);
    //注销平台驱动
    platform_driver_unregister(&ds18b20_driver);
    printk("驱动已卸载\n");
}

module_init(my_test_module_init);
module_exit(my_test_module_exit);
MODULE_LICENSE("GPL");