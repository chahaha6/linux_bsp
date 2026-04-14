#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/string.h>
#include <linux/device.h> 
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
struct s_ds18b20
{
    dev_t dev_num;
    struct cdev ds18b20_cdev;
    struct class *ds18b20_class;
    struct device *ds18b20_device;
    int ds18b20_gpio;
};
struct s_ds18b20 *ds18b20;

void ds18b20_reset(void)
{
    int retry = 0;
    // 1. 主机复位脉冲
    gpio_direction_output(ds18b20->ds18b20_gpio, 0);
    udelay(700); 
    // 2. 释放总线
    gpio_direction_input(ds18b20->ds18b20_gpio);
    // 3. 等待存在脉冲（低电平）
    while(gpio_get_value(ds18b20->ds18b20_gpio)) {
     if(retry++ > 1000) { 
            printk(KERN_ERR "DS18B20: No Presence Pulse!\n"); 
            return; 
        }
        udelay(1);
    }
    retry = 0;
    // 4. 等待存在脉冲结束（恢复高电平）
    while(!gpio_get_value(ds18b20->ds18b20_gpio)) {
        if(retry++ > 1000) return; // 超时返回
        udelay(1);
    }
    udelay(480); // 等待时序完成

}


void ds18b20_writebit(unsigned char bit) {
//将GPIO方向设置为输出，并输出高电平
    gpio_direction_output(ds18b20->ds18b20_gpio,1);
    //将GPIO拉低gpiod_set_value(ds18b20->ds18b20_gpio,0);
    //若bit为1，则延时10微秒
    if (bit){
        udelay(10);
        //将GPIO方向设置为输出，并输出高电平
        gpio_direction_output(ds18b20->ds18b20_gpio,1);
    }
    //延时65微秒
    udelay(65);
    //将GPIO方向设置为输出
    gpio_direction_output(ds18b20->ds18b20_gpio,1);
    //延时2微秒
    udelay(2);
}

void ds18b20_writebyte(int data) {
    int i;
    for (i=0; i<8; i++) {
    //逐位写入数据
    ds18b20_writebit(data&0x01);
    data=data>>1;
    }
}

unsigned char ds18b20_readbit(void)
{
    unsigned char bit;
    gpio_direction_output(ds18b20->ds18b20_gpio,1);//将GPIO方向设置为输出，并输出高电平
    gpio_set_value(ds18b20->ds18b20_gpio,0);//将GPIO输出设置为低电平
    udelay(2);//延时2微秒
    gpio_direction_input(ds18b20->ds18b20_gpio);//将GPIO方向设置为输入
    udelay(10);//延时10微秒
    bit=gpio_get_value(ds18b20->ds18b20_gpio);//读取GPIO的值
    udelay(60);//延时60微秒
    return bit;
}
int ds18b20_readbyte(void)
{
    int data=0;
    int i;
    for (i=0; i<8; i++)
    {
    //读取单个位(bit)并根据位的位置进行左移操作
    data|=ds18b20_readbit()<<i;
    }
    return data;
}

int ds18b20_readtemp(void) 
{
    int temp_l, temp_h, temp;
    ds18b20_reset();//复位 ds18b20
    ds18b20_writebyte(0xCC);// 发送写入字节命令 0xCC（跳过ROM）
    ds18b20_writebyte(0x44);// 发送写入字节命令 0x44（启动温度转换）
    mdelay(750);// 延时 750 微秒，等待温度转换完成
    ds18b20_reset();//复位 ds18b20
    ds18b20_writebyte(0xCC);// 发送写入字节命令 0xCC（跳过ROM）
    ds18b20_writebyte(0xBE);// 发送写入字节命令 0xBE（读取温度值）
    temp_l=ds18b20_readbyte();// 读取温度低位字节
    temp_h=ds18b20_readbyte();// 读取温度高位字节
    temp_h=temp_h<<8;//将温度高位字节左移 8 位
    temp=temp_h|temp_l;// 组合温度值
    return temp;
}

int ds18b20_open(struct inode*inode, struct file*file)
{
    return 0;
}

ssize_t ds18b20_read(struct file *file, char __user *buf, size_t size, loff_t*offs) {
    int ds18b20_temp;
    int temp_raw;

    // 1. 检查用户空间缓冲区大小
    if (size < sizeof(int)) {
        return -EINVAL;
    }

    temp_raw = ds18b20_readtemp(); // 假设这是读取的原始值
    // 简单的 0 检查（调试用）
    if (temp_raw == 0) {
        printk(KERN_WARNING "DS18B20: Read 0, Check Wiring or Power!\n");
    }

    // 2. 转换为实际温度 (假设 16位数据，0.0625精度)
    // 这里简单处理，直接返回整数部分 * 100 以便用户层处理
    ds18b20_temp = (temp_raw * 625) / 100; // 约等于 0.0625 倍

    if (copy_to_user(buf, &ds18b20_temp, sizeof(ds18b20_temp))) {
        return -EFAULT; 
    }
    
    // 3. 返回实际拷贝的字节数 (关键修复)
    return sizeof(ds18b20_temp); 
}

