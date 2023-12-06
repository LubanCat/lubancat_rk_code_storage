#include "mywidget.h"
//#include "skin.h"

#include <QPainter>
#include <QVBoxLayout>

MyWidget::MyWidget(QWidget *parent) : QtAnimationWidget(parent)
{
    // 初始化界面
    InitWidget();
}

MyWidget::~MyWidget()
{

}

void MyWidget::InitWidget()
{
    m_widgetTitle= new QtWidgetTitleBar(this);
    m_widgetTitle->SetScalSize(800, 80);
    m_widgetTitle->SetBackground(Qt::transparent);
    m_widgetTitle->SetBtnHomePixmap(QPixmap(":/images/backlight/menu_icon.png"), QPixmap(":/images/backlight/menu_icon_pressed.png"));
    m_widgetTitle->setFont(QFont("思源黑体 CN Bold"));
    m_widgetTitle->SetTitle(tr("自定义窗口"), "#ffffff", 32);
    connect(m_widgetTitle, SIGNAL(signalBackHome()), this, SIGNAL(signalBackHome()));

    QVBoxLayout *m_verLayoutAll = new QVBoxLayout(this);
    m_verLayoutAll->setContentsMargins(0, 0, 0, 0);
    m_verLayoutAll->setSpacing(0);
    m_verLayoutAll->addWidget(m_widgetTitle, 1);
    m_verLayoutAll->addStretch(5);
}

void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(m_scaleX, m_scaleY);
    painter.fillRect(QRect(0, 0, m_nBaseWidth, m_nBaseHeight), QColor("#000000"));

}

void MyWidget::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
}

void MyWidget::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);
}

void MyWidget::resizeEvent(QResizeEvent *e)
{
    SetScaleValue();
    QWidget::resizeEvent(e);
}

