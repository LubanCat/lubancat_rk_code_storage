#include "student.h"

Student::Student(QString name , QObject *parent) : QObject{parent}
{
    m_name = name;
}

Student::~Student()
{
    qDebug("Student对象被删除");
}

int Student::getScore()
{
    return  m_score;
}

void Student::setScore(int value)
{
    if (m_score != value)
    {
        m_score= value;
        emit scoreChanged(m_score);  //发射信号
    }
}

void Student::subScore()
{
    if(m_score > 0)
    {
        m_score--;
        emit scoreChanged(m_score);  //发射信号
    }
}

void Student::addScore()
{
    if(m_score < 100)
    {
        m_score++;
        emit scoreChanged(m_score);  //发射信号
    }
}
