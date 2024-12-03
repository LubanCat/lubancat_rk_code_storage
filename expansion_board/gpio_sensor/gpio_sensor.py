###############################################
#
#  file: gpio_sensor.py
#  update: 2024-10-16
#  usage: 
#      sudo python gpio_sensor.py
#
###############################################

import time
import gpiod

class GPIO:
    global gpio, gpiochip

    # 初始化GPIO对象，打开指定的GPIO芯片和引脚
    def __init__(self, gpionum, gpiochipx):
        self.gpiochip = gpiod.Chip(gpiochipx, gpiod.Chip.OPEN_BY_NUMBER)
        self.gpio = self.gpiochip.get_line(gpionum)

    # 设置引脚为输出方向，并设置初始值
    def set_direction_out(self, val):
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[val])
    
    # 设置引脚为输入方向
    def set_direction_in(self):
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_IN)

    # 设置引脚的输出值
    def set(self, val):
        self.gpio.set_value(val)

    # 获取引脚的输入值
    def get(self):
        return self.gpio.get_value()

    # 释放GPIO引脚
    def release(self):
        self.gpio.release()

# 光敏电阻模块
ldr = GPIO(8, "6")
ldr.set_direction_in()

# 热敏电阻模块
thermistor = GPIO(9, "6")
thermistor.set_direction_in()

# 火焰检测模块
flame = GPIO(10, "6")
flame.set_direction_in()

def main():
    try:
        while True:
            
            # 循环读取三个传感器的状态，并输出到控制台
            do_ldr = ldr.get()
            do_thermistor = thermistor.get()
            do_flame = flame.get()

            res = ("过亮" if do_ldr == 0 else "正常")
            print(f"光敏电阻模块DO脚信号:{do_ldr}, 当前状态:{res}")

            res = ("过热" if do_thermistor == 0 else "正常")
            print(f"热敏电阻模块DO脚信号:{do_thermistor}, 当前状态:{res}")

            res = ("有火焰" if do_flame == 0 else "正常")
            print(f"火焰检测模块DO脚信号:{do_flame}, 当前状态:{res}")

            print("\n")
                  
            # 每隔0.5秒读取一次
            time.sleep(1)

    except Exception as e:
        
        # 捕获异常并打印错误信息
        print("exit...：", e)
    
    finally:
        
        # 确保在退出时释放所有GPIO引脚
        ldr.release()
        thermistor.release()
        flame.release()
        
if __name__ == "__main__":  
    main()