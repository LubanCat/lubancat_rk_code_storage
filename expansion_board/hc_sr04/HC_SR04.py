import gpiod
import time
import math

chip3_C1_TRIG = 17
chip3_B6_ECHO = 14

chip_TRIG = gpiod.Chip("3", gpiod.Chip.OPEN_BY_NUMBER)
chip_ECHO = gpiod.Chip("3", gpiod.Chip.OPEN_BY_NUMBER)

TRIG=chip_TRIG.get_line(chip3_C1_TRIG)
TRIG.request(consumer="TRIG", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

ECHO=chip_ECHO.get_line(chip3_B6_ECHO)
ECHO.request(consumer="ECHO", type=gpiod.LINE_REQ_DIR_IN)
#ECHO=chip_TRIG.get_line(chip3_B6_ECHO)
#ECHO.request(consumer="ECHO", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

try:

    while True:
        TRIG.set_value(0)
        TRIG.set_value(1)
        time.sleep(0.00001)
        TRIG.set_value(0)
        
        # 获取当前时间
        while(ECHO.get_value()==0):
            pass
        start_time = time.time()
        while(ECHO.get_value()==1):
            pass
        end_time = time.time()
        dist=(end_time-start_time)* 34300 / 2
        print("Distance:%.2f cm"%dist)
        time.sleep(0.02)
finally:
    TRIG.release()
    ECHO.release()