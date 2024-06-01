import time
import gpiod

# 根据需要控制的引脚修改
LINE0_OFFSET = 0
LINE1_OFFSET = 1
LINE2_OFFSET = 2

chip0 = gpiod.Chip("6", gpiod.Chip.OPEN_BY_NUMBER)

gpio6_0 = chip0.get_line(LINE0_OFFSET)
gpio6_1 = chip0.get_line(LINE1_OFFSET)
gpio6_2 = chip0.get_line(LINE2_OFFSET)

gpio6_0.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])
gpio6_1.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])
gpio6_2.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])


print(gpio6_0.consumer())
print(gpio6_1.consumer())
print(gpio6_2.consumer())

try:
    while True:
        gpio6_0.set_value(1)
        gpio6_1.set_value(1)
        gpio6_2.set_value(1)
        time.sleep(0.5)
        gpio6_0.set_value(0)
        gpio6_0.set_value(0)
        gpio6_0.set_value(0)
        time.sleep(0.5)
finally:
    gpio6_0.set_value(1)
    gpio6_1.set_value(1)
    gpio6_2.set_value(1)
    gpio6_0.release()
    gpio6_1.release()
    gpio6_2.release()

