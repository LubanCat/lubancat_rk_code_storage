/******************************************************************
 Copyright (C) 2019 - All Rights Reserved by
 文 件 名 : mainwindow.cpp ---
 作 者    : Niyh(lynnhua)
 编写日期 : 2019
 说 明    :
 历史纪录 :
 <作者>    <日期>        <版本>        <内容>
  Niyh	   2019    	1.0.0 1     文件创建
*******************************************************************/
#include "mainwindow.h"
#include "statusbarwidget.h"
//#include "skin.h"
#include "mywidget.h"
#include "logon.h"

#include <QPainter>
#include <QBoxLayout>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QTimerEvent>
#include <QRegExp>

#define MOUSE_DEV_PATH       "/dev/input/by-path"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
#ifdef __arm__
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
#endif

    m_widgetWorkSpace = NULL;
    m_nCurrentIndex = -1;
    m_bStartApp = false;

    InitWidget();
    InitDesktop();
    InitThreads();
}

MainWindow::~MainWindow()
{
#ifdef CHECK_MOUSE_BY_TIMER
    killTimer(m_nMouseCheckId);
#endif
    if (NULL != m_widgetWorkSpace) {
        delete m_widgetWorkSpace;
        m_widgetWorkSpace = NULL;
    }
}

void MainWindow::InitWidget() {
    QVBoxLayout *verLayout = new QVBoxLayout(this);
    verLayout->setContentsMargins(0, 0, 0, 0);
    verLayout->setSpacing(0);

    m_launcherWidget = new LauncherWidget(this);
    m_launcherWidget->SetWallpaper(QPixmap(":/images/mainwindow/background.png"));
    connect(m_launcherWidget, SIGNAL(currentItemClicked(int)), this, SLOT(SltCurrentAppChanged(int)));
    verLayout->addWidget(m_launcherWidget, 1);
}

void MainWindow::InitDesktop()
{
    // 第一页
    int nPage = 0;
    m_launchItems.insert(0, new LauncherItem(0, nPage, tr("文件管理"), QPixmap(":/images/mainwindow/ic_file.png")));
    m_launchItems.insert(3, new LauncherItem(3, nPage, tr("相册"), QPixmap(":/images/mainwindow/ic_photos.png")));
    m_launchItems.insert(5, new LauncherItem(5, nPage, tr("天气"), QPixmap(":/images/mainwindow/ic_weather.png")));
    m_launchItems.insert(6, new LauncherItem(6, nPage, tr("记事本"), QPixmap(":/images/mainwindow/ic_notepad.png")));
    m_launchItems.insert(7, new LauncherItem(7, nPage, tr("时钟"), QPixmap(":/images/mainwindow/ic_clock.png")));
    m_launchItems.insert(8, new LauncherItem(8, nPage, tr("电子书"), QPixmap(":/images/mainwindow/ic_ebook.png")));
    m_launchItems.insert(10, new LauncherItem(10, nPage, tr("计算器"), QPixmap(":/images/mainwindow/ic_calc.png")));

    m_launchItems.insert(1, new LauncherItem(1, nPage, tr("视频播放"), QPixmap(":/images/mainwindow/ic_video.png")));
    m_launchItems.insert(2, new LauncherItem(2, nPage, tr("ADC"), QPixmap(":/images/mainwindow/ic_adc.png")));
    m_launchItems.insert(4, new LauncherItem(4, nPage, tr("相机"), QPixmap(":/images/mainwindow/ic_camera.png")));
    m_launchItems.insert(9, new LauncherItem(9, nPage, tr("温湿度"), QPixmap(":/images/mainwindow/ic_temp.png")));
    m_launchItems.insert(11, new LauncherItem(11, nPage, tr("音乐播放"), QPixmap(":/images/mainwindow/ic_music.png")));

    // 第二页
    nPage++;
    m_launchItems.insert(12, new LauncherItem(12, nPage, tr("RGB彩灯"), QPixmap(":/images/mainwindow/ic_light.png")));
    m_launchItems.insert(14, new LauncherItem(14, nPage, tr("网络浏览器"), QPixmap(":/images/mainwindow/ic_webview.png")));

    m_launchItems.insert(15, new LauncherItem(15, nPage, tr("汽车仪表"), QPixmap(":/images/mainwindow/ic_car.png")));
    m_launchItems.insert(16, new LauncherItem(16, nPage, tr("背光调节"), QPixmap(":/images/mainwindow/ic_backlight.png")));
    m_launchItems.insert(19, new LauncherItem(19, nPage, tr("按键测试"), QPixmap(":/images/mainwindow/ic_key.png")));
    m_launchItems.insert(23, new LauncherItem(23, nPage, tr("系统设置"), QPixmap(":/images/mainwindow/ic_setting.png")));

    m_launchItems.insert(13, new LauncherItem(13, nPage, tr("陀螺仪"), QPixmap(":/images/mainwindow/ic_gyroscope.png")));
    m_launchItems.insert(17, new LauncherItem(17, nPage, tr("蜂鸣器"), QPixmap(":/images/mainwindow/ic_beep.png")));
    m_launchItems.insert(18, new LauncherItem(18, nPage, tr("录音"), QPixmap(":/images/mainwindow/ic_record.png")));

    // 第三页
    nPage++;
    m_launchItems.insert(24, new LauncherItem(24, nPage, tr("InfoNES模拟器"), QPixmap(":/images/mainwindow/ic_game.png")));

    nPage++;
    // 第四页
    m_launchItems.insert(25, new LauncherItem(25, nPage, tr("登录对话框"), QPixmap(":/images/mainwindow/ic_webview.png")));


    m_launcherWidget->SetPageCount(nPage+1);
    m_launcherWidget->SetItems(m_launchItems);
}

