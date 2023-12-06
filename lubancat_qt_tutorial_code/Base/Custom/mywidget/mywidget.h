#ifndef MYWIDGET_H
#define MYWIDGET_H

#include "qtwidgetbase.h"

class MyWidget : public QtAnimationWidget
{
public:
    explicit MyWidget(QWidget *parent = 0);
    ~MyWidget();
private:
    void InitWidget();
protected:
    void paintEvent(QPaintEvent *);
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);
    void resizeEvent(QResizeEvent *e);

private:
    QtWidgetTitleBar    *m_widgetTitle;
};

#endif // MYWIDGET_H
