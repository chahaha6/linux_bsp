#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/stat.h>

struct i2c_client *ap3216_client;
struct cdev *ap3216_dev;
dev_t ap3216_devt;
struct class *ap3216_class;
struct device *ap3216_device;
/* AP3216C 寄存器定义 */
#define AP3216C_SYSTEMCONFIG    0x00    // 系统配置寄存器
#define AP3216C_ALSDATALOW      0x0C    // ALS 数据低字节
#define AP3216C_ALSDATAHIGH     0x0D    // ALS 数据高字节
#define AP3216C_PSDATALOW       0x0E    // PS 数据低字节
#define AP3216C_PSDATAHIGH      0x0F    // PS 数据高字节
#define AP3216C_IRDATALOW       0x0A    // IR 数据低字节
#define AP3216C_IRDATAHIGH      0x0B    // IR 数据高字节

static void ap3216_write_reg(u8 reg_addr, u8 data, u8 len);
static int ap3216_read_reg(u8 reg_addr);
int ap3216_open(struct inode *inode, struct file *file)
{
    printk("file is open \n");
    return 0;
}
int ap3216_close(struct inode *inode, struct file *file)
{
    printk("file is close \n");
    return 0;

}
ssize_t ap3216_read(struct file *file, char __user *userbuf, size_t size, loff_t *offset)
{
    u8 als_low, als_high;
    u8 ps_low, ps_high;
    u8 ir_low, ir_high;
    uint16_t als = 0, ps = 0, ir = 0;
    uint16_t data_buf[3];
    als_low = ap3216_read_reg(AP3216C_ALSDATALOW);
    als_high = ap3216_read_reg(AP3216C_ALSDATAHIGH);
    als = ((uint16_t)als_high << 8) | als_low;
    
    // 读取 PS (接近传感器) 数据
    ps_low = ap3216_read_reg(AP3216C_PSDATALOW);
    ps_high = ap3216_read_reg(AP3216C_PSDATAHIGH);
    // 参考裸机代码的处理逻辑
    if (ps_low & 0x40) {
        ps = 0;  // 无效数据
    } else {
        ps = ((uint16_t)(ps_high & 0x3F) << 4) | (ps_low & 0x0F);
    }
    
    // 读取 IR (红外) 数据
    ir_low = ap3216_read_reg(AP3216C_IRDATALOW);
    ir_high = ap3216_read_reg(AP3216C_IRDATAHIGH);
    if (ir_low & 0x80) {
        ir = 0;  // 无效数据
    } else {
        ir = ((uint16_t)ir_high << 2) | (ir_low & 0x03);
    }
    data_buf[0] = als;
    data_buf[1] = ps;
    data_buf[2] = ir;
    copy_to_user(userbuf, data_buf, sizeof(data_buf));
    return sizeof(data_buf);

}

struct file_operations ap3216_fops = {
    .owner =THIS_MODULE,
    .open = ap3216_open,
    .read = ap3216_read,
    .release = ap3216_close,
};
static void ap3216_write_reg(u8 reg_addr, u8 data, u8 len)
{
    u8 buff[256];
    struct i2c_msg msgs[] = {
        [0] = {
            .addr = ap3216_client->addr,
            .flags = 0,
            .len = len + 1,
            .buf = buff,
        },
    };
    buff[0] = reg_addr;
    memcpy(&buff[1], &data, len);
    i2c_transfer(ap3216_client->adapter, msgs, 1);
}

static int ap3216_read_reg(u8 reg_addr)
{
    u8 data;
    struct i2c_msg msgs[] = {
        [0] = {
            .addr = ap3216_client->addr,
            .flags = 0,
            .len = sizeof(reg_addr),
            .buf = &reg_addr,
        },
        [1] = {
            .addr = ap3216_client->addr,
            .flags = I2C_M_RD,
            .len = sizeof(data),
            .buf = &data,
        },
    };
    i2c_transfer(ap3216_client->adapter, msgs, 2);
    return data;
}

const struct of_device_id ap3216_id[] = {
    { .compatible = "ap3216" },
    {}
};

static const struct i2c_device_id ap3216_ids[] = {
    {}
};

int ap3216_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    printk("client: probe called, addr=0x%x\n", client->addr);
    ap3216_client = client;
    printk("ap3216: probe called, addr=0x%x\n", ap3216_client->addr);
    
    /* ----- 初始化芯片 ----- */
    // 1. 软件复位
    printk("ap3216: writing 0x04 to reg 0x00 (reset)\n");
    ap3216_write_reg(AP3216C_SYSTEMCONFIG, 0x04, 1);
    mdelay(15);
    
    // 2. 启动所有功能 (ALS + PS + IR)
    printk("ap3216: writing 0x03 to reg 0x00 (start all)\n");
    ap3216_write_reg(AP3216C_SYSTEMCONFIG, 0x03, 1);
    mdelay(10);  // 等待第一次转换完成
    
    ap3216_dev = cdev_alloc();
    cdev_init(ap3216_dev, &ap3216_fops);
    ret = alloc_chrdev_region(&ap3216_devt, 0, 1, "ap3216");
    if(ret < 0)
    {
        printk("alloc_chrdev_region is error");
    }
    cdev_add(ap3216_dev, ap3216_devt, 1);
    
    ap3216_class = class_create(THIS_MODULE, "sensor");
    ap3216_device = device_create(ap3216_class, NULL,ap3216_devt, NULL, "ap3216");

    return 0;
}

int ap3216_remove(struct i2c_client *client)
{
    printk("ap3216: remove called\n");
    device_destroy(ap3216_class, ap3216_devt);
    class_destroy(ap3216_class);
    cdev_del(ap3216_dev);
    unregister_chrdev_region(ap3216_devt,1);
    kfree(ap3216_dev);

    // 关闭所有功能，进入掉电模式
    return 0;
}

static struct i2c_driver ap3216_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "ap3216",
        .of_match_table = ap3216_id,
    },
    .probe = ap3216_probe,
    .remove = ap3216_remove,
    .id_table = ap3216_ids,  
};

int __init my_test_module_init(void)
{
    int ret;
    printk(KERN_INFO "ap3216: init start\n");
    ret = i2c_add_driver(&ap3216_driver);
    if (ret < 0) {
        printk( "ap3216: i2c_add_driver failed, ret=%d\n", ret);
        return ret;  
    }
    printk( "ap3216: init ok, driver registered\n");
    return 0;
}

void __exit my_test_module_exit(void)
{
    printk( "ap3216: exit\n");
    i2c_del_driver(&ap3216_driver);
}

MODULE_LICENSE("GPL");
module_init(my_test_module_init);
module_exit(my_test_module_exit);