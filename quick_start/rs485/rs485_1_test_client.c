#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <unistd.h>
#include <termios.h>

#define SERVER_GPIO_INDEX   "90"  //流控引脚对应编号
const char default_path[] = "/dev/ttyS3";  //rs485对应的串口

/*
*串口配置成功返回0，失败返回-1；
*/
int set_uart(int fd,int nSpeed, int nBits, char nEvent, int nStop);

static int _server_ioctl_init(void)
{
   int fd;
   //index config
   fd = open("/sys/class/gpio/export", O_WRONLY);
   if(fd < 0)
         return 1;

   write(fd, SERVER_GPIO_INDEX, strlen(SERVER_GPIO_INDEX));
   close(fd);

   //direction config
   fd = open("/sys/class/gpio/gpio" SERVER_GPIO_INDEX "/direction", O_WRONLY);
   if(fd < 0)
         return 2;

   write(fd, "out", strlen("out"));
   close(fd);

   return 0;
}

static int _server_ioctl_on(void)
{
   int fd;

   fd = open("/sys/class/gpio/gpio" SERVER_GPIO_INDEX "/value", O_WRONLY);
   if(fd < 0){
        printf("_server_ioctl_open error\n");
         return 1;
   }
   write(fd, "1", 1);
   close(fd);
   return 0;
}

static int _server_ioctl_off(void)
{
   int fd;

   fd = open("/sys/class/gpio/gpio" SERVER_GPIO_INDEX "/value", O_WRONLY);
   if(fd < 0)
         return 1;

   write(fd, "0", 1);
   close(fd);
   return 0;
}

static void _modbus_rtu_server_ioctl(int on)
{
   if (on) {
         _server_ioctl_on();
   } else {
         _server_ioctl_off();
   }
}

static int _server_ioctl_exit(void)
{
    int fd;
    fd = open("/sys/class/gpio/uexport", O_WRONLY);
    if(fd < 0)
         return 1;

    write(fd, SERVER_GPIO_INDEX, strlen(SERVER_GPIO_INDEX));
    close(fd);

}

int main(int argc,char *argv[])
{
    int fd;
    int res;
    char *path;
    //char buf[1024] = "Embedfire 485 send test.\n";
    char buf[1024] = "1111111\n";
    char buf1[1024];
    int i;

    if(argc > 1)
    {
        path = argv[1];
    }
    else
    {
        path = (char *)default_path;
    }

    fd = open(path,O_RDWR);
    if(fd < 0)
    {
        perror(path);
        exit(-1);
    }

    if( set_uart(fd,115200,8,'n',1) )
    {
        printf("set uart error\n");
    }

    _server_ioctl_init();

    for(i=0;i<100;i++){
        //485-1发送
        _modbus_rtu_server_ioctl(1);
        write(fd, buf, strlen(buf));
        printf("485-1 Send data, res = %d bytes data: %s\r\n",strlen(buf), buf);
        usleep(10000); // 10毫秒
        //sleep(1);

        //485-1读取
        _modbus_rtu_server_ioctl(0);
        memset(buf1,0,1024);
        res = read(fd, buf1, 1024);
        if(res > 0)
        {
            printf("485-1 Read data, res = %d bytes data: %s\r\n",res, buf1);
        }

        printf("cycle index is %d\r\n",i);
        usleep(10000); // 10毫秒
        //sleep(1);
    }

    _server_ioctl_exit();
    close(fd);

    return 0;
}

int set_uart(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{

    struct termios opt;

    //清空串口接收缓冲区
    tcflush(fd, TCIOFLUSH);

    //获取串口配置参数
    tcgetattr(fd, &opt);

    opt.c_cflag &= (~CBAUD);    //清除数据位设置
    opt.c_cflag &= (~PARENB);   //清除校验位设置

    //opt.c_iflag |= IGNCR;       //忽略接收数据中的'\r'字符，在windows中换行为'\r\n'
    opt.c_iflag &= (~ICRNL);    //不将'\r'转换为'\n'

    opt.c_lflag &= (~ECHO);     //不使用回显

    //设置波特率
    switch(nSpeed)
    {
        case 2400:
            cfsetspeed(&opt,B2400);
            break;

        case 4800:
            cfsetspeed(&opt,B4800);
            break;

        case 9600:
            cfsetspeed(&opt,B9600);
            break;

        case 38400:
            cfsetspeed(&opt,B38400);
            break;

        case 115200:
            cfsetspeed(&opt,B115200);
            break;

        default:
            return -1;
    }

    //设置数据位
    switch(nBits)
    {
        case 7:
            opt.c_cflag |= CS7;
            break;

        case 8:
            opt.c_cflag |= CS8;
            break;

        default:
            return -1;
    }

    //设置校验位
    switch(nEvent)
    {
        //无奇偶校验
        case 'n':
        case 'N':
            opt.c_cflag &= (~PARENB);
            break;

        //奇校验
        case 'o':
        case 'O':
            opt.c_cflag |= PARODD;
            break;

        //偶校验
        case 'e':
        case 'E':
            opt.c_cflag |= PARENB;
            opt.c_cflag &= (~PARODD);
            break;

        default:
            return -1;
    }

    //设置停止位
    switch(nStop)
    {
        case 1:
            opt.c_cflag &= ~CSTOPB;
            break;
        case 2:
            opt.c_cflag |= CSTOPB;
            break;
        default:
            return -1;
    }
    //设置串口
    tcsetattr(fd,TCSANOW,&opt);

    return 0;
}