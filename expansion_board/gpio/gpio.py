import time
import gpiod

# 根据需要控制的引脚修改
LINE_OFFSET = 0

chip0 = gpiod.Chip("6", gpiod.Chip.OPEN_BY_NUMBER)

gpio6_0 = chip0.get_line(LINE_OFFSET)
gpio6_0.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

print(gpio6_0.consumer())

try:
    while True:
        gpio6_0.set_value(1)
        time.sleep(0.5)
        gpio6_0.set_value(0)
        time.sleep(0.5)
finally:
    gpio6_0.set_value(1)
    gpio6_0.release()
