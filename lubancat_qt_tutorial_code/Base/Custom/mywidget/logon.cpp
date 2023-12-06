/********************************************************
 *  @projectName CampusManageSystem
 *  @brief       摘要
 *  @author      贪贪贪丶慎独
 *  @date        2020-03-04
 *******************************************************/
#include "logon.h"
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QMouseEvent>
#include <QCheckBox>
#include <QStackedWidget>
#include <QComboBox>

Logon::Logon(QWidget *parent) :
    QDialog(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint);//设置窗口不显示最大化最小化关闭控件
    //this->setWindowTitle(tr("请登录"));
    this->setFixedSize(405, 435);//设置窗口大小 设置后窗口大小不可通过鼠标拖动改变大小
    this->setStyleSheet("background-color:white;border: 1px solid #C6C6C6");//设置窗口样式 背景为白色 边框1xp 颜色C6C6C6
    //
    loginMode=0;
    loginType=0;

    //登录窗口控件布局
    QLabel *windowsLab = new QLabel(this);
    windowsLab->setText(tr("账号登录"));
    windowsLab->setStyleSheet("background-color:#5292FE;padding-left:14px;color:white;font-size:18px;font-weight:bold;border:0px;");
    windowsLab->setGeometry(0, 0, 405, 40);
    QPushButton *closeBtn = new QPushButton(this);
    closeBtn->setGeometry(370, 5, 30, 30);
    closeBtn->setFlat(true);
    closeBtn->setStyleSheet("background-image: url(:/images/etc/login/close.png); border:0px");
    connect(closeBtn, SIGNAL(clicked()), SLOT(close()));

    signtypeLab = new QLabel(this);
    signtypeLab->setGeometry(200, 45, 135, 30);
    signtypeLab->setStyleSheet("background-image: url(:/images/etc/login/signtype1.png); border:0px");
    signiconTbtn = new QToolButton(this);
    signiconTbtn->setGeometry(340, 45, 60, 60);
    signiconTbtn->setStyleSheet("background-image: url(:/images/etc/login/signicon1.png); border:0px");
    connect(signiconTbtn, SIGNAL(clicked()), SLOT(setloginMode()));

    widgetMain = new QStackedWidget(this);
    widgetMain->setStyleSheet("border:0px");

    QWidget *widgetTrad= new QWidget();

    signidBtn = new QPushButton(widgetTrad);
    signidBtn->setStyleSheet("color:#288FE7;font-size:22px;font-weight:bold;border-bottom:3px solid #37AEFE;");
    signidBtn->setText(tr("账号登录"));
    signidBtn->setGeometry(0, 0, 142, 50);
    connect(signidBtn, SIGNAL(clicked()), SLOT(setidType()));
    signphoneBtn = new QPushButton(widgetTrad);
    signphoneBtn->setStyleSheet("color:#666666;font-size:22px;font-weight:bold;border-bottom:1px solid #E5E5E5;");
    signphoneBtn->setText(tr("短信登录"));
    signphoneBtn->setGeometry(143, 0, 142, 50);
    connect(signphoneBtn, SIGNAL(clicked()), SLOT(setphoneType()));

    userName=new QLineEdit(widgetTrad);
    userName->setStyleSheet("border:1px solid #E5E5E5; padding-left:10px; color:#CCCCCC;font-size:20px;");
    userName->setPlaceholderText(tr("账号/邮箱/电话"));
    userName->setGeometry(0, 70, 285, 40);
    passWord=new QLineEdit(widgetTrad);
    passWord->setStyleSheet("border:1px solid #E5E5E5; padding-left:10px; color:#CCCCCC;font-size:20px;");
    passWord->setGeometry(0, 120, 285, 40);
    passWord->setPlaceholderText(tr("请输入密码"));
    passWord->setEchoMode(QLineEdit::Password);
    getMessageBtn=new QPushButton(widgetTrad);
    getMessageBtn->setStyleSheet("border:1px solid #E5E5E5; color:#666666;font-size:13px;");
    getMessageBtn->setText("短信获取");
    getMessageBtn->setGeometry(175,120,110,40);
    getMessageBtn->hide();
    saveBox=new QCheckBox(widgetTrad);
    saveBox->setStyleSheet("color:#666666;font-size:13px;");
    saveBox->setText(tr("记住密码"));
    saveBox->setGeometry(0,170,80,20);
    autoBox=new QCheckBox(widgetTrad);
    autoBox->setStyleSheet("color:#666666;font-size:13px;");
    autoBox->setText(tr("自动登录"));
    autoBox->setGeometry(90,170,80,20);
    forgetpswdLab=new QLabel(widgetTrad);
    forgetpswdLab->setStyleSheet("color:#666666;font-size:13px;");
    forgetpswdLab->setText(tr("忘记密码"));
    forgetpswdLab->setGeometry(200,170,80,20);
    QPushButton *signinBtn=new QPushButton(widgetTrad);
    signinBtn->setStyleSheet("background-color:#169AF3; color:#FFFFFF;font-size:22px;border-radius:5;");
    signinBtn->setText("登 录");
    signinBtn->setGeometry(0, 210, 285, 40);
    connect(signinBtn, SIGNAL(clicked()), SLOT(trysignin()));
    QLabel *otherType=new QLabel(widgetTrad);
    otherType->setText(tr("其他登录方式"));
    otherType->setStyleSheet("color:#999999;font-size:13px;qproperty-alignment:'AlignCenter';");
    otherType->setGeometry(0, 270, 285, 15);
    QPushButton* signweiboBtn=new QPushButton(widgetTrad);
    signweiboBtn->setGeometry(110, 295, 25, 25);
    signweiboBtn->setStyleSheet("background-image: url(:/images/etc/login/weibo.png); border:0px");
    QPushButton* signrenrenBtn=new QPushButton(widgetTrad);
    signrenrenBtn->setGeometry(150, 295, 25, 25);
    signrenrenBtn->setStyleSheet("background-image: url(:/images/etc/login/renren.png); border:0px");

    QWidget *widgetScan= new QWidget();
    QLabel *infoLab=new QLabel(widgetScan);
    infoLab->setText(tr("手机扫码，安全登录"));
    infoLab->setStyleSheet("color:#333333;font-size:22px;qproperty-alignment:'AlignCenter';");
    infoLab->setGeometry(0, 0, 285, 50);
    QLabel *qrLab=new QLabel(widgetScan);
    qrLab->setStyleSheet("background-image: url(:/images/etc/login/QRcode.png);");
    qrLab->setGeometry(70, 60, 145, 145);
    QLabel *advLab=new QLabel(widgetScan);
    advLab->setText(tr("安全登录，扫我开始"));
    advLab->setStyleSheet("color:#999999;font-size:20px;qproperty-alignment:'AlignCenter';");
    advLab->setGeometry(0, 200, 285, 40);
    QCheckBox* auto2Btn=new QCheckBox(widgetScan);
    auto2Btn->setText(tr("自动登录"));
    auto2Btn->setGeometry(110, 245, 80, 25);
    auto2Btn->setStyleSheet("color:#999999;font-size:13px;");
    QToolButton *signqqTbtn=new QToolButton(widgetScan);
    signqqTbtn->setIcon(QIcon(":/images/etc/login/qq.png"));
    signqqTbtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    signqqTbtn->setText(tr("QQ登录"));
    signqqTbtn->setStyleSheet("color:#999999;font-size:18px;border:5px;");
    signqqTbtn->setGeometry(40, 295, 100, 30);
    QToolButton *signweixinTbtn=new QToolButton(widgetScan);
    signweixinTbtn->setIcon(QIcon(":/images/etc/login/weixin.png"));
    signweixinTbtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    signweixinTbtn->setText(tr("微信登录"));
    signweixinTbtn->setStyleSheet("color:#999999;font-size:18px;border:5px;");
    signweixinTbtn->setGeometry(150, 295, 100, 30);


    widgetMain->addWidget(widgetTrad);
    widgetMain->addWidget(widgetScan);
    widgetMain->setGeometry(60,80,285, 320);
    widgetMain->show();
}

