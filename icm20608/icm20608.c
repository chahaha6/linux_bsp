#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

/* ICM20608 寄存器定义 */
#define ICM20608_WHO_AM_I        0x75
#define ICM20608_PWR_MGMT_1      0x6B
#define ICM20608_ACCEL_CONFIG    0x1C
#define ICM20608_GYRO_CONFIG     0x1B
#define ICM20608_ACCEL_XOUT_H    0x3B

/* 配置值 */
#define ICM20608_ACCEL_FS_2G     0x00
#define ICM20608_GYRO_FS_250DPS  0x00
#define ICM20608_PWR_RESET       0x80
#define ICM20608_PWR_WAKE_UP     0x01

struct cdev *icm20608_dev;
dev_t icm20608_devt;
struct class *icm20608_class;
struct device *icm20608_device;
struct spi_device *icm20608_spi;

static int icm20608_spi_rw(struct spi_device *spi, u8 *tx_buf, u8 *rx_buf, size_t len)
{
    struct spi_transfer t = {
        .tx_buf = tx_buf,
        .rx_buf = rx_buf,
        .len = len,
    };
    struct spi_message m;
    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    return spi_sync(spi, &m);
}
/* 写寄存器 */
static int icm20608_write_reg(struct spi_device *spi, u8 reg, u8 value)
{
    u8 tx_buf[2] = {reg & 0x7F, value};
    return icm20608_spi_rw(spi, tx_buf, NULL, 2);
}

/* 读单个寄存器 */
static int icm20608_read_reg(struct spi_device *spi, u8 reg, u8 *value)
{
    u8 tx_buf[2] = {reg | 0x80, 0x00};
    u8 rx_buf[2];
    int ret = icm20608_spi_rw(spi, tx_buf, rx_buf, 2);
    *value = rx_buf[1];
    return ret;
}

/* 连续读多个寄存器 */
static int icm20608_read_regs(struct spi_device *spi, u8 reg, u8 *data, size_t len)
{
    u8 *tx_buf = kmalloc(len + 1, GFP_KERNEL);
    int ret;
    
    if (!tx_buf)
        return -ENOMEM;
    
    tx_buf[0] = reg | 0x80;
    memset(&tx_buf[1], 0x00, len);
    
    ret = icm20608_spi_rw(spi, tx_buf, data, len + 1);
    kfree(tx_buf);
    return ret;
}

int icm20608_open(struct inode *node , struct file *fd)
{
    printk("open fd is ok\n");
    return 0;
}

ssize_t icm20608_read(struct file *fd, char __user *userbuf, size_t size, loff_t *offset)
{
    printk("read is ok\n");

    u8 data[14];
    s16 accel_x, accel_y, accel_z;
    s16 gyro_x, gyro_y, gyro_z;
    s16 temp;
    char buf[256];
    int len;
    
    if (!icm20608_spi) {
        printk(KERN_ERR "icm20608: spi device not available\n");
        return -ENODEV;
    }
    
    /* 连续读取 14 字节传感器数据 */
    if (icm20608_read_regs(icm20608_spi, ICM20608_ACCEL_XOUT_H, data, 14) < 0) {
        printk(KERN_ERR "icm20608: failed to read sensor data\n");
        return -EIO;
    }
    
    /* 拼接数据（高位在前） */
    accel_x = (s16)((data[0] << 8) | data[1]);
    accel_y = (s16)((data[2] << 8) | data[3]);
    accel_z = (s16)((data[4] << 8) | data[5]);
    temp    = (s16)((data[6] << 8) | data[7]);
    gyro_x  = (s16)((data[8] << 8) | data[9]);
    gyro_y  = (s16)((data[10] << 8) | data[11]);
    gyro_z  = (s16)((data[12] << 8) | data[13]);
    
    /* 格式化输出 */
    len = snprintf(buf, sizeof(buf),
            "ACCEL: X=%6d, Y=%6d, Z=%6d\n"
            "GYRO:  X=%6d, Y=%6d, Z=%6d\n"
            "TEMP:  %6d\n",
            accel_x, accel_y, accel_z,
            gyro_x, gyro_y, gyro_z, temp);
    
    if (size > len)
        size = len;
    if (copy_to_user(userbuf, buf, size)) {
        printk(KERN_ERR "icm20608: copy_to_user failed\n");
        return -EFAULT;
    }
    
    printk("icm20608: read %zu bytes\n", size);
    return size;

}

int icm20608_release(struct inode *node, struct file *fd)
{
    printk("file is close\n");
}
struct file_operations icm20608_fops ={
    .owner = THIS_MODULE,
    .open = icm20608_open,
    .read = icm20608_read,
    .release = icm20608_release,
};


static const struct of_device_id icm20608_id[] = {
	{ .compatible = "my_icm20608" },
	{ }
};

static const struct spi_device_id icm20608_ids[] = {
	{ "my_icm20608" },
	{ }
};

static int icm20608_probe(struct spi_device *spi)
{
	int ret = 0;
    u8 whoami;
    printk(KERN_INFO "icm20608: PROBE 成功！！！\n");

    icm20608_spi = spi;

    icm20608_dev = cdev_alloc();
    cdev_init(icm20608_dev, &icm20608_fops);
    ret = alloc_chrdev_region(&icm20608_devt, 0, 1, "icm20608");
    if(ret < 0)
    {
        printk("alloc_chrdev_region is error");
    }
    cdev_add(icm20608_dev, icm20608_devt, 1);
    
    icm20608_class = class_create(THIS_MODULE, "sensor");
    icm20608_device = device_create(icm20608_class, NULL,icm20608_devt, NULL, "icm20608");

    icm20608_write_reg(spi, ICM20608_PWR_MGMT_1, ICM20608_PWR_RESET);
    mdelay(10);
    // 唤醒设备
    icm20608_write_reg(spi, ICM20608_PWR_MGMT_1, ICM20608_PWR_WAKE_UP);
    mdelay(10);
    
    // 配置加速度计和陀螺仪
    icm20608_write_reg(spi, ICM20608_ACCEL_CONFIG, ICM20608_ACCEL_FS_2G);
    icm20608_write_reg(spi, ICM20608_GYRO_CONFIG, ICM20608_GYRO_FS_250DPS);
    
    // 读取设备 ID
    icm20608_read_reg(spi, ICM20608_WHO_AM_I, &whoami);
    if (whoami == 0xAF) {
        printk(KERN_INFO "icm20608: WHO_AM_I = 0x%02x (OK)\n", whoami);
    } else {
        printk(KERN_WARNING "icm20608: WHO_AM_I = 0x%02x (Expected 0xAF)\n", whoami);
    }
    
	return 0;
}

static int icm20608_remove(struct spi_device *spi)
{
	printk(KERN_INFO "icm20608: remove\n");
	return 0;
}

struct spi_driver icm20608_driver = {
	.driver = {
		.name = "my_icm20608",
		.owner = THIS_MODULE,
		.of_match_table = icm20608_id,  /* 现在能找到了 */
	},
	.probe = icm20608_probe,
	.remove = icm20608_remove,
	.id_table = icm20608_ids,
};

static int __init my_init(void)
{
	printk("icm20608: 驱动加载\n");
	return spi_register_driver(&icm20608_driver);
}

static void __exit my_exit(void)
{
	spi_unregister_driver(&icm20608_driver);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
