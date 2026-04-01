#include <asm-generic/errno-base.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/sched.h>

#define MYDEV_NAME "xxx_sample_chardev"

struct xxx_sample_chardev
{
    struct cdev *c_dev;
    struct class *mydev_class;
    struct device *mydev;
    //定义信号量
    struct semaphore sem;
    //定义互斥体
    struct mutex mtx;
};

struct xxx_sample_chardev my_chrdev;

ssize_t xxx_sample_chardev_read(struct file *file, char __user *userbuf, size_t size, loff_t *offset)
{
    int ret;

    //信号量写法
    // ret = down_trylock(&my_chrdev.sem);
    // if (ret) {
    //     printk("设备已锁，返回 EBUSY\n");
    //     return -EBUSY;
    // }
    //互斥量写法
    ret = mutex_trylock(&my_chrdev.mtx);
    if(!ret)
    {
        printk("设备已锁，返回 EBUSY\n");
        return -EBUSY;
    }
    printk("内核read执行，pid=%d\n", current->pid);

    return size;
}

ssize_t xxx_sample_chardev_write(struct file *file, const char __user *usrbuf, size_t size, loff_t *offset)
{
    printk("内核write执行\n");
    return size;
}

int xxx_sample_chardev_open(struct inode *inode, struct file *file)
{
    printk("内核open执行\n");
    return 0;
}

int xxx_sample_chardev_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&my_chrdev.mtx);
    // up(&my_chrdev.sem); // 关闭才解锁
    printk("内核close执行，已解锁\n");
    return 0;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = xxx_sample_chardev_open,
    .read = xxx_sample_chardev_read,
    .write = xxx_sample_chardev_write,
    .release = xxx_sample_chardev_release,
};

int __init my_test_module_init(void)
{
    int ret;
    dev_t devno;

    printk("驱动入口\n");

    my_chrdev.c_dev = cdev_alloc();
    if (!my_chrdev.c_dev) return -ENOMEM;

    cdev_init(my_chrdev.c_dev, &fops);
    ret = alloc_chrdev_region(&devno, 0, 1, MYDEV_NAME);
    if (ret) return ret;

    cdev_add(my_chrdev.c_dev, devno, 1);

    my_chrdev.mydev_class = class_create(THIS_MODULE, "MYLED");
    my_chrdev.mydev = device_create(my_chrdev.mydev_class, NULL, devno, NULL, MYDEV_NAME);

    //初始化信号量
    sema_init(&my_chrdev.sem, 1); 

    //初始化互斥量
    mutex_init(&my_chrdev.mtx);
    return 0;
}

void __exit my_test_module_exit(void)
{
    device_destroy(my_chrdev.mydev_class, my_chrdev.c_dev->dev);
    class_destroy(my_chrdev.mydev_class);
    cdev_del(my_chrdev.c_dev);
    unregister_chrdev_region(my_chrdev.c_dev->dev, 1);
    kfree(my_chrdev.c_dev);
    printk("驱动出口\n");
}

MODULE_LICENSE("GPL");
module_init(my_test_module_init);
module_exit(my_test_module_exit);
