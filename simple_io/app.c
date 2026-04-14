#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// 1. 定义全局变量
char g_sig;     // 用于存储符号
float g_temp;   // 用于存储温度值

void ds18b20_get_temp(int value) 
{
    if ((value >> 11) & 0x01) {
        g_sig = '-';
        value = ~value + 1; 
        value &= ~(0xF8 << 8);  
    } else {
        g_sig = '+';
    }
    g_temp = value * 0.0625; 
}

int main(int argc, char*argv[]) {
    int fd;
    int data;
    
    fd = open("/dev/my_ds18b20", O_RDWR); 
    if (fd < 0) {
        printf("打开文件失败！\n");
        return -1;
    }

    while(1) {
        if (read(fd, &data, sizeof(data)) < 0) { 
            printf("读取数据失败！\n");
            break; 
        }
        
        ds18b20_get_temp(data); 
        
        printf("温度为 %c%.4f\n", g_sig, g_temp); 
        
        sleep(1); 
    }
    
    close(fd); 
    return 0;
}