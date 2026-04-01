#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/xxx_sample_chardev"

int main()
{
    int fd;
    char buf[128] = "hello poll driver!";

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    write(fd, buf, strlen(buf));
    printf("已写入数据：%s\n", buf);

    close(fd);
    return 0;
}
