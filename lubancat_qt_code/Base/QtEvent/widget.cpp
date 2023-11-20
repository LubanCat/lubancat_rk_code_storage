#include "widget.h"
#include "ui_widget.h"

#include "myevent.h"
#include <QMouseEvent>
#include <QDateTime>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowTitle("Event Test");

    installEventFilter(this); //安装事件过滤

    timer = new QTimer();
    connect(timer,SIGNAL(timeout()), this, SLOT(handletimeout()));  //定时发送自定义事件
    timer->start(5000);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::handletimeout()
{
    myevent *mypostEvent = new myevent();
    //使用postevent方式发送，事件添加到事件队列，此事件分配给this来处理,,系统会自动释放的。
    QCoreApplication::postEvent(this, mypostEvent);
}


void Widget::mousePressEvent(QMouseEvent *event)
{
    QString mesg;

    if(event->button() == Qt::LeftButton)
        mesg=QString("鼠标左键:(%1,%2)").arg(event->x()).arg(event->y());
    else if(event->button() == Qt::RightButton)
        mesg=QString("鼠标右键:(%1,%2)").arg(event->x()).arg(event->y());
    else if(event->button() == Qt::MiddleButton)
        mesg=QString("鼠标滚轮:(%1,%2)").arg(event->x()).arg(event->y());

    ui->textEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(mesg));
}

void Widget::closeEvent(QCloseEvent *event)
{
    QString dlgTitle="question";
    QString str="关闭窗口";

    QString mesg=QString("关闭窗口事件");
    ui->textEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(mesg));

    int choose=QMessageBox::question(this, dlgTitle, str,QMessageBox::Yes,QMessageBox::No);

    //使用ignore和accept函数，不接受或接受事件
    switch (choose) {
    case QMessageBox::Yes:
        event->accept();
        break;
    case QMessageBox::No:
        event->ignore();
        break;
    default:
        event->accept();
        break;
    }
}

bool Widget::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Tab) {
            QString mesg=QString("tab按键:(eventype,%1)").arg(e->type());
            ui->textEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(mesg));
            return true;
        }
    }

    if (e->type() == myevent::myEventype())
    {
        QString mesg=QString("自定义事件");
        ui->textEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(mesg));
        return true;
    }

    return QWidget::event(e); //其他类型事件,执行父类的event()
}

bool Widget::eventFilter(QObject *object, QEvent *event)
{
    if (object == this && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            QString mesg=QString("空格按键");
            ui->textEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(mesg));
            return true;
        } else
            return false;
    }
    return QWidget::eventFilter(object,event);     //执行父类的eventFilter()函数
}



