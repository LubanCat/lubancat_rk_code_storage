import board
import digitalio

# 根据具体板卡修改使用的GPIO
# 以LubanCat2 board为例, board.GPIO11就是引出的40Ppin的第11个物理引脚，board.GPIO11 = GPIO3_A5 = Pin 101
led = digitalio.DigitalInOut(board.GPIO11)
led.direction = digitalio.Direction.OUTPUT

try:
    while True:
        led.value=1
        time.sleep(0.5)
        led.value=0
        time.sleep(0.5)

finally:
    led.value = True
    led.deinit()