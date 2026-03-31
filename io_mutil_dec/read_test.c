#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#define DEVICE_PATH "/dev/xxx_sample_chardev"

int main()
{
    int fd;
    int ret;
    char buf[128];
    struct pollfd fds[1];  // poll 结构体

    // 打开设备（不需要 O_NONBLOCK，poll 自己管理阻塞）
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }

    // 配置 poll：监听读事件
    fds[0].fd = fd;
    fds[0].events = POLLIN;  // 监听“可读”事件

    printf("=== poll 等待驱动数据... ===\n");

    while (1)
    {
        // poll 阻塞等待，-1 表示永久等待
        ret = poll(fds, 1, -1);

        if (ret < 0) {
            perror("poll error");
            break;
        }

        // 判断是否是读事件触发
        if (fds[0].revents & POLLIN)
        {
            memset(buf, 0, sizeof(buf));
            ret = read(fd, buf, sizeof(buf)-1);

            if (ret > 0) {
                printf("✅ poll 触发成功！读到数据：%s\n", buf);
            }
        }
    }

    close(fd);
    return 0;
}
