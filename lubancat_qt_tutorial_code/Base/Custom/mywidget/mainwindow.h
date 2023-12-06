/******************************************************************
 Copyright (C) 2019 - All Rights Reserved by
 文 件 名 : mainwindow.h ---
 作 者    : Niyh(lynnhua)
 编写日期 : 2019
 说 明    :
 历史纪录 :
 <作者>    <日期>        <版本>        <内容>
  Niyh	   2019    	1.0.0 1     文件创建
*******************************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include "launcherwidget.h"
#include "qtwidgetbase.h"
#include "threadmousecheck.h"


class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    LauncherWidget      *m_launcherWidget;
    QMap<int, LauncherItem*> m_launchItems;
    QtAnimationWidget   *m_widgetWorkSpace;

    int                  m_nCurrentIndex;
    bool                 m_bStartApp;

    ThreadMouseCheck    *m_threadUsbInsert;

#ifdef CHECK_MOUSE_BY_TIMER
    int              m_nMouseCheckId;
#endif
private:
    void InitWidget();
    void InitDesktop();
    void InitThreads();

    bool CheckDevice(QString device);
    QString CheckMPU6050Device();
private slots:
    void SltCurrentAppChanged(int index);

    void SltBackHome();
    void SltChangeCursorShap(Qt::CursorShape shape);
    void SltAppStartOk();

    void SltMouseInsert(bool bOk);

protected:
    void resizeEvent(QResizeEvent *e);
#ifdef BUILD_WIN_NES
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
#endif

    // 新增鼠标检测
    bool CheckMouseInsert();

#if CHECK_MOUSE_BY_TIMER
    void timerEvent(QTimerEvent *e);
#endif
};

#endif // MAINWINDOW_H
