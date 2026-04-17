#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

struct i2c_client *ap3216_client;

static void ap3216_write_reg(u8 reg_addr, u8 data, u8 len);
static int ap3216_read_reg(u8 reg_addr);

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
            .flags = I2C_M_RD,  // 用宏代替 1，更清晰
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
    int read_val;
    u8 chip_id;

    printk("client: probe called, addr=0x%x\n", client->addr);
    ap3216_client = client;
    printk("ap3216: probe called, addr=0x%x\n", ap3216_client->addr);
    
    // AP3216 系统配置寄存器地址为 0x00，写入 0x04 进行软件复位
    printk("ap3216: writing 0x04 to reg 0x00\n");
    ap3216_write_reg(0x00, 0x04, 1);
    
    // 等待复位完成
    mdelay(15);
    
    read_val = ap3216_read_reg(0x00);
    printk("ap3216: read back 0x%02x from reg 0x00\n", read_val);
    /* ----- 测试结束 ----- */
    chip_id = ap3216_read_reg(0x10);
    printk("ap3216: chip id = 0x%02x\n", chip_id);
    return 0;
}

int ap3216_remove(struct i2c_client *client)
{
    printk("ap3216: remove called\n");
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