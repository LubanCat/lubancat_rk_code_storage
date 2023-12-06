#include "mainwindow.h"
#include "mywidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //MainWindow w;

    MyWidget w;

    w.resize(640, 480);
    w.show();
    return a.exec();
}
