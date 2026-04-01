#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#define MYDEV_NAME "xxx_sample_chardev"

// 描述一个字符设备：应该有哪些属性：
// 1.设备名称，2.设备号：主设备好，次设备号，3.对设备进行操作的结构体struct file_operations
struct xxx_sample_chardev
{
    struct cdev *c_dev;
    // 添加设备类，设备属性：
    struct class *mydev_class;
    struct device *mydev;
    spinlock_t my_spin_lock;
};
// 1.在全局中定义的 xxx_sample_charde的对象
struct xxx_sample_chardev my_chrdev;

// 在内核定义对应的函数接口：
// 与文件read对应的函数指针
ssize_t xxx_sample_chardev_read(struct file *file, char __user *userbuf, size_t size, loff_t *offset)
{
    printk("内核中的xxx_sample_chardev_read执行了\n");
    return size;
}
// 与write对应的函数指针
ssize_t xxx_sample_chardev_write(struct file *file, const char __user *usrbuf, size_t size, loff_t *offset)
{
    printk("内核中的xxx_sample_chardev_write执行了\n");
    // 唤醒等待队列中的进程：
    return size;
}

// 与open对应的函数指针
int xxx_sample_chardev_open(struct inode *inode, struct file *file)
{
    printk("内核中的xxx_sample_chardev_open执行了\n");
    // 初始化设备的等待队列：
    return 0;
}
// 与close对应函数指针：
int xxx_sample_chardev_release(struct inode *inode, struct file *file)
{
    printk("内核中的xxx_sample_chardev_release执行了\n");
    return 0;
}

struct file_operations fops = {
    .open = xxx_sample_chardev_open,
    .read = xxx_sample_chardev_read,
    .write = xxx_sample_chardev_write,
    .release = xxx_sample_chardev_release,
};

// 入口函数：
int __init my_test_module_init(void)
{
    int ret;
    printk("A模块的入口函数执行了");
    // 2.3单独申请设备号的方式：
    my_chrdev.c_dev = cdev_alloc();
    if (my_chrdev.c_dev == NULL)
    {
        printk("cdev_alloc err:\n");
        return -ENOMEM;
    }
    // cdev初始化：
    cdev_init(my_chrdev.c_dev, &fops);
    // 申请设备号
    ret = alloc_chrdev_region(&my_chrdev.c_dev->dev, 0, 1, MYDEV_NAME);
    if (ret)
    {
        printk("alloc_chrdev_region err\n");
        return ret;
    }
    printk("申请到的主设备号 = %d\n", MAJOR(my_chrdev.c_dev->dev));

    // 把cde对象添加到内核设备链表中：
    ret = cdev_add(my_chrdev.c_dev, my_chrdev.c_dev->dev, 1);
    if (ret)
    {
        printk("cdev_add err:");
        return ret;
    }
    // 申请设备类：
    my_chrdev.mydev_class = class_create(THIS_MODULE, "MYLED");
    if (IS_ERR(my_chrdev.mydev_class))
    {
        printk("class_create失败\n");
        return PTR_ERR(my_chrdev.mydev_class);
    }
    // 申请设备对象：向上提交uevent事件：建立了设备节点与设备号之间的关系。
    my_chrdev.mydev = device_create(my_chrdev.mydev_class, NULL, my_chrdev.c_dev->dev, NULL, MYDEV_NAME);
    if (IS_ERR(my_chrdev.mydev))
    {
        printk("device_create失败\n");
        return PTR_ERR(my_chrdev.mydev);
    }
    // 初始化等待队列头：
    spin_lock_init(&my_chrdev.my_spin_lock);
    return 0;
}

// 出口函数：
void __exit my_test_module_exit(void)
{
    printk("出口函数执行了\n"); // 把调试信息放在了系统日志缓冲区，使用dmesg来显示。
    // 清理资源。
    // 先销毁设备，再销毁设备类：
    device_destroy(my_chrdev.mydev_class, my_chrdev.c_dev->dev);
    class_destroy(my_chrdev.mydev_class);
    cdev_del(my_chrdev.c_dev);
    unregister_chrdev_region(my_chrdev.c_dev->dev, 1);
    kfree(my_chrdev.c_dev);
}

// 指定许可：
MODULE_LICENSE("GPL");
// 指定入口及出口函数：
module_init(my_test_module_init);
module_exit(my_test_module_exit);