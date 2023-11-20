#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDebug>
#include <QBitArray>
#include <QRegularExpression>
#include <QPlainTextEdit>
#include <QDateTime>
#include <QStack>
#include <QQueue>
#include <QSet>

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

    void test_qvector();
    void test_qlist();
    void test_queue();
    void test_qstack();
    void test_qset();
    void test_qmap();
    void test_qmultimap();
    void test_hash();
    void test_qmultihash();
    void test_iteratorJava();
    void test_iteratorStl();
    void test_foreach();

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
