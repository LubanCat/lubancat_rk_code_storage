import time
import gpiod

LINE_OFFSET = 6

chip0 = gpiod.Chip("6", gpiod.Chip.OPEN_BY_NUMBER)

gpio6_6 = chip0.get_line(LINE_OFFSET)
gpio6_6.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

print(gpio6_6.consumer())

try:
    while True:
        gpio6_6.set_value(1)
        time.sleep(0.5)
        gpio6_6.set_value(0)
        time.sleep(0.5)
finally:
    gpio6_6.set_value(1)
    gpio6_6.release()
