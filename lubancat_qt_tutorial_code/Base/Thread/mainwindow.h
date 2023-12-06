#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mythread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QThread workerThread;
    MyRunThread * thread1;
    GenerateNumber * thread2;

private:
    Ui::MainWindow *ui;
    QThread * myThread;

public slots:
    void thread1Value(int val);
    void thread2Value(int val);
private slots:
    void on_btn_thread1_clicked();
    void on_btn_thread2_clicked();
    void on_btn_MyThread_clicked();
};
#endif // MAINWINDOW_H
