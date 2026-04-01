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

    // 打开设备（不需要 O_NONBLOCK，poll 自己管理阻塞）
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open failed");
        return -1;
    }
    read(fd,buf,128);
    while (1)
    {
        sleep(1);
    }
    
    close(fd);
    return 0;
}
