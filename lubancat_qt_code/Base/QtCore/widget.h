#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "student.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Student *stu1;
    Student *stu2;
    Ui::Widget *ui;

private slots:
//自定义槽函数
    void   do_scoreChange(int  value);
    void   do_spinChanged(int value);

//按钮的槽函数
    void on_btnObjectInfo_clicked();
    void on_btnClear_clicked();
    void on_stu1Add_clicked();
    void on_stu2Add_clicked();
    void on_stu1Sub_clicked();
    void on_stu2Sub_clicked();
};
#endif // WIDGET_H
