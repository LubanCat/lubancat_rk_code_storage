#include "myevent.h"

QEvent::Type myevent::myType = QEvent::None;

myevent::myevent():QEvent(myEventype())
{
}

QEvent::Type myevent::myEventype()
{
    if (myType == QEvent::None)
        myType = (QEvent::Type)QEvent::registerEventType();  //注册事件

    return myType;
}
