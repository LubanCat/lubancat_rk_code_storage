#ifndef Q_H
#define Q_H

#include <QObject>

class q : public QObject
{
    Q_OBJECT
public:
    explicit q(QObject *parent = nullptr);

signals:

};

#endif // Q_H
