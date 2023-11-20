#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDebug>
#include <QBitArray>
#include <QRegularExpression>
#include <QPlainTextEdit>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    static Widget *m_widget;
    static void logOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    static void write(QString str);

    void test_byteArray();
    void test_bitArray();
    void test_string();
    void test_stringList();

private:
    Ui::Widget *ui;

};
#endif // WIDGET_H