Logon::~Logon()
{
}
//拖拽操作
void Logon::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        windowsDrag = true;
        //获得鼠标的初始位置
        mouseStartPoint = event->globalPos();
        //mouseStartPoint = event->pos();
        //获得窗口的初始位置
        windowTopLeftPoint = this->frameGeometry().topLeft();
    }
}

void Logon::mouseMoveEvent(QMouseEvent *event)
{
    if(windowsDrag)
    {
        //获得鼠标移动的距离
        QPoint distance = event->globalPos() - mouseStartPoint;
        //QPoint distance = event->pos() - mouseStartPoint;
        //改变窗口的位置
        this->move(windowTopLeftPoint + distance);
    }
}

void Logon::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        windowsDrag = false;
    }
}
void Logon::setloginMode()
{
    if(loginMode)
    {
        signtypeLab->setGeometry(200, 45, 135, 30);
        signtypeLab->setStyleSheet("background-image: url(:/images/etc/login/signtype1.png); border:0px");
        signiconTbtn->setStyleSheet("background-image: url(:/images/etc/login/signicon1.png); border:0px");

        widgetMain->setCurrentIndex(0);
        loginMode=0;
    }
    else
    {
        signtypeLab->setGeometry(215, 45, 120, 30);
        signtypeLab->setStyleSheet("background-image: url(:/images/etc/login/signtype2.png); border:0px");
        signiconTbtn->setStyleSheet("background-image: url(:/images/etc/login/signicon2.png); border:0px");

        widgetMain->setCurrentIndex(1);
        loginMode=1;
    }
}
void Logon::setidType()
{
    if(loginType!=0)
    {
        signidBtn->setStyleSheet("color:#288FE7;font-size:22px;font-weight:bold;border-bottom:3px solid #37AEFE;");
        signphoneBtn->setStyleSheet("color:#666666;font-size:22px;font-weight:bold;border-bottom:1px solid #E5E5E5;");

        userName->setPlaceholderText(tr("用户名/手机/学号"));
        passWord->setGeometry(0, 120, 285, 40);
        passWord->setPlaceholderText(tr("请输入密码"));
        getMessageBtn->hide();
        saveBox->show();
        autoBox->setGeometry(90,170,80,20);
        forgetpswdLab->show();

        loginType=0;
    }
}
void Logon::setphoneType()
{
    if(loginType!=1)
    {
        signphoneBtn->setStyleSheet("color:#288FE7;font-size:22px;font-weight:bold;border-bottom:3px solid #37AEFE;");
        signidBtn->setStyleSheet("color:#666666;font-size:22px;font-weight:bold;border-bottom:1px solid #E5E5E5;");

        userName->setPlaceholderText(tr("请输入手机号"));
        passWord->setGeometry(0, 120, 170, 40);
        passWord->setPlaceholderText(tr("请输入验证码"));
        getMessageBtn->show();
        saveBox->hide();
        autoBox->setGeometry(0,170,80,20);
        forgetpswdLab->hide();

        loginType=1;
    }
}
/**
* @describe 尝试登录 查询数据库
* @param    无
*
* @return   无
*/
void Logon::trysignin()
{
    this->accept();
}
/**
* @describe 退出登录
* @param    无
*
* @return   无
*/
void Logon::exit()
{
    this->reject();
}

