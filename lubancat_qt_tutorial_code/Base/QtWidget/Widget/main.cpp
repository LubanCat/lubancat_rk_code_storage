#include "mainwindow.h"
#include <QApplication>
//#include "skin.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 初始化皮肤文件
//    Skin::InitSkin();

    MainWindow w;
    w.resize(800, 480);
    w.show();

    return a.exec();
}
