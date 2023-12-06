#ifndef GPIOLED_H
#define GPIOLED_H

#include <QDebug>

extern "C"{
#include <gpiod.h>
}

class GpioLed : public QObject
{
    Q_OBJECT

public:
    /*测试使用引脚GPIO3_A5,实际根据鲁班猫板卡引脚修改*/
    const char *chipname = "gpiochip0";
    int line_num = 5;

    struct gpiod_chip *chip;
    struct gpiod_line *line_led;

public:
    int init();
    void set_value(bool flag);
    explicit GpioLed(QObject *parent = nullptr);
    ~GpioLed();

signals:

};

#endif // GPIOLED_H
