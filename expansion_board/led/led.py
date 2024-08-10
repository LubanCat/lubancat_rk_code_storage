###############################################
#
#  file: led.py
#  update: 2024-08-10
#  usage: 
#      sudo python led.py
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

# led gpiochip
gpiochip_led = "6"

# led gpionum
gpionum_r_led = 0
gpionum_g_led = 1
gpionum_b_led = 2

# led init
red_led = GPIO(gpionum_r_led, gpiochip_led, 1)          # 初始化红色led, 初始电平为高电平，灯灭
green_led = GPIO(gpionum_g_led, gpiochip_led, 1)        # 初始化绿色led, 初始电平为高电平，灯灭
blue_led = GPIO(gpionum_b_led, gpiochip_led, 1)         # 初始化蓝色led, 初始电平为高电平，灯灭

def main():
    try:
        while True:
            
            red_led.set(0)
            green_led.set(0)
            blue_led.set(0)

            time.sleep(0.5)

            red_led.set(1)
            green_led.set(1)
            blue_led.set(1)

            time.sleep(0.5)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        # led off
        red_led.set(1)
        green_led.set(1)
        blue_led.set(1)

        # led release
        red_led.release()
        green_led.release()
        blue_led.release()
        
if __name__ == "__main__":  
    main()

