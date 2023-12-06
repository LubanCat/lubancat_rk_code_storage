#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>

class MyRunThread : public QThread
{
    Q_OBJECT
public:
    explicit MyRunThread();

protected:
    void run();               //线程的事件循环

signals:
    void value(int);
};


class GenerateNumber : public QObject
{
    Q_OBJECT

private slots:
    void slot_MyThread();

signals:
    void get_value(int);

};

#endif // MYTHREAD_H
