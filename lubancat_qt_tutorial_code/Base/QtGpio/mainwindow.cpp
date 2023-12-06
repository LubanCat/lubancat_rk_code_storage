#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /*设置使用的GPIO，初始化GPIO3_A5输出*/
    GPIO3_A5.chipname="gpiochip3";
    GPIO3_A5.line_num=5;
    GPIO3_A5.init();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_BtnOnOff_clicked(bool checked)
{
    if(checked)
    {
        /*按键控件按下on，checked=ture ，gpio输出低电平，led亮*/
        GPIO3_A5.set_value(!checked);
        ui->BtnOnOff->setStyleSheet(QString::fromUtf8("border-image: url(:/ic_btn_pre.png);"));
        ui->BtnRED->setStyleSheet(QString::fromUtf8("border-image: url(:/led_red.png);"));
    }
    else
    {
        /*按键控件off，checked=false ，gpio输出高电平，led灭*/
        GPIO3_A5.set_value(!checked);
        ui->BtnOnOff->setStyleSheet(QString::fromUtf8("border-image: url(:/ic_btn.png);"));
        ui->BtnRED->setStyleSheet(QString::fromUtf8("border-image: url(:/led_off.png);"));
    }
}

