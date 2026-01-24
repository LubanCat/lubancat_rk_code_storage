#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>

//第一部分代码/
//根据具体的设备修改
const char default_path[] = "/dev/ttyS3";

int main(int argc, char *argv[])
{
    int fd;
    int res;
    char *path;
    char buf[1024] = "Embedfire tty send test.\n";

    //第二部分代码/
    //若无输入参数则使用默认终端设备
    if (argc > 1)
        path = argv[1];
    else
        path = (char *)default_path;

    //获取串口设备描述符
    printf("This is tty/usart demo (Raw Mode).\n");
    fd = open(path, O_RDWR | O_NOCTTY);  // O_NOCTTY：不将串口设为控制终端
    if (fd < 0) {
        printf("Fail to Open %s device\n", path);
        return -1;
    }


    //第三部分代码/
    struct termios opt;
    //清空串口接收缓冲区
    tcflush(fd, TCIOFLUSH);
    //获取串口参数opt
    if (tcgetattr(fd, &opt) != 0) {
        printf("Failed to get serial attributes\n");
        close(fd);
        return -1;
    }

    // -------------------------- 原始模式核心配置 --------------------------
    // 关闭规范模式，取消行缓冲、回显、信号处理
    // ICANON：关闭规范模式；ECHO/ECHOE：关闭回显；ISIG：关闭信号,如Ctrl+C
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // 关闭软件流控、回车/换行转换等输入处理
    // IXON/IXOFF/IXANY：关闭软件流控；ICRNL/INLCR/IGNCR：取消回车<->换行转换
    opt.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);
    
    // 关闭输出处理，原始输出，不修改任何字符
    opt.c_oflag &= ~OPOST;
    
    // 设置读取规则：VMIN=1，至少读1个字符才返回，VTIME=0，无超时，阻塞等待
    opt.c_cc[VMIN] = 1;   // 最小读取字符数：有1个字符就返回
    opt.c_cc[VTIME] = 0;  // 超时时间：0表示无限阻塞，直到有数据
    // ---------------------------------------------------------------------

    //设置串口输出波特率
    cfsetospeed(&opt, B9600);
    //设置串口输入波特率
    cfsetispeed(&opt, B9600);
    
    // 设置数据位数：8位
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    // 校验位：无校验
    opt.c_cflag &= ~PARENB;
    opt.c_iflag &= ~INPCK;
    // 停止位：1位
    opt.c_cflag &= ~CSTOPB;
    // 启用接收器，忽略调制解调器状态线
    opt.c_cflag |= CREAD | CLOCAL;

    // 更新配置，TCSANOW：立即生效
    if (tcsetattr(fd, TCSANOW, &opt) != 0) {
        printf("Failed to set serial attributes\n");
        close(fd);
        return -1;
    }
    printf("Device %s is set to 9600bps,8N1 (Raw Mode)\n", path);

    //第四部分代码/
    do {
        // 发送字符串
        write(fd, buf, strlen(buf));
        // 接收字符串，原始模式下，有1个字符就会返回
        res = read(fd, buf, sizeof(buf)-1);  // 留1位给结束符，避免越界
        if (res > 0) {
            // 给接收到的字符串加结束符
            buf[res] = '\0';
            printf("Receive res = %d bytes data: %s\n", res, buf);
        } else if (res < 0) {
            perror("Read error");
            break;
        }
        usleep(100000);
    } while (res >= 0);

    printf("Exit, res = %d\n", res);
    close(fd);
    return 0;
}