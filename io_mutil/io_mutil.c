#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/input.h>

int main(int argc, char const *argv[])
{
    int fd1 = open("/dev/tty", O_RDWR);
    if (fd1 == -1)
    {
        perror("open err");
        return -1;
    }
    int fd2 = open("/dev/input/mouse0",O_RDONLY);
    if(fd2 == -1)
    {
        perror("open err:");
        return -1;
    }
    int nbyts = 0;
    char buf[128] = {0};
    struct input_absinfo mouseInfo = {0};
    //1.创建fd的集合（select中使用一个bitmap位域图）
    fd_set save_fd_set, modify_fd_set;
    //对集合进行清0：
    FD_ZERO(&save_fd_set);
    //2.把我们关心fd放入到集合中：
    FD_SET(fd1,&save_fd_set);
    FD_SET(fd2,&save_fd_set);

    //3.确定监控fd集合的边界：
    int maxfd = fd1 > fd2 ? fd1 : fd2;
    while (true)
    {
        modify_fd_set = save_fd_set;
        int fds = select(maxfd + 1,&modify_fd_set,NULL,NULL,NULL); 
        if(fds == -1)
        {
            perror("select err:");
            return -1;
        }
        //3.遍历select返回以后，在内核被标记的那个fd_set:modf
        for(int eventfd = 0; eventfd < maxfd + 1; eventfd++)
        {
            if(FD_ISSET(eventfd,&modify_fd_set))
            {
                if(eventfd == fd1)
                {
                    memset(buf,0,sizeof(buf));
                    nbyts = read(eventfd,buf,sizeof(buf)-1);
                    if(nbyts == -1)
                    {
                        perror("read err:");
                        return -1;
                    }
                    printf("读取keyboard设备,数据为=%s\n",buf);
                }

                if(eventfd == fd2)
                {
                    nbyts = read(eventfd,&mouseInfo,sizeof(mouseInfo));
                    if(nbyts == -1)
                    {
                        perror("read err:");
                        return -1;
                    }
                    printf("读取mouse设备,数据为=%d\n",mouseInfo.value);
                }
            }
        }
    }
    close(fd1);
    close(fd2);
    return 0;
}
