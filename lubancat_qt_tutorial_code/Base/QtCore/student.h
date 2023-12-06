#ifndef STUDENT_H
#define STUDENT_H

#include <QObject>

class Student : public QObject
{
    Q_OBJECT

    //定义附加的类信息
    Q_CLASSINFO("author", "Embedfire")
    Q_CLASSINFO("url", "https://embedfire.com/")

    //定义属性
    Q_PROPERTY(int score READ getScore WRITE setScore NOTIFY scoreChanged)     //定义属性score
    Q_PROPERTY(QString name MEMBER m_name)      //定义属性name
    Q_PROPERTY(int age MEMBER m_age)        //定义属性age

public:
    explicit Student(QString name, QObject *parent);
    ~Student();                                //析构函数
    int     getScore();
    void    setScore(int value);
    void    subScore();
    void    addScore();

private:
    int  m_age=18;
    int  m_score=60;
    QString m_name;

signals:
    void    scoreChanged(int  value);        //自定义信号

};

#endif // STUDENT_H
