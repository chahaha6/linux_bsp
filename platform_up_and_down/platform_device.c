#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

//此代码为名字匹配的platform
// 寄存器物理基地址
#define CCM_CCGR1_BASE     0x20C406C
#define TAMPER3_BASE       0x2290014
#define GPIO5_BASE         0x020AC000

struct resource my_resource_res[] = {
    [0] = {
        .start = CCM_CCGR1_BASE,
        .end   = CCM_CCGR1_BASE + 3,  // 4字节寄存器
        .name  = "CCM_CCGR",
        .flags = IORESOURCE_MEM,      // 标记为内存资源
    },
    [1] = {
        .start = TAMPER3_BASE,
        .end   = TAMPER3_BASE + 3,
        .name  = "TAMPER3",
        .flags = IORESOURCE_MEM,
    },
    [2] = {
        .start = GPIO5_BASE + 0x0,    // GPIO5_DR
        .end   = GPIO5_BASE + 0x3,
        .name  = "GPIO5_DR",
        .flags = IORESOURCE_MEM,
    },
    [3] = {
        .start = GPIO5_BASE + 0x4,    // GPIO5_GDIR
        .end   = GPIO5_BASE + 0x7,
        .name  = "GPIO5_GDIR",
        .flags = IORESOURCE_MEM,
    },
};

void my_release(struct device *dev)
{
    printk("设备释放\n");
}

// 平台设备结构体
struct platform_device my_device = {
    .name = "my_device01", 
    .id   = -1,
    .resource = my_resource_res,
    .num_resources = ARRAY_SIZE(my_resource_res),
    .dev = {
        .release = my_release,
    },
};

static int __init my_dev_init(void)
{
    platform_device_register(&my_device);
    printk(KERN_INFO "平台设备注册成功\n");
    return 0;
}

static void __exit my_dev_exit(void)
{
    platform_device_unregister(&my_device);
    printk(KERN_INFO "平台设备注销\n");
}

MODULE_LICENSE("GPL");
module_init(my_dev_init);
module_exit(my_dev_exit);
