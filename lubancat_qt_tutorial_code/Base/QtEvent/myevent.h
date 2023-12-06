#ifndef MYEVENT_H
#define MYEVENT_H

#include <QEvent>

class myevent : public QEvent
{
public:
    myevent();

    static Type myEventype();
private:
    static Type myType;
};

#endif // MYEVENT_H
