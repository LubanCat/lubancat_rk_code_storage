#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    thread1 = new MyRunThread();
    connect(thread1, SIGNAL(value(int)), this, SLOT(thread1Value(int)));

    thread2 = new GenerateNumber();
    thread2->moveToThread(&workerThread);
    connect(thread2, SIGNAL(get_value(int)), this, SLOT(thread2Value(int)));
    connect(ui->btn_thread2, SIGNAL(clicked()), thread2, SLOT(slot_MyThread()));
    connect(&workerThread, &QThread::finished, thread2, &QObject::deleteLater);

    //打印UI界面主线程id
    qDebug()<<"Main thread: "<<QThread::currentThread()<<" id :"<<QThread::currentThreadId();;
}

MainWindow::~MainWindow()
{
    workerThread.quit();
    workerThread.wait();
    delete ui;
}

void MainWindow::thread1Value(int val)
{
    ui->lineEdit_thread1->setText(QString::number(val));
}

void MainWindow::thread2Value(int val)
{
    ui->lineEdit_thread2->setText(QString::number(val));
}


void MainWindow::on_btn_thread1_clicked()
{
    thread1->start();
}


void MainWindow::on_btn_thread2_clicked()
{
    workerThread.start();
}


void MainWindow::on_btn_MyThread_clicked()
{
    //线程执行，使用lambda函数
    myThread=QThread::create([=]()
    {
        for (int i=0;i<10 ;i++ )
        {
            qDebug()<<" myThread : "<<QThread::currentThread()<<" id :"<<QThread::currentThreadId();
        }
    });

    connect(myThread,&QThread::started,[=]()
    {
        qDebug()<<" myThread  started.";
    });

    connect(myThread,&QThread::finished,[=]()
    {
        qDebug()<<" myThread  finished.";
    });

    // 删除
    connect(myThread,&QThread::finished,myThread,&QThread::deleteLater);

    // 启动线程
    myThread->start();
}

