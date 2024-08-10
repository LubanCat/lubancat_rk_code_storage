###############################################
#
#  file: buzzer.py
#  update: 2024-08-10
#  usage: 
#      sudo python buzzer.py
#
###############################################

import time
import gpiod

class GPIO:
    global gpio, gpiochip

    def __init__(self, gpionum, gpiochipx, val):
        self.gpiochip = gpiod.Chip(gpiochipx, gpiod.Chip.OPEN_BY_NUMBER)
        self.gpio = self.gpiochip.get_line(gpionum)
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[val])
    
    def set(self, val):
        self.gpio.set_value(val)

    def release(self):
        self.gpio.release()

# buzzer gpiochip
gpiochip_buzzer = "6"

# buzzer gpionum
gpionum_buzzer = 6

# buzzer init
buzzer = GPIO(gpionum_buzzer, gpiochip_buzzer, 0)          # 初始化蜂鸣器, 初始电平为低电平，蜂鸣器不工作

def main():
    try:
        while True:
            
            buzzer.set(1)

            time.sleep(0.5)

            buzzer.set(0)

            time.sleep(0.5)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        # buzzer off
        buzzer.set(0)

        # buzzer release
        buzzer.release()
        
if __name__ == "__main__":  
    main()