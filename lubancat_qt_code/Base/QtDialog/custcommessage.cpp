#include "custcommessage.h"
#include <QPainter>
#include <QMouseEvent>

MyFrame::MyFrame(QWidget *parent) : QFrame(parent)
{
    this->resize(320, 160);

    message_title="提示";
    accept_text="确认";
    reject_text="取消";
}

MyFrame::~MyFrame()
{

}

void MyFrame::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRect title_rect(0,0,this->width(),this->height()/6);
    QRect message_rect(0,this->height()/6,this->width(),this->height()/2);
    QRect accept_rect(0,this->height()/3*2,this->width()/2,this->height()/3);
    QRect reject_rect(this->width()/2,this->height()/3*2,this->width()/2-1,this->height()/3);

    //绘制文字
    QFont font = painter.font();
    font.setPixelSize(16);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(accept_rect,Qt::AlignCenter,accept_text);
    painter.drawText(reject_rect,Qt::AlignCenter,reject_text);
    painter.drawText(title_rect,Qt::AlignCenter,message_title);

    font.setPixelSize(14);
    painter.setFont(font);
    painter.drawText(message_rect,Qt::AlignCenter,message);

    //绘制线条
    painter.drawLine(0,this->height()/3*2,this->width(),this->height()/3*2);
    painter.drawLine(this->width()/2,this->height()/3*2,this->width()/2,this->height());
}

void MyFrame::setText(QString title,QString message,QString accept,QString reject)
{
    this->message_title=title;
    this->message=message;
    this->accept_text=accept;
    this->reject_text=reject;
}


CustomMessage::CustomMessage(QWidget *parent) : QDialog(parent)
{
    this->setModal(true);

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->resize(320, 160);

    frame = new MyFrame(this);
    frame->setStyleSheet("background-color:#f2f2f2;border-radius:5px;");
    frame->show();
}

CustomMessage::~CustomMessage()
{

}

void CustomMessage::setMessage(QString Message)
{
    frame->setText("提示",Message,"确认","取消");
}

void CustomMessage::mouseReleaseEvent( QMouseEvent *)
{

}

void CustomMessage::mousePressEvent( QMouseEvent * event )
{
    if(event->pos().y()>this->height()/3*2)
    {
        if(event->pos().x()>this->width()/2)
        {
            rejectOperate();
        }
        else
        {
            acceptOperate();
        }
    }
    this->update();
}

void CustomMessage::mouseMoveEvent(QMouseEvent *event)
{
}

//确认操作
void CustomMessage::acceptOperate()
{
    this->accept();
}

//取消操作
void CustomMessage::rejectOperate()
{
    this->reject();
}

