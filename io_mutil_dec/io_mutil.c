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
    // 等待队列的属性：
    wait_queue_head_t wait_queue;
    int8_t condition;
};

char kernel_buf[128] = {0};
// 1.在全局中定义的 xxx_sample_charde的对象
struct xxx_sample_chardev my_chrdev;

// 在内核定义对应的函数接口：
// 与文件read对应的函数指针
ssize_t xxx_sample_chardev_read(struct file *file, char __user *userbuf, size_t size, loff_t *offset)
{
    // 先检查一文件的标记：
    if (file->f_flags & O_NONBLOCK)
    {
        if (my_chrdev.condition == 0)
        {
            // 条件不满足，则直接返回。s
            return -EAGAIN;
        }
        else
        {
            if (size >= sizeof(kernel_buf))
            {
                size = sizeof(userbuf);
            }
            copy_to_user(userbuf, kernel_buf + *offset, size);
            my_chrdev.condition = 0;
            return size;
        }
    }

    // 此时当前进程不满足条件在此阻塞：
    wait_event_interruptible(my_chrdev.wait_queue, my_chrdev.condition);
    my_chrdev.condition = 0;
    if (size >= sizeof(kernel_buf))
    {
        size = sizeof(kernel_buf);
    }
    copy_to_user(userbuf, kernel_buf + *offset, size);
    printk("内核中的xxx_sample_chardev_read执行了\n");
    return size;
}
// 与write对应的函数指针
ssize_t xxx_sample_chardev_write(struct file *file, const char __user *usrbuf, size_t size, loff_t *offset)
{
    // 回调函数中参数：
    // 参数1：就是用户进程中使用open内核中创建的struct file的实例的地址。
    // 参数2：usrbuf:就是用户进程中数据的地址。
    // 参数3: size,用户进程中要拷贝的字节数。
    // 参数4：当前文件的偏移量。单元也是字节。

    // 调用copy_from_user从用户进程中获取数据：
    int ret = 0;
    if (size >= sizeof(kernel_buf))
    {
        size = sizeof(kernel_buf);
    }
    memset(kernel_buf, 0, sizeof(kernel_buf));

    ret = copy_from_user(kernel_buf + *offset, usrbuf, size);
    if (ret)
    {
        printk("copy_from_usre failed");
        return -EIO;
    }
    printk("内核中的xxx_sample_chardev_write执行了kf[0] = %s\n", kernel_buf);
    // 唤醒等待队列中的进程：
    my_chrdev.condition = 1;
    wake_up_interruptible(&my_chrdev.wait_queue);
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
//这就回调函数就是IO多路复用机制中进行回调的调函数。
unsigned int xxx_sample_chardev_poll(struct file * file, struct poll_table_struct * table)
{
    int mask = 0;
    //1.把fd指定的设备中的等待队列挂载到wait列表。
    poll_wait(file, &my_chrdev.wait_queue, table);
    //2.如果条件满足，置位相位相应标记掩码mask:
    //POLL_IN 只读事件产生的code, POLL_OUT只写事件， POLL_ERR错误事件，...
    if(my_chrdev.condition == 1)
    {
        return mask | POLL_IN;
    }

    return mask;

}

struct file_operations fops = {
    .open = xxx_sample_chardev_open,
    .read = xxx_sample_chardev_read,
    .write = xxx_sample_chardev_write,
    .release = xxx_sample_chardev_release,
    .poll = xxx_sample_chardev_poll,
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
    init_waitqueue_head(&my_chrdev.wait_queue);
    my_chrdev.condition = 0;

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