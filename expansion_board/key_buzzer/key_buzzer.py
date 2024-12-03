###############################################
#
#  file: key_buzzer.py
#  update: 2024-10-16
#  usage: 
#      sudo python key_buzzer.py /dev/input/event<X>
#
###############################################

import evdev
import time
import gpiod
import sys

if len(sys.argv) != 2:
    print("Usage: python key_buzzer.py /dev/input/event<X>")
    sys.exit(1)

# 获取传入的参数
arg1 = sys.argv[1]  

# 蜂鸣器引脚初始化
gpiochip = gpiod.Chip("6", gpiod.Chip.OPEN_BY_NUMBER)                           # 打开GPIO芯片
gpio = gpiochip.get_line(6)                                                     # 获取第6号引脚
gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])    # 请求将引脚设置为输出，并初始化为低电平

def main():

    try:

        device = evdev.InputDevice(str(arg1))                                   # 打开输入设备，监测按键事件
        for event in device.read_loop():                                        # 循环读取按键事件
            if event.code == 11 or event.code == 2 or event.code == 3 :         # 检查三个按键的按下和释放
                if event.value == 1:                                            # 按键按下
                    print("buzzer on")
                    gpio.set_value(1)                                           # 打开蜂鸣器
                elif event.value == 0:                                          # 按键释放
                    print("buzzer off")
                    gpio.set_value(0)                                           # 关闭蜂鸣器

    except Exception as e:
        
        print("exit...：", e)                                                   # 捕获异常并打印错误信息
    
    finally:
        
        gpio.set_value(0)                                                       # 确保在程序退出时关闭蜂鸣器和释放GPIO引脚
        gpio.release()
        
if __name__ == "__main__":  
    main()