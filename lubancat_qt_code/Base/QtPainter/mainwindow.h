#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPainterPath>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event);
    void myBrush(QPainter *qp);
    void myDrawPoints(QPainter *qp);
    void myDrawLines(QPainter *qp);
    void myDrawRect(QPainter *qp);
    void myDrawShape(QPainter *qp);
    void myDrawGradient(QPainter *qp);
    void myDrawText(QPainter *qp);
    void myDrawPath(QPainter *qp);

private slots:

private:
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
