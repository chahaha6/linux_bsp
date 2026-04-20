#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>  
#include <unistd.h>  

int main(int argc, char *argv[])
{
    int fd;
    char angle;

    if (argc != 2) {
        printf("Usage: %s <angle 0-180>\n", argv);
        return -1;
    }

    fd = open("/dev/sg90", O_WRONLY);
    if (fd < 0) {
        printf("Open /dev/sg90 error: %d\n", fd);
        return -1;
    }

    angle = (char)atoi(argv);
    printf("Setting servo to angle: %d\n", angle);<websource>source_group_web_2</websource>

    if (write(fd, &angle, 1) != 1) {
        perror("Write error");
        close(fd);
        return -1;
    }

    sleep(3);

    close(fd);
    return 0;
}