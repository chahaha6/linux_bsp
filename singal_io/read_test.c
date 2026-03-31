#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
// 设备文件路径
#define DEV_PATH "/dev/xxx_sample_chardev"

int fd;

// 信号处理函数：内核发来 SIGIO 时，自动执行这里！
void sigio_handler(int signo)
{
    char buf[128] = {0};
    int ret;

    printf("\n=====================================\n");
    printf("收到内核 SIGIO 信号！设备已就绪\n");

    // 此时 read 一定不会阻塞！
    ret = read(fd, buf, sizeof(buf)-1);
    if(ret > 0)
    {
        printf("读取数据成功：%s\n", buf);
    }
    printf("=====================================\n");
}

int main()
{
    int flags;

    // 1. 打开设备
    fd = open(DEV_PATH, O_RDWR);
    if(fd < 0)
    {
        perror("open failed");
        exit(-1);
    }
    printf("打开设备成功: %s\n", DEV_PATH);

    // 2. 绑定 SIGIO 信号处理函数
    signal(SIGIO, sigio_handler);

    // 3. 设置当前进程为“接收SIGIO信号的进程”
    fcntl(fd, F_SETOWN, getpid());

    // 4. 开启信号驱动IO模式（FASYNC）
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | FASYNC);

    // ==============================
    // 主线程：休眠等待信号
    // 不占CPU、不轮询、不阻塞
    // ==============================
    printf("\n等待内核 SIGIO 通知...\n");
    while(1)
    {
        pause(); // 休眠，直到收到信号
    }

    close(fd);
    return 0;
}
