#ifndef CUSTOMMESSAGE_H
#define CUSTOMMESSAGE_H

#include <QDialog>
#include <QFrame>

//MyFrame 为了实现圆角效果
class MyFrame : public QFrame {
    Q_OBJECT
public:
    explicit MyFrame(QWidget *parent = 0);
    ~MyFrame();
    void setText(QString title,QString message,QString accept,QString reject);

private:
    QString message;
    QString message_title;
    QString accept_text;
    QString reject_text;

    bool accept_hover;
    bool reject_hover;
protected:
    void paintEvent(QPaintEvent *);
};

class CustomMessage : public QDialog {
    Q_OBJECT

public:
    explicit CustomMessage(QWidget *parent = 0);
    ~CustomMessage();
    void setMessage(QString Message);

private slots:
    void acceptOperate();
    void rejectOperate();
private:
    MyFrame* frame;
protected:
    void mousePressEvent( QMouseEvent * event );
    void mouseReleaseEvent( QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *event);
};

#endif
