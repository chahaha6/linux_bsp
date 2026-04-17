#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEV_PATH "/dev/my_ds18b20"

int main()
{
    int fd;
    char buffer[32];
    int len;

    // 1. 打开设备文件
    fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Open device failed");
        printf("提示：请确认驱动是否已加载 (lsmod)，设备节点是否存在 (ls /dev/ds18b20)\n");
        return -1;
    }

    printf("打开设备成功，开始读取温度...\n");

    // 2. 循环读取
// 修改 app.c 中的循环部分
    while (1) { 
    // 每次循环都重新打开设备
    fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Open device failed");
        sleep(2);
        continue;
    }

    memset(buffer, 0, sizeof(buffer));
    len = read(fd, buffer, sizeof(buffer) - 1);
    if (len > 0) {
        printf("当前温度: %s ℃\n", buffer);
    } else {
        // 当 read 返回 0 时，表示本次读取结束，不是错误
        // 只有当 len < 0 时才是真正的错误
        if (len < 0) {
            perror("Read device failed");
        }
    }

    close(fd); // 读取后立即关闭
    sleep(2);
}

    close(fd);
    return 0;
}