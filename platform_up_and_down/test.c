#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h> 

int main(int argc, char const *argv[])
{
    int fd = open("/dev/my_device01", O_RDWR);
    if (fd == -1)
    {
        perror("open err");
        return -1;
    }
    char buf[128] = {0};
    int nbytes = 0;
    while (true)
    {
        printf("请输入：\n");
        fgets(buf,sizeof(buf),stdin);
        nbytes = write(fd,buf,strlen(buf));
        if(nbytes == -1)
        {
            perror("write err:");
            return -1;
        }
    }

    close(fd);

    return 0;
}