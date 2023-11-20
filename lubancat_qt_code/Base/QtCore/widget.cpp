#include "widget.h"
#include "ui_widget.h"
#include    <QMetaProperty>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    stu1 = new Student("xiaoming", this);
    //设置属性值
    stu1->setProperty("score",95);
    stu1->setProperty("age",20);
    stu1->setProperty("sex","Boy");  //sex是动态属性

    //关联
    connect(stu1,SIGNAL(scoreChanged(int)),this,SLOT(do_scoreChange(int)));

    stu2 = new Student("xiaomei", this);
    //设置属性值
    stu2->setProperty("score",85);
    stu2->setProperty("age",21);
    stu2->setProperty("sex","Girl");  //sex是动态属性
    connect(stu2,SIGNAL(scoreChanged(int)),this,SLOT(do_scoreChange(int)));

    ui->spinStu1->setProperty("isStu1",true);        //isStu1动态属性
    ui->spinStu2->setProperty("isStu2",true);        //isStu2动态属性

    ui->spinStu1->setValue(stu1->property("score").toInt());  //通过property获取score属性值
    ui->spinStu2->setValue(stu2->property("score").toInt());  //通过property获取score属性值

    connect(ui->spinStu1,SIGNAL(valueChanged(int)), this,SLOT(do_spinChanged(int)));
    connect(ui->spinStu2,SIGNAL(valueChanged(int)), this,SLOT(do_spinChanged(int)));

    //connect(ui->spinStu2, &QSpinBox::valueChanged,  this,&Widget::do_spinChanged);
}

Widget::~Widget()
{
    delete ui;
    qDebug()<<"删除 widget";
}

void Widget::do_scoreChange(int value)
{
    Q_UNUSED(value);
    Student *stu = qobject_cast<Student *>(sender());   //获取信号发射者

    QString Name=stu->property("name").toString();   //获取属性 姓名
    QString Sex=stu->property("sex").toString();     //获取动态属性
    int Age=stu->property("age").toInt();            //通过属性获得 年龄
    QString Score=stu->property("score").toString();   //姓名
    QString str=QString("name=%1, sex=%2, age=%3, score=%4").arg(Name).arg(Sex).arg(Age).arg(Score);
    ui->textEdit->append(str);
}

void Widget::do_spinChanged(int value)
{
    QSpinBox *spinBox = qobject_cast<QSpinBox *>(sender());   //获取信号发射者
    if (spinBox->property("isStu1").toBool())                 //根据动态属性判断是哪个spinBox
    {
       stu1->setScore(value);
       ui->textEdit->append("spinStu1 改变");
    }
    else
    {
        stu2->setScore(value);
        ui->textEdit->append("spinStu2 改变");
    }
}

void Widget::on_btnObjectInfo_clicked()
{
    QObject *obj=stu1;    //这里是显示stu1的信息
    const QMetaObject *meta=obj->metaObject();  //获得元对象
    ui->textEdit->clear();                 //清理
    ui->textEdit->append(QString("类名称：%1\n").arg(meta->className()));

    ui->textEdit->append("属性信息:");
    for (int i=meta->propertyOffset();i<meta->propertyCount();i++)
    {
        const char* propName=meta->property(i).name();
        QString propValue=obj->property(propName).toString();
        QString str=QString("Name=%1，Value=%2").arg(propName).arg(propValue);
        ui->textEdit->append(str);
    }

    //获取附加的类信息
    ui->textEdit->append("");
    ui->textEdit->append("附加的类信息:");
    for (int i=meta->classInfoOffset();i<meta->classInfoCount();++i)
    {
        QMetaClassInfo classInfo=meta->classInfo(i);
        ui->textEdit->append(
        QString("Name=%1; Value=%2").arg(classInfo.name()).arg(classInfo.value()));
    }
}


void Widget::on_btnClear_clicked()
{
    ui->textEdit->clear();
}


void Widget::on_stu1Add_clicked()
{
    stu1->addScore();
}


void Widget::on_stu2Add_clicked()
{
    stu2->addScore();
}


void Widget::on_stu1Sub_clicked()
{
    stu1->subScore();
}


void Widget::on_stu2Sub_clicked()
{
    stu1->subScore();
}

