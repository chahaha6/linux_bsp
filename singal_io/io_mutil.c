#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fcntl.h> // 需要这个头文件来使用 O_ASYNC 等标志

#define MYDEV_NAME "xxx_sample_chardev"

// 1. 设备结构体：只保留 cdev, class, device 和 fasync_struct
struct xxx_sample_chardev
{
    struct cdev *c_dev;
    struct class *mydev_class;
    struct device *mydev;
    
    // 异步通知队列
    struct fasync_struct *fap;
};

char kernel_buf[128] = {0};
int data_ready = 0; // 简单的标志位，不需要配合等待队列
struct xxx_sample_chardev my_chrdev;

// 2. read 函数：只处理非阻塞/异步逻辑
ssize_t xxx_sample_chardev_read(struct file *file, char __user *userbuf, size_t size, loff_t *offset)
{
    // 信号驱动 I/O 通常配合非阻塞模式使用
    // 如果数据没准备好，直接返回 -EAGAIN，不要阻塞
    
    if (data_ready == 0)
    {
        return -EAGAIN; // 告诉应用层：现在没数据，稍后再试（或者等信号）
    }

    // 有数据，进行拷贝
    if (size >= sizeof(kernel_buf))
    {
        size = sizeof(kernel_buf);
    }
    
    if (copy_to_user(userbuf, kernel_buf, size))
    {
        return -EFAULT;
    }

    // 读取完成后，重置标志位
    data_ready = 0;
    
    printk("内核：数据已读取，标志位清零\n");
    return size;
}

// 3. write 函数：写入数据，发送信号
ssize_t xxx_sample_chardev_write(struct file *file, const char __user *usrbuf, size_t size, loff_t *offset)
{
    int ret = 0;
    
    if (size >= sizeof(kernel_buf))
    {
        size = sizeof(kernel_buf);
    }
    
    memset(kernel_buf, 0, sizeof(kernel_buf));

    ret = copy_from_user(kernel_buf, usrbuf, size);
    if (ret)
    {
        printk("copy_from_user failed");
        return -EIO;
    }
    
    printk("内核：收到数据：%s\n", kernel_buf);
    
    // 1. 设置数据就绪标志
    data_ready = 1;
    
    // 2. 关键：发送 SIGIO 信号给应用层
    // POLL_IN 表示有数据可读
    kill_fasync(&my_chrdev.fap, SIGIO, POLL_IN);
    
    return size;
}

// 4. open 函数
int xxx_sample_chardev_open(struct inode *inode, struct file *file)
{
    printk("内核：设备打开\n");
    return 0;
}

// 5. release 函数
int xxx_sample_chardev_release(struct inode *inode, struct file *file)
{
    printk("内核：设备关闭\n");
    return 0;
}

// 6. fasync 函数：这是信号驱动的核心入口
// 当应用层调用 fcntl(fd, F_SETFL, O_ASYNC) 时，内核会调用此函数
int xxx_sample_chardev_fasync(int fd, struct file *file, int on)
{
    // fasync_helper 会自动管理异步队列
    return fasync_helper(fd, file, on, &my_chrdev.fap);
}

// 7. 定义操作结构体
// 注意：去掉了 .poll
struct file_operations fops = {
    .open = xxx_sample_chardev_open,
    .read = xxx_sample_chardev_read,
    .write = xxx_sample_chardev_write,
    .release = xxx_sample_chardev_release,
    .fasync = xxx_sample_chardev_fasync, // 必须注册这个
};

// 8. 入口函数
int __init my_test_module_init(void)
{
    int ret;
    printk("模块初始化\n");

    my_chrdev.c_dev = cdev_alloc();
    if (my_chrdev.c_dev == NULL)
    {
        return -ENOMEM;
    }
    
    cdev_init(my_chrdev.c_dev, &fops);
    
    ret = alloc_chrdev_region(&my_chrdev.c_dev->dev, 0, 1, MYDEV_NAME);
    if (ret)
    {
        cdev_del(my_chrdev.c_dev);
        return ret;
    }
    
    ret = cdev_add(my_chrdev.c_dev, my_chrdev.c_dev->dev, 1);
    if (ret)
    {
        unregister_chrdev_region(my_chrdev.c_dev->dev, 1);
        cdev_del(my_chrdev.c_dev);
        return ret;
    }

    my_chrdev.mydev_class = class_create(THIS_MODULE, "MYLED");
    if (IS_ERR(my_chrdev.mydev_class))
    {
        cdev_del(my_chrdev.c_dev);
        unregister_chrdev_region(my_chrdev.c_dev->dev, 1);
        return PTR_ERR(my_chrdev.mydev_class);
    }

    my_chrdev.mydev = device_create(my_chrdev.mydev_class, NULL, my_chrdev.c_dev->dev, NULL, MYDEV_NAME);
    if (IS_ERR(my_chrdev.mydev))
    {
        device_destroy(my_chrdev.mydev_class, my_chrdev.c_dev->dev);
        class_destroy(my_chrdev.mydev_class);
        cdev_del(my_chrdev.c_dev);
        unregister_chrdev_region(my_chrdev.c_dev->dev, 1);
        return PTR_ERR(my_chrdev.mydev);
    }

    // 初始化 fasync 指针
    my_chrdev.fap = NULL;
    data_ready = 0;

    return 0;
}

// 9. 出口函数
void __exit my_test_module_exit(void)
{
    printk("模块卸载\n");
    // 确保所有异步通知都被移除
    if(my_chrdev.fap) {
        // 实际上 fasync_helper 在 close 时会处理，这里主要是清理资源
        // 强制移除所有关联 (通常不需要手动调，close 会触发 fasync(fd, file, -1))
    }
    
    device_destroy(my_chrdev.mydev_class, my_chrdev.c_dev->dev);
    class_destroy(my_chrdev.mydev_class);
    cdev_del(my_chrdev.c_dev);
    unregister_chrdev_region(my_chrdev.c_dev->dev, 1);
    kfree(my_chrdev.c_dev);
}

MODULE_LICENSE("GPL");
module_init(my_test_module_init);
module_exit(my_test_module_exit);