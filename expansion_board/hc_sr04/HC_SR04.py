###############################################
#
#  file: hc_sr04.py
#  update: 2024-08-27
#  usage: 
#      sudo python hc_sr04.py
#
###############################################

import gpiod
import time
import math

# A-D : 0-3
# number = group * 8 + x
# e.g. : B0 = 1 * 8 + 0 = 8
#	     C4 = 2 * 8 + 4 = 20 
#
gpionum_trig = 17
gpionum_echo = 14

gpiochip_num_trig = "3"
gpiochip_num_echo = "3"

gpiochip_trig = gpiod.Chip(gpiochip_num_trig, gpiod.Chip.OPEN_BY_NUMBER)
gpiochip_echo = gpiod.Chip(gpiochip_num_echo, gpiod.Chip.OPEN_BY_NUMBER)

TRIG=gpiochip_trig.get_line(gpionum_trig)
TRIG.request(consumer="TRIG", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

ECHO=gpiochip_echo.get_line(gpionum_echo)
ECHO.request(consumer="ECHO", type=gpiod.LINE_REQ_DIR_IN)

timeout = 0.2

def main():

    try:
        
        while True:
            
            # 触发信号 10us
            TRIG.set_value(0)
            TRIG.set_value(1)
            time.sleep(0.00001)
            TRIG.set_value(0)
            
            start_time = time.time()
            while ECHO.get_value()==0 and (time.time() - start_time) < timeout:
                time.sleep(0.0001)
            if ECHO.get_value() == 0:  
                print("Waiting for the ECHO start signal times out")  
                continue

            # 获取当前时间    
            start_time = time.time()

            while ECHO.get_value()==1 and (time.time() - start_time) < timeout:
                time.sleep(0.0001)
            if ECHO.get_value() == 1:  
                print("Waiting for the ECHO end signal times out")  
                continue
            end_time = time.time()

            dist=(end_time - start_time) * 34300 / 2

            print("Distance:%.2f cm"%dist)
            time.sleep(0.02)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:

        TRIG.release()
        ECHO.release()
        
if __name__ == "__main__":  
    main()
