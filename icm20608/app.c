#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

static int running = 1;

void sigint_handler(int sig)
{
    running = 0;
}

int main(int argc, char *argv[])
{
    int fd;
    char buf[256];
    int ret;
    int count = 0;
    int delay_ms = 500;  // 默认500ms
    
    if (argc > 1) {
        delay_ms = atoi(argv[1]);
        if (delay_ms <= 0) delay_ms = 500;
    }
    
    signal(SIGINT, sigint_handler);
    
    fd = open("/dev/icm20608", O_RDONLY);
    if (fd < 0) {
        perror("无法打开设备 /dev/icm20608");
        return -1;
    }
    
    printf("开始持续读取 ICM20608 数据...\n");
    printf("按 Ctrl+C 退出\n\n");
    
    while (running) {
        memset(buf, 0, sizeof(buf));
        
        ret = read(fd, buf, sizeof(buf) - 1);
        if (ret < 0) {
            perror("读取失败");
            break;
        }
        
        if (ret > 0) {
            count++;
            printf("[%d] ----------------------------------------\n", count);
            printf("%s", buf);
        }
        
        usleep(delay_ms * 1000);
    }
    
    printf("\n共读取 %d 次数据\n", count);
    close(fd);
    return 0;
}