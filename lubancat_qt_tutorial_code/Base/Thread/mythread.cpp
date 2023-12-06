#include "mythread.h"
#include    <QRandomGenerator>

MyRunThread::MyRunThread()
{

}

void MyRunThread::run()
{
    int m_value = QRandomGenerator::global()->bounded(1, 100);  //产生一个随机整数，在[1,100]
    msleep(100);          //线程休眠100ms
    emit value(m_value);
    quit();                // 退出线程的事件循环
}


void GenerateNumber::slot_MyThread()
{
    int m_value = QRandomGenerator::global()->bounded(1, 100);  //产生一个随机整数，在[1,100]
    emit get_value(m_value);
}



