#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QGridLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>

#include "uart.h"

class Widget : public QWidget
{
    Q_OBJECT

public:
    Uart uart3;            //串口
    void iniUI();          //初始化窗口

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    QTextEdit *textEdit;   //用于显示信息
    QLineEdit *lineEdit;   //用于输入
    QPushButton *BtnSend;  //发送按钮

private slots:
    void do_send();        //发送
    void do_recvData();    //接收串口信息
};
#endif // WIDGET_H
