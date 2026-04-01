#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
//asdasdasd
#define DEV_PATH "/dev/xxx_sample_chardev"

int main()
{
    int fd;
    char buf[128] = "Hello SIGIO! 信号驱动IO测试成功";

    fd = open(DEV_PATH, O_RDWR);
    if(fd < 0)
    {
        perror("open");
        return -1;
    }

    write(fd, buf, strlen(buf));
    printf("数据已写入驱动！\n");

    close(fd);
    return 0;
}
