#include "widget.h"

void Widget::iniUI()
{
    //创建文本框，只读
    textEdit = new QTextEdit;
    textEdit->setReadOnly(1);

    //
    lineEdit = new QLineEdit;
    BtnSend = new QPushButton("发送");

    QHBoxLayout *HLay =new QHBoxLayout;
    HLay->addWidget(lineEdit);
    HLay->addWidget(BtnSend);

    //创建垂直布局，并设置为主布局
    QVBoxLayout *VLay=new QVBoxLayout(this);
    VLay->addWidget(textEdit);         //添加textEdit
    VLay->addSpacing(5);               //添加空隙
    VLay->addLayout(HLay);             //添加HLay

    setLayout(VLay);    //设置为窗口的主布局
}

void Widget::do_recvData()
{
    if(uart3.uart_read() > 0)
    {
        //textEdit->append("pc: "+QString(uart3._recvData));
        textEdit->append("pc: "+uart3._recvData);
    }
}

void Widget::do_send()
{
    if(!lineEdit->text().isEmpty())
    {
        uart3.uart_sendData(lineEdit->text().toUtf8()+"\n");
        textEdit->append("cat: "+lineEdit->text().toUtf8());
        lineEdit->clear();
    }
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    setGeometry(0,0,640,480);
    iniUI();                      //窗口创建与初始化
    setWindowTitle("串口测试");    //设置窗口标题
    uart3.uart_open();            //连接串口

    QTimer* timerRead = new QTimer;
    timerRead->start(500);

    //信号与槽的关联
    connect(timerRead, SIGNAL(timeout()),this, SLOT(do_recvData()));
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(do_send()));
    connect(BtnSend,SIGNAL(clicked()),this,SLOT(do_send()));
}

Widget::~Widget()
{
    uart3.uart_close();
}

