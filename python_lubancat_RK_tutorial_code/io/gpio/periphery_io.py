from periphery import GPIO

# 根据具体板卡的LED灯和按键连接修改使用的Chip和Line
LED_CHIP = "/dev/gpiochip0"
LED_LINE_OFFSET = 8

BUTTON_CHIP = "/dev/gpiochip0"
BUTTON_LINE_OFFSET = 18

led = GPIO(LED_CHIP, LED_LINE_OFFSET, "out")
button = GPIO(BUTTON_CHIP, BUTTON_LINE_OFFSET, "in")

try:
    while True:
        led.write(button.read())
finally:
    led.write(True)
    led.close()
    button.close()

