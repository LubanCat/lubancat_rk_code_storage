#include "widget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 这里debug输出到窗口，也可以注释下面w窗口程序，复制测试次序到这，直接输出到终端
    Widget w;
    w.show();

    return a.exec();
}
