#include "gpioled.h"

GpioLed::GpioLed(QObject *parent)
    : QObject{parent}
{

}

GpioLed::~GpioLed()
{
    gpiod_line_release(line_led);
    gpiod_chip_close(chip);
}

int GpioLed::init()
{
    int ret;

    /* 打开GPIO控制器 */
    chip = gpiod_chip_open_by_name(chipname);
    if (!chip)
    {
        qDebug("gpiod_chip_open_by_name failed!");
        return -1;
    }

    /* 获取引脚 */
    line_led = gpiod_chip_get_line(chip, line_num);
    if (!line_led)
    {
        gpiod_chip_close(chip);
        qDebug("gpiod_chip_get_line failed!");
        return -1;
    }

    /* 配置引脚为输出模式 name为“led” 初始电平为high 教程测试的连接电路，高电平时led不亮*/
    ret = gpiod_line_request_output(line_led, "led", 1);
    if (ret)
    {
        qDebug("led request error.");
        return -1;
    }
    return 0;
}

void GpioLed::set_value(bool flag)
{
    gpiod_line_set_value(line_led, flag?1:0);
}
