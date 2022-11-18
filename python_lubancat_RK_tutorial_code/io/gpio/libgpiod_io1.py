import gpiod

# 根据具体板卡的LED灯和按键连接修改使用的Chip和Line

# 这里以Lubancat2为例，使用GPIO0_B0接LED，GPIO0_C2接按键
LED_LINE_OFFSET = 8
BUTTON_LINE_OFFSET = 18

chip0_led = gpiod.Chip("0", gpiod.Chip.OPEN_BY_NUMBER)
chip0_button = gpiod.Chip("0", gpiod.Chip.OPEN_BY_NUMBER)

led = chip0_led.get_line(LED_LINE_OFFSET)
led.request(consumer="LED", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

button = chip0_button.get_line(BUTTON_LINE_OFFSET)
button.request(consumer="BUTTON", type=gpiod.LINE_REQ_DIR_IN)

print(led.consumer())
print(button.consumer())

try:
    while True:
        led.set_value(button.get_value())
finally:
    led.set_value(1)
    led.release()
    button.release()