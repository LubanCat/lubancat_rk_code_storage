#include "mywidget.h"

#include <QLabel>
#include <QBoxLayout>

MyWidget::MyWidget(QWidget *parent) : QWidget(parent)
{
    // 创建一个 QLabel 控件，并设置文本为 "hello widget"
    QLabel *label =new QLabel("hello widget");
    // 设置 QLabel 对齐方式为居中对齐
    label->setAlignment(Qt::AlignCenter);

    // 创建一个 QVBoxLayout 布局，并将其设置为 MyWidget 的布局
    QVBoxLayout *verLayout = new QVBoxLayout(this);
    // 设置布局的边距为0
    verLayout->setContentsMargins(0, 0, 0, 0);
    // 设置布局的间距为0，以避免控件之间的间隔
    verLayout->setSpacing(0);

    // 将 QLabel 控件添加到垂直布局中
    verLayout->addWidget(label);
}
