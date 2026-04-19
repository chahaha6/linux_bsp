#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

/* 设置串口参数：波特率、数据位、校验位、停止位 */
int set_uart(int fd, int speed, int bits, char check, int stop)
{
    struct termios newtio, oldtio;

    // 保存原有串口配置
    if (tcgetattr(fd, &oldtio) != 0) {
        perror("tcgetattr");
        return -1;
    }

    // 清零 newtio
    bzero(&newtio, sizeof(newtio));

    // 控制模式标志：启用接收器和本地连接
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    // 数据位
    switch (bits) {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr, "Unsupported data bits: %d\n", bits);
        return -1;
    }

    // 奇偶校验位
    switch (check) {
    case 'O':   // 奇校验 (Odd)
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':   // 偶校验 (Even)
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'N':   // 无校验 (None)
        newtio.c_cflag &= ~PARENB;
        break;
    default:
        fprintf(stderr, "Unsupported parity: %c\n", check);
        return -1;
    }

    // 波特率
    switch (speed) {
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    default:
        fprintf(stderr, "Unsupported baud rate: %d\n", speed);
        return -1;
    }

    // 停止位
    switch (stop) {
    case 1:
        newtio.c_cflag &= ~CSTOPB;  // 1位停止位
        break;
    case 2:
        newtio.c_cflag |= CSTOPB;   // 2位停止位
        break;
    default:
        fprintf(stderr, "Unsupported stop bits: %d\n", stop);
        return -1;
    }

    // 刷新输入队列
    tcflush(fd, TCIFLUSH);

    // 使配置立即生效
    if (tcsetattr(fd, TCSANOW, &newtio) != 0) {
        perror("tcsetattr");
        return -2;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int fd;
    char *send_str = "Hello UART5!";
    char buf[256];
    int count;

    if (argc > 1) {
        send_str = argv[1];
    }

    fd = open("/dev/ttymxc5", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("open /dev/ttymxc5");
        fprintf(stderr, "请检查设备节点是否存在，或尝试其他端口如 /dev/ttymxc2 等。\n");
        return -1;
    }

    // 设置串口参数：115200, 8位数据, 无校验, 1位停止位
    if (set_uart(fd, 115200, 8, 'N', 1) < 0) {
        fprintf(stderr, "串口配置失败\n");
        close(fd);
        return -1;
    }

    // 发送数据
    count = write(fd, send_str, strlen(send_str));
    if (count < 0) {
        perror("write");
        close(fd);
        return -1;
    }
    printf("已发送: %s (长度=%d)\n", send_str, count);

    // 等待数据回环（因为硬件 TX 和 RX 短接）
    usleep(100000);  // 延时 100ms

    // 读取回环数据
    memset(buf, 0, sizeof(buf));
    count = read(fd, buf, sizeof(buf) - 1);
    if (count < 0) {
        perror("read");
        close(fd);
        return -1;
    } else if (count == 0) {
        printf("未收到数据！请检查 TX/RX 是否短接，或设备是否正确。\n");
    } else {
        buf[count] = '\0';
        printf("接收到: %s (长度=%d)\n", buf, count);
    }

    // 关闭设备
    close(fd);
    return 0;
}