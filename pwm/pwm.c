#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/fs.h>
#include <linux/of.h> // 包含 of_device_id 定义

// 1. 定义设备号变量，而不是指针
dev_t sg90_devt; 
struct cdev sg90_cdev; // 定义 cdev 结构体实体
struct class *sg90_class;
struct device *sg90_device;
struct pwm_device *sg90_pwm;

int sg90_open(struct inode *node , struct file *fd)
{
    printk("open fd is ok\n");
    pwm_config(sg90_pwm, 500000, 20000000); //设置初始脉冲宽度为 0.5ms, 周期为 20ms
    pwm_set_polarity(sg90_pwm, PWM_POLARITY_NORMAL); //设置 pwm 极性为正极性
    pwm_enable(sg90_pwm); //启动 pwm 输出，使舵机归中位
    return 0;
}

// 2. 添加返回值
int sg90_release(struct inode *node, struct file *fd)
{
    printk("file is close\n");
    pwm_config(sg90_pwm, 500000, 20000000); //设置初始脉冲宽度为 0.5ms, 周期为 20ms
    pwm_disable(sg90_pwm);
    return 0;
}

ssize_t sg90_write(struct file *fd, const char __user *buf, size_t size, loff_t *offset)
{
    int ret;
    unsigned char data[1];
    printk("This is cdev test write\n");
    //从用户空间拷贝数据到内核空间
    ret = copy_from_user(data, buf, size);
    if (ret) {
        printk("copy_from_user failed\n");
        return -EFAULT;
    }
    pwm_config(sg90_pwm, 500000 + data[0] * 100000 / 9, 20000000);
    return size;
}

struct file_operations sg90_fops ={
    .owner = THIS_MODULE,
    .open = sg90_open,
    .release = sg90_release,
    .write = sg90_write,
};

// 设备树匹配表
static const struct of_device_id sg90_match_table[] = {
    { .compatible = "sg90" },
    { }
};
MODULE_DEVICE_TABLE(of, sg90_match_table);

// 平台设备ID表 (如果不需要兼容旧式设备注册，可以省略)
static const struct platform_device_id sg90_device_table[] = {
    { .name = "sg90" }, // 修正语法错误，使用 .name
    { }
};
MODULE_DEVICE_TABLE(platform, sg90_device_table);


static int sg90_probe(struct platform_device *pdev)
{
    int ret = 0;
    printk(KERN_INFO "probe is ok \n");

    // 获取 PWM 设备
    // 注意：devm_pwm_get 第一个参数是 &pdev->dev
    sg90_pwm = devm_pwm_get(&pdev->dev, NULL);
    if (IS_ERR(sg90_pwm)) {
        dev_err(&pdev->dev, "Unable to request PWM\n");
        return PTR_ERR(sg90_pwm);
    }

    // 4. 修正字符设备注册逻辑
    // 传递 &sg90_devt 而不是 sg90_dev
    ret = alloc_chrdev_region(&sg90_devt, 0, 1, "sg90_name");
    if(ret < 0)
    {
        printk(KERN_ERR "alloc_chrdev_region error: %d\n", ret);
        return ret; // 注册失败直接返回
    }

    // 初始化 cdev，传递结构体地址
    cdev_init(&sg90_cdev, &sg90_fops);
    
    // 添加 cdev
    ret = cdev_add(&sg90_cdev, sg90_devt, 1);
    if (ret < 0) {
        unregister_chrdev_region(sg90_devt, 1);
        return ret;
    }

    // 创建类
    sg90_class = class_create(THIS_MODULE, "sg90");
    if (IS_ERR(sg90_class)) {
        cdev_del(&sg90_cdev);
        unregister_chrdev_region(sg90_devt, 1);
        return PTR_ERR(sg90_class);
    }

    // 创建设备节点 /dev/sg90
    sg90_device = device_create(sg90_class, NULL, sg90_devt, NULL, "sg90");
    if (IS_ERR(sg90_device)) {
        class_destroy(sg90_class);
        cdev_del(&sg90_cdev);
        unregister_chrdev_region(sg90_devt, 1);
        return PTR_ERR(sg90_device);
    }

    return 0;
}
static int sg90_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "sg90: remove\n");
    // 资源释放 (虽然 devm_ 会自动释放 pwm，但字符设备需要手动释放)
    device_destroy(sg90_class, sg90_devt);
    class_destroy(sg90_class);
    cdev_del(&sg90_cdev);
    unregister_chrdev_region(sg90_devt, 1);
    return 0;
}

static struct platform_driver sg90_driver = {
    .probe = sg90_probe,
    .remove = sg90_remove,
    .driver = {
        .name = "sg90",
        .of_match_table = sg90_match_table,
    },
    .id_table = sg90_device_table,
};

static int __init my_init(void)
{
    int ret;
    printk("sg90: 驱动加载\n");
    // 5. 注册驱动，而不是设备
    ret = platform_driver_register(&sg90_driver);
    if(ret < 0 )
    {
        printk("platform_driver_register is error: %d\n", ret);
    }
    return ret;
}

static void __exit my_exit(void)
{
    printk("sg90: 驱动卸载\n");
    platform_driver_unregister(&sg90_driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");