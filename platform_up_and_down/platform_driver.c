#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
// 封装自定义的设备结构体：
struct myled_device
{
    // 提供设备号与操作方法的：
    struct cdev* c_dev;
    // 创建设备节点的
    struct class* dev_class;
    struct device* dev;
    // 添加映射属性：
    int* ccgr;
    int* tamper;
    int* gpio_dr;
    int* gpio_gdir;
};

struct myled_device myled = {0};
int myled_open(struct inode *inode, struct file *file)
{
    printk("myled_open 执行了\n");
    return 0;
}
ssize_t myled_write(struct file *file, const char *usrbuf, size_t size, loff_t *offset)
{
    char k_buf[128] = {0};
    printk("myled_write 执行了\n");
    copy_from_user(k_buf + *offset,usrbuf,size);
    /**
     *    if(flag)
    {
        *GPIO5_DR &= ~(1<<3);
    }
    else
    {
        *GPIO5_DR |= (1<<3);
    }
    */
    if(k_buf[0] == '1')
    {
        *myled.gpio_dr &= ~(1<<3);
    }
    else{
        *myled.gpio_dr |= (1<<3);
    }
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
struct file_operations fops = {
    .open = myled_open,
    .read = myled_read,
    .write = myled_write,
    .release = myled_close,
};

int my_dev_driver_probe(struct platform_device *pdev)
{
    // 1.在驱动中实现上通：建立向上提供的接口（操作方法fops）,设备号，设备节点
    // 提供操作方法fops:
    myled.c_dev = cdev_alloc();
    cdev_init(myled.c_dev,&fops);
    //申请设备号：
    alloc_chrdev_region(&myled.c_dev->dev,0, 1, pdev->name);
    //把cdev注册到内核设备管理链表中：
    cdev_add(myled.c_dev,myled.c_dev->dev,1);

    myled.dev_class = class_create(THIS_MODULE, pdev->name);

    myled.dev = device_create(myled.dev_class,NULL,myled.c_dev->dev,NULL, pdev->name);

    // 2.下达：获取硬件资源信息，并建立映射：
    /**
     * 
    *CCM_CCGR1 |= (3 << 26);
    *IOMUXC_SNVS_SW_MUX_CTL_PAD_SNVS_TAMPER3 = 0x105;
    *GPIO5_GDIR |= (1 << 3);
    *GPIO5_DR |= (1 << 3);
    */
    myled.ccgr = ioremap(pdev->resource[0].start, 4);
    printk("rcc = %#x\n",pdev->resource[0].start);
    *myled.ccgr |= (3 << 26);

    myled.tamper = ioremap(pdev->resource[1].start, 4);
    printk("modr = %#x\n",pdev->resource[1].start);
    *myled.tamper  = 0x105;

    myled.gpio_dr = ioremap(pdev->resource[2].start, 4);
    *myled.gpio_dr |= (1 << 3);
    printk("odr = %#x\n",pdev->resource[2].start);

    myled.gpio_gdir = ioremap(pdev->resource[3].start, 4);
    *myled.gpio_gdir |= (1 << 3);
    printk("odr = %#x\n",pdev->resource[3].start);
    
    return 0;
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    printk("my_dev_driver_remover函数执行了\n");
    device_destroy(myled.dev_class, myled.c_dev->dev);
    class_destroy(myled.dev_class);
    kfree(myled.c_dev);
    return 0;
}
//匹配列表：
struct platform_device_id id_table_match[] = {
    [0] = {"my_device01", 1},
    [1] = {"my_device02", 2},
    [2] = {/*最后一个一定要给一个空元素，代表结束*/}};

// 1.定义一个平台驱动对象：
struct platform_driver my_platform_driver = {
    .probe = my_dev_driver_probe,
    .remove = my_dev_driver_remove,
    .driver = {
        .name = "my_device_driver",
    },
    .id_table = id_table_match,
};

// 入口函数：
int __init my_test_module_init(void)
{
    printk("init is ok!");
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
// 指定入口及出口函数：
module_init(my_test_module_init);
module_exit(my_test_module_exit);