void MainWindow::InitThreads()
{
    m_threadUsbInsert = new ThreadMouseCheck(this);
    connect(m_threadUsbInsert, SIGNAL(signalMouseInsert(bool)), this, SLOT(SltMouseInsert(bool)));
#ifdef __arm__
    m_threadUsbInsert->start();
#endif

}

void MainWindow::SltCurrentAppChanged(int index)
{
    if(index==25)
    {
        Logon *logon= new Logon(this);
//        logon->exec();
        logon->open();

//        delete logon;
        return;
    }

    if (m_bStartApp) return;

    m_launcherWidget->setEnabled(false);
    m_bStartApp = true;

    if (NULL != m_widgetWorkSpace ) {
        if (m_nCurrentIndex != index) {
            disconnect(m_widgetWorkSpace, SIGNAL(signalBackHome()), this, SLOT(SltBackHome()));
            delete m_widgetWorkSpace;
            m_widgetWorkSpace = NULL;
        } else {
            m_widgetWorkSpace->setVisible(true);
            m_widgetWorkSpace->StartAnimation(QPoint(this->width(), this->height()), QPoint(0, 0), 300, true);
            return;
        }
    }

    m_widgetWorkSpace = new MyWidget(this);

    if (NULL != m_widgetWorkSpace) {
        m_widgetWorkSpace->resize(this->size());
        connect(m_widgetWorkSpace, SIGNAL(signalBackHome()), this, SLOT(SltBackHome()));
        connect(m_widgetWorkSpace, SIGNAL(signalAnimationFinished()), this, SLOT(SltAppStartOk()));

        m_nCurrentIndex = index;
        m_widgetWorkSpace->setVisible(true);
        m_widgetWorkSpace->StartAnimation(QPoint(this->width(), this->height()), QPoint(0, 0), 300, true);
    }
}

void MainWindow::SltBackHome()
{
    if (NULL != m_widgetWorkSpace) {
        m_widgetWorkSpace->StartAnimation(QPoint(0, 0), QPoint(-this->width(), -this->height()), 300, false);
    }
}

void MainWindow::SltChangeCursorShap(Qt::CursorShape shape)
{
    this->setCursor(shape);
}

void MainWindow::SltAppStartOk()
{
    m_bStartApp = false;
    m_launcherWidget->setEnabled(true);
}

void MainWindow::SltMouseInsert(bool bOk)
{
}

void MainWindow::resizeEvent(QResizeEvent *e)
{

    if (NULL != m_widgetWorkSpace) {
        m_widgetWorkSpace->resize(this->size());
    }

    QWidget::resizeEvent(e);
}

bool MainWindow::CheckMouseInsert()
{
    QDir dir(MOUSE_DEV_PATH);
    if (!dir.exists()) return false;

    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); i++) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.fileName().contains("mouse")) {
            return true;
        }
    }

    return false;
}
