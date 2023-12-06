#ifndef UART_H
#define UART_H

extern "C"{
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
}

#include <QObject>

class Uart : public QObject
{
    Q_OBJECT

signals:

public:
    QByteArray _recvData;
    void uart_open(QString dev = QString("/dev/ttyS3"), speed_t buad = B115200);
    void uart_sendData(QByteArray data);
    void uart_close();
    int uart_read();

public:
    explicit Uart(QObject *parent = nullptr);

private:
    int _fd;

signals:
    void recvData(QByteArray);

};

#endif // UART_H
