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


#define DEVICE_NAME "my_ds18b20"
#define MAJOR_NUM 0 // 0表示自动分配主设备号

/* 
 * 硬件寄存器地址定义 
 * 请务必根据你的开发板原理图/数据手册核对这些地址！
 * 这里使用的是你提供的地址 (看起来像 i.MX6ULL 的地址)
 */
#define CCM_CCGR3_BASE          0x020C4074
#define IOMUXC_SW_MUX_BASE      0x020E01E0 // CSI_HSYNC
#define GPIO4_BASE              0x020A8000
#define GPIO4_GDIR_OFFSET       0x0004
#define GPIO4_DR_OFFSET         0x0000

/* 虚拟地址指针 */
static void __iomem *ccm_ccgr3_virt;
static void __iomem *iomuxc_virt;
static void __iomem *gpio4_gdir_virt;
static void __iomem *gpio4_dr_virt;

/* 设备号 */
static int ds18b20_major = MAJOR_NUM;
static int ds18b20_minor = 0;
static struct class *ds18b20_class;
static struct device *ds18b20_device;

/* 
 * 底层 GPIO 操作封装 
 * 对应你原代码中的 ds18b20_gpio_as_input/output 等
 */
static void ds18b20_io_init(void)
{
    u32 val;

    // 1. 使能 GPIO4 时钟 (CCM_CCGR3[CG6] = 0b11)
    writel(readl(ccm_ccgr3_virt) | (3 << 12), ccm_ccgr3_virt);

    // 2. 配置 IOMUXC (CSI_HSYNC -> GPIO4_IO20, Alt5)
    val = readl(iomuxc_virt);
    val &= ~(0xF);
    val |= 0x5; 
    writel(val, iomuxc_virt);

    // 3. 设置 GPIO4_IO20 为输出模式 (初始化为高电平)
    writel(readl(gpio4_gdir_virt) | (1 << 20), gpio4_gdir_virt);
    writel(readl(gpio4_dr_virt) | (1 << 20), gpio4_dr_virt);
}

static void ds18b20_set_output(void)
{
    writel(readl(gpio4_gdir_virt) | (1 << 20), gpio4_gdir_virt);
}

static void ds18b20_set_input(void)
{
    writel(readl(gpio4_gdir_virt) & ~(1 << 20), gpio4_gdir_virt);
}

static void ds18b20_write_bit_val(int val)
{
    if (val)
        writel(readl(gpio4_dr_virt) | (1 << 20), gpio4_dr_virt);
    else
        writel(readl(gpio4_dr_virt) & ~(1 << 20), gpio4_dr_virt);
}

static int ds18b20_read_bit_val(void)
{
    return (readl(gpio4_dr_virt) >> 20) & 0x1;
}

/* 
 * DS18B20 时序操作 
 */
static void ds18b20_delay_us(int us)
{
    udelay(us);
}

// 复位序列
static int ds18b20_reset(void)
{
    int retry = 0;
    int res;
    
    // 1. 主机复位脉冲
    ds18b20_set_output();       // 设置为输出模式
    ds18b20_write_bit_val(0);   // 拉低总线
    ds18b20_delay_us(700);      // 保持低电平 700us (参考代码值)
    
    // 2. 释放总线
    ds18b20_set_input();        // 切换为输入模式
    ds18b20_delay_us(80);       // 等待 15-60us (参考代码使用 80us)
    
    // 3. 等待存在脉冲（低电平）
    retry = 0;
    // 4. 等待存在脉冲结束（恢复高电平）
    ds18b20_delay_us(480);      // 等待时序完成 (参考代码值)
    
    return 0; // 返回 0 表示成功
}

// 写一位
static void ds18b20_write_bit(int val)
{
    if (!val) {
        ds18b20_set_output();
        ds18b20_write_bit_val(0);
        ds18b20_delay_us(60); // 写0：拉低 60-120us
        ds18b20_write_bit_val(1); // 释放
        ds18b20_delay_us(2);
    } else {
        ds18b20_set_output();
        ds18b20_write_bit_val(0);
        ds18b20_delay_us(2);  // 写1：拉低 <15us
        ds18b20_write_bit_val(1); // 释放
        ds18b20_delay_us(60); // 保持高电平直到时隙结束
    }
}

// 写一字节
static void ds18b20_write_byte(unsigned char data)
{
    int i;
    for (i = 0; i < 8; i++) {
        ds18b20_write_bit(data & (1 << i));
    }
}

// 读一位
static int ds18b20_read_bit(void)
{
    int val;
    ds18b20_set_output();
    ds18b20_write_bit_val(0);
    ds18b20_delay_us(2);
    ds18b20_set_input();
    ds18b20_delay_us(10); // 采样时间
    
    val = ds18b20_read_bit_val();
    ds18b20_delay_us(50); // 完成时隙
    return val;
}

// 读一字节
static unsigned char ds18b20_read_byte(void)
{
    int i;
    unsigned char data = 0;
    for (i = 0; i < 8; i++) {
        if (ds18b20_read_bit())
            data |= (1 << i);
    }
    return data;
}

