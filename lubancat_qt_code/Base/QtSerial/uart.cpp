#include "uart.h"

Uart::Uart(QObject *parent)
    : QObject{parent}
{
    _fd=0;
}

void Uart::uart_open(QString dev, speed_t buad)
{
    struct termios options;

    if ((_fd = open(dev.toUtf8().data(), O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) == -1)
    {
        qDebug("open failed!");
        return;
    }

    // 读写
    fcntl (_fd, F_SETFL, O_RDWR) ;

    // 设置波特率，串口的其他参数设置参考下https://doc.embedfire.com/linux/rk356x/linux_base/zh/latest/linux_app/uart/uart.html#id12
    tcgetattr (_fd, &options);
    cfmakeraw   (&options) ;
    cfsetispeed (&options, buad) ;
    cfsetospeed (&options, buad) ;
    tcsetattr (_fd, TCSANOW, &options) ;
}

void Uart::uart_sendData(QByteArray data)
{
    write (_fd, data.data(), strlen(data));
}

void Uart::uart_close()
{
    if(_fd > 0)
    {
        close (_fd);
        _fd = 0;
    }
}

int Uart::uart_read()
{
    int size;
    uint8_t x;

    // 可读取的字节数
    if (ioctl (_fd, FIONREAD, &size) == -1)
        return -1;

    _recvData.clear();

    // 读取串口
    for(int index = 0; index < size; index++)
    {
        if (read (_fd, &x, 1) != 1)
            return -1 ;
        _recvData.append((uchar)((int)x) & 0xFF);
    }

    return size;
}
