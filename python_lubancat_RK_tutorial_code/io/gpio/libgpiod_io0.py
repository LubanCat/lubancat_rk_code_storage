import time
import gpiod

# 根据具体板卡的LED灯连接修改使用的Chip和Line,没有LED可以自行外接
LINE_OFFSET = 8

chip0 = gpiod.Chip("0", gpiod.Chip.OPEN_BY_NUMBER)

gpio0_b0 = chip0.get_line(LINE_OFFSET)
gpio0_b0.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

print(gpio0_b0.consumer())

try:
    while True:
        gpio0_b0.set_value(1)
        time.sleep(0.5)
        gpio0_b0.set_value(0)
        time.sleep(0.5)
finally:
    gpio0_b0.set_value(1)
    gpio0_b0.release()

