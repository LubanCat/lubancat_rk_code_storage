#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    void mousePressEvent(QMouseEvent *event);    //重写鼠标按下事件
    void closeEvent(QCloseEvent *event);         //重写窗口关闭事件

//    void mouseMoveEvent(QMouseEvent *event);
//    void mouseReleaseEvent(QMouseEvent *event);
//    void mouseDoubleClickEvent(QMouseEvent *event);
//    void wheelEvent(QWheelEvent *event);
//    void keyPressEvent(QKeyEvent *event);
//    void resizeEvent(QResizeEvent *event);
//    void paintEvent(QPaintEvent *event);

    bool event(QEvent *e);                     //重新实现event()函数
    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void handletimeout();

private:
    Ui::Widget *ui;
    QTimer *timer;
};
#endif // WIDGET_H
