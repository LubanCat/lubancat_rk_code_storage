###############################################
#
#  file: main.py
#  update: 2024-10-18
#  usage: 
#      sudo python main.py
#
###############################################

import evdev
import time
import gpiod
import sys
import threading
from config import ConfigManager

class GPIO:
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

# LED心跳线程函数
def led_heart_beat_thread_fun():
    print("创建LED心跳线程")
    while True:
        rled.set(0)         # 点亮LED1
        time.sleep(1)
        rled.set(1)         # 熄灭LED1
        time.sleep(1)

# 光敏LED线程函数
def led_brightness_thread_fun():
    print("创建光敏LED线程")
    while True:
        if ldr.get() == 0:
            gled.set(1)     # 光线弱时点亮LED2
        else:
            gled.set(0)     # 光线强时熄灭LED2
        time.sleep(0.2)

# 温度报警线程函数
def led_alarm_thread_fun():
    print("创建温度报警线程")
    while True:
        if ntc.get() == 0 or flame.get() == 0:
            bled.set(0)     # 温度报警时点亮LED3
            buzzer.set(1)   # 蜂鸣器响
        time.sleep(0.2)

# 配置文件初始化
config_file = '../configuration.json' 
config_manager = ConfigManager(config_file)  
if config_manager.inspect_current_environment() is not True:  
    exit()

# 板载按键初始化
key_config = config_manager.get_board_config("key")                 # 从配置文件中获取按键相关配置信息 
if key_config is None:  
    print("can not find 'key' key in ", config_file)
    exit()
key_event = key_config['event']                                     # 获取按键的输入事件
key1_device = evdev.InputDevice(key_event)

# 初始化光敏电阻模块
ldr_config = config_manager.get_board_config("ldr")                 # 从配置文件中获取光敏模块相关配置信息 
if ldr_config is None:  
    print("can not find 'ldr' key in ", config_file)
    exit()

ldr_chip = ldr_config['pin_chip']
ldr_num = ldr_config['pin_num']

ldr = GPIO(ldr_num, ldr_chip)
ldr.set_direction_in()

# 初始化热敏电阻模块
ntc_config = config_manager.get_board_config("ntc")                 # 从配置文件中获取热敏模块相关配置信息 
if ntc_config is None:  
    print("can not find 'ntc' key in ", config_file)
    exit()

ntc_chip = ntc_config['pin_chip']
ntc_num = ntc_config['pin_num']

ntc = GPIO(ntc_num, ntc_chip)
ntc.set_direction_in()

# 初始化火焰检测模块
flame_config = config_manager.get_board_config("flame")             # 从配置文件中获取火焰模块相关配置信息 
if flame_config is None:  
    print("can not find 'flame' key in ", config_file)
    exit()

flame_chip = flame_config['pin_chip']
flame_num = flame_config['pin_num']

flame = GPIO(flame_num, flame_chip)
flame.set_direction_in()

# 初始化LED1
rled_config = config_manager.get_board_config("led-r")              # 从配置文件中获取ledr相关配置信息 
if rled_config is None:  
    print("can not find 'led-r' key in ", config_file)
    exit()

rled_chip = rled_config['pin_chip']
rled_num = rled_config['pin_num']

rled = GPIO(rled_num, rled_chip)
rled.set_direction_out(1)

# 初始化LED2
gled_config = config_manager.get_board_config("led-g")              # 从配置文件中获取ledg相关配置信息 
if gled_config is None:  
    print("can not find 'led-g' key in ", config_file)
    exit()

gled_chip = gled_config['pin_chip']
gled_num = gled_config['pin_num']

gled = GPIO(gled_num, gled_chip)
gled.set_direction_out(1)

# 初始化LED3
bled_config = config_manager.get_board_config("led-b")              # 从配置文件中获取ledg相关配置信息 
if bled_config is None:  
    print("can not find 'led-b' key in ", config_file)
    exit()

bled_chip = bled_config['pin_chip']
bled_num = bled_config['pin_num']

bled = GPIO(bled_num, bled_chip)
bled.set_direction_out(1)

# 初始化蜂鸣器
buzzer_config = config_manager.get_board_config("buzzer")           # 从配置文件中获取ledg相关配置信息 
if buzzer_config is None:  
    print("can not find 'buzzer' key in ", buzzer_config)
    exit()

buzzer_chip = buzzer_config['pin_chip']
buzzer_num = buzzer_config['pin_num']

buzzer = GPIO(buzzer_num, buzzer_chip)
buzzer.set_direction_out(0)

# 创建线程
led_heart_beat_thread = threading.Thread(target=led_heart_beat_thread_fun)
led_brightness_thread = threading.Thread(target=led_brightness_thread_fun)
led_alarm_thread = threading.Thread(target=led_alarm_thread_fun)

# 设置线程为守护线程
led_heart_beat_thread.daemon = True
led_brightness_thread.daemon = True
led_alarm_thread.daemon = True

# 启动线程
led_heart_beat_thread.start()
led_brightness_thread.start()
led_alarm_thread.start()

def main():
    try:
        while True:
            for event in key1_device.read_loop():           # 循环读取按键事件
                if event.code == 11 and event.value == 1:   # 检查key1按键是否按下
                    bled.set(1)                             # 点亮LED3
                    buzzer.set(0)                           # 关闭蜂鸣器
            time.sleep(0.2)

    except Exception as e:
        print("退出...：", e)                               # 处理异常
    
    finally:
        # 清理资源
        rled.set(1)
        gled.set(1)
        bled.set(1)
        buzzer.set(0)

        rled.release()
        gled.release()
        bled.release()
        buzzer.release()
        
if __name__ == "__main__":  
    main()
