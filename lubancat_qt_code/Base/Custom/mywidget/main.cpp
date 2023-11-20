/******************************************************************
 Copyright (C) 2019 - All Rights Reserved by
 文 件 名 : main.cpp ---
 作 者    : Niyh(lynnhua)
 编写日期 : 2019
 说 明    :
 历史纪录 :
 <作者>    <日期>        <版本>        <内容>
  Niyh	   2019    	1.0.0 1     文件创建
*******************************************************************/
#include "mainwindow.h"
//#include "splashscreen.h"
//#include "skin.h"

#include <QApplication>
#include <QTextCodec>
#include <QDesktopWidget>
#include <QScreen>
#include <QDir>
#include <QTranslator>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.setWindowTitle(QStringLiteral("野火@Linux Qt Demo"));
    w.resize(800, 480);
    w.show();
    return a.exec();
}
