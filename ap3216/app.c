/*
 * ap3216_test.c - AP3216C 传感器测试程序
 * 编译: gcc -o ap3216_test ap3216_test.c
 * 运行: ./ap3216_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
    int fd;
    unsigned short data[3];
    
    fd = open("/dev/ap3216", O_RDONLY);
    if (fd < 0) {
        printf("无法打开设备 /dev/ap3216\n");
        return -1;
    }
    
    while (1) {
        lseek(fd, 0, SEEK_SET);  // 重置文件偏移
        
        if (read(fd, data, sizeof(data)) == sizeof(data)) {
            printf("ALS: %-5u  PS: %-5u  IR: %-5u\n", data[0], data[1], data[2]);
        } else {
            printf("读取失败\n");
        }
        
        usleep(500000);  // 500ms
    }
    
    close(fd);
    return 0;
}