int ds18b20_release(struct inode*inode, struct file*file)
{
    return 0;
}

struct file_operations ds18b20_fops = 
{
    .owner = THIS_MODULE,
    .open = ds18b20_open,
    .read = ds18b20_read,
    .release = ds18b20_release,
};

int my_dev_driver_probe(struct platform_device *pdev) {
    int ret;
    
    ds18b20 = kzalloc(sizeof(*ds18b20), GFP_KERNEL);
    if (!ds18b20) return -ENOMEM;

    // 1. 获取 GPIO
    ds18b20->ds18b20_gpio = of_get_named_gpio(pdev->dev.of_node, "gpio", 0); 
    if (!gpio_is_valid(ds18b20->ds18b20_gpio)) {
        printk(KERN_ERR "Invalid GPIO\n");
        ret = -ENODEV;
        goto err_free;
    }

    // 2. 申请 GPIO (关键修复：检查返回值)
    ret = gpio_request(ds18b20->ds18b20_gpio, "ds18b20_io");
    if (ret < 0) {
        printk(KERN_ERR "GPIO request failed\n");
        goto err_free;
    }
    gpio_direction_output(ds18b20->ds18b20_gpio, 1);

    // 3. 注册字符设备
    ret = alloc_chrdev_region(&ds18b20->dev_num, 0, 1, "myds18b20");
    if (ret < 0) goto err_gpio;

    cdev_init(&ds18b20->ds18b20_cdev, &ds18b20_fops);
    ret = cdev_add(&ds18b20->ds18b20_cdev, ds18b20->dev_num, 1);
    if (ret < 0) goto err_cdev;

    ds18b20->ds18b20_class = class_create(THIS_MODULE, "sensors");
    if (IS_ERR(ds18b20->ds18b20_class)) {
        ret = PTR_ERR(ds18b20->ds18b20_class);
        goto err_class;
    }

    ds18b20->ds18b20_device = device_create(ds18b20->ds18b20_class, NULL, ds18b20->dev_num, NULL, "my_ds18b20");
    if (IS_ERR(ds18b20->ds18b20_device)) {
        ret = PTR_ERR(ds18b20->ds18b20_device);
        goto err_device;
    }

    printk("Probe Success! GPIO:%d\n", ds18b20->ds18b20_gpio);
    return 0;

err_device:
    class_destroy(ds18b20->ds18b20_class);
err_class:
    cdev_del(&ds18b20->ds18b20_cdev);
err_cdev:
    unregister_chrdev_region(ds18b20->dev_num, 1);
err_gpio:
    gpio_free(ds18b20->ds18b20_gpio);
err_free:
    kfree(ds18b20);
    return ret;
}

int my_dev_driver_remove(struct platform_device *pdev)
{
    printk("goodbye! \n");
    
    // 安全释放资源，防止 probe 失败导致的空指针崩溃
    if (ds18b20) {
        device_destroy(ds18b20->ds18b20_class, ds18b20->dev_num);
        class_destroy(ds18b20->ds18b20_class);
        cdev_del(&ds18b20->ds18b20_cdev);
        unregister_chrdev_region(ds18b20->dev_num, 1);
        kfree(ds18b20);
        ds18b20 = NULL; // 释放后清零，防止悬空指针
    }
    return 0;
}

struct of_device_id of_node_match_table[] = {
    {.compatible = "my_ds18b20"},
    {}
};
MODULE_DEVICE_TABLE(of, of_node_match_table); // 建议添加此宏，让模块支持自动加载

struct platform_driver ds18b20_driver = {
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
    printk("单总线驱动注册成功\n");
    return 0;
}

void __exit my_test_module_exit(void)
{

    if (ds18b20) {
        device_destroy(ds18b20->ds18b20_class, ds18b20->dev_num);
        class_destroy(ds18b20->ds18b20_class);
        cdev_del(&ds18b20->ds18b20_cdev);
        unregister_chrdev_region(ds18b20->dev_num, 1);
        kfree(ds18b20);
    }
    
    platform_driver_unregister(&ds18b20_driver);
    printk("驱动已卸载\n");
}

module_init(my_test_module_init);
module_exit(my_test_module_exit);
MODULE_LICENSE("GPL");