/* 
 * 核心读取温度逻辑 
 * 对应你原代码中的 ds18b20_data_read
 */
static int ds18b20_get_temperature(char *buf)
{
    unsigned char tempL, tempH;
    short temp_raw;
    int integer, decimal;

    // 1. 初始化
    if (ds18b20_reset() != 0) {
        printk(KERN_ERR "DS18B20 Reset Failed!\n");
        return -1;
    }

    // 2. 跳过 ROM (0xCC) + 温度转换 (0x44)
    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0x44);
    
    // 3. 等待转换完成 (实际项目中建议使用延时或中断，这里简单延时)
    // 12位精度最长需要 750ms，这里稍微减少一点等待时间
    msleep(800); 

    // 4. 再次复位 + 跳过 ROM (0xCC) + 读暂存器 (0xBE)
    if (ds18b20_reset() != 0) return -1;
    ds18b20_write_byte(0xCC);
    ds18b20_write_byte(0xBE);

    // 5. 读取数据
    tempL = ds18b20_read_byte();
    tempH = ds18b20_read_byte();

    // 6. 数据处理 (修复了原代码的负数处理逻辑)
    temp_raw = (tempH << 8) | tempL;
    
    if (temp_raw < 0) {
        // 负温度处理：取补码
        temp_raw = -temp_raw;
        integer = temp_raw / 16;
        decimal = (temp_raw % 16) * 10 / 16;
        sprintf(buf, "-%d.%d", integer, decimal);
    } else {
        // 正温度处理
        integer = temp_raw / 16;
        decimal = (temp_raw % 16) * 10 / 16;
        sprintf(buf, "%d.%d", integer, decimal);
    }

    return 0;
}

/* 
 * 文件操作接口 
 */
static int ds18b20_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t ds18b20_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char temp_str[20];
    int ret;

    if (*ppos > 0) return 0; // 防止重复读取

    ret = ds18b20_get_temperature(temp_str);
    if (ret < 0) {
        return -EIO;
    }

    // 将数据拷贝到用户空间
    if (copy_to_user(buf, temp_str, strlen(temp_str))) {
        return -EFAULT;
    }

    *ppos += strlen(temp_str);
    return strlen(temp_str);
}

static int ds18b20_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations ds18b20_fops = {
    .owner = THIS_MODULE,
    .open = ds18b20_open,
    .read = ds18b20_read,
    .release = ds18b20_release,
};

/* 
 * 模块加载与卸载 
 */
static int __init ds18b20_init(void)
{
    int ret;

    // 1. 注册字符设备
    ds18b20_major = register_chrdev(ds18b20_major, DEVICE_NAME, &ds18b20_fops);
    if (ds18b20_major < 0) {
        printk(KERN_ERR "Register chrdev failed!\n");
        return ds18b20_major;
    }

    // 2. 创建类
    ds18b20_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(ds18b20_class)) {
        unregister_chrdev(ds18b20_major, DEVICE_NAME);
        printk(KERN_ERR "Create class failed!\n");
        return PTR_ERR(ds18b20_class);
    }

    // 3. 创建设备节点 (/dev/ds18b20)
    ds18b20_device = device_create(ds18b20_class, NULL, MKDEV(ds18b20_major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(ds18b20_device)) {
        class_destroy(ds18b20_class);
        unregister_chrdev(ds18b20_major, DEVICE_NAME);
        printk(KERN_ERR "Create device failed!\n");
        return PTR_ERR(ds18b20_device);
    }

    // 4. 内存映射 (关键步骤：将物理地址映射到内核虚拟地址)
    ccm_ccgr3_virt = ioremap(CCM_CCGR3_BASE, 4);
    iomuxc_virt = ioremap(IOMUXC_SW_MUX_BASE, 4);
    gpio4_gdir_virt = ioremap(GPIO4_BASE + GPIO4_GDIR_OFFSET, 4);
    gpio4_dr_virt = ioremap(GPIO4_BASE + GPIO4_DR_OFFSET, 4);

    if (!ccm_ccgr3_virt || !iomuxc_virt || !gpio4_gdir_virt || !gpio4_dr_virt) {
        printk(KERN_ERR "ioremap failed!\n");
        // 实际应在此处做错误清理
    }

    // 5. 初始化 GPIO
    ds18b20_io_init();
    
    printk(KERN_INFO "DS18B20 Driver loaded successfully!\n");
    return 0;
}

static void __exit ds18b20_exit(void)
{
    // 取消映射
    iounmap(ccm_ccgr3_virt);
    iounmap(iomuxc_virt);
    iounmap(gpio4_gdir_virt);
    iounmap(gpio4_dr_virt);

    device_destroy(ds18b20_class, MKDEV(ds18b20_major, 0));
    class_destroy(ds18b20_class);
    unregister_chrdev(ds18b20_major, DEVICE_NAME);
    printk(KERN_INFO "DS18B20 Driver unloaded.\n");
}

module_init(ds18b20_init);
module_exit(ds18b20_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lai Mu");
MODULE_DESCRIPTION("DS18B20 Temperature Sensor Driver");