###############################################
#
#  file: ec11.py
#  update: 2024-08-29
#  usage: 
#      sudo python ec11.py
#
###############################################

import time
import gpiod
import os
import struct
import subprocess
import math
import board
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306
import evdev
import threading

# oled初始化
oled_init_flag = 1      # oled初始化标志位
try:
    oled_i2c = busio.I2C(board.I2C3_SCL, board.I2C3_SDA)        # 初始化I2C
    disp = adafruit_ssd1306.SSD1306_I2C(128, 64, oled_i2c)      # 创建oled对象

    width = disp.width  
    height = disp.height  
    image = Image.new('1', (width, height))  
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, width, height), outline=0, fill=0)    # 清屏

    font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 12, encoding="unic")     # 加载字体
except Exception as e:
    oled_init_flag = 0
    print(e)
#-----------------------------------------------------------------

# ec11初始化
lock = threading.Lock()     # 创建线程锁
ec11_direction = 0          # 0:不动作 1:顺时针旋转 2:逆时针旋转 3:按键按下顺时针旋转 4:按键按下逆时针旋转 5:按键按下
count = 0                   # 旋转步数累计

# 设备文件路径
ec11_SW_event = '/dev/input/event8'
ec11_A_event = '/dev/input/event7'
ec11_B_event = '/dev/input/event6'
ec11_SW_device = evdev.InputDevice(ec11_SW_event)   # 创建按键设备
ec11_A_device = evdev.InputDevice(ec11_A_event)     # 创建A相设备
ec11_B_device = evdev.InputDevice(ec11_B_event)     # 创建B相设备
#-----------------------------------------------------------------

def oled_display_thread():
    """
    @brief : oled显示线程，实时更新显示的步数
    @param : none
    @return: none
    """
    global ec11_direction
    global count
    while True:
        draw.rectangle((0, 0, width, height), outline=0, fill=0)    # 清屏
        draw.text((5, 0), str(count), font=font, fill=255)          # 显示当前步数
        disp.image(image)  
        disp.show() 
        time.sleep(0.01)

if oled_init_flag == 1:
    oled_obj = threading.Thread(target=oled_display_thread)
    oled_obj.daemon = True
    oled_obj.start()
#-----------------------------------------------------------------

# ec11检测线程
ec11_SW_value = 0
ec11_A_value = 1
ec11_B_value = 1
def ec11_scan_thread(device):
    """
    @brief : ec11旋转编码器检测线程，读取旋转和按键事件
    @param : device - 设备对象
    @return: none
    """
    global ec11_SW_value, ec11_A_value, ec11_B_value, ec11_direction
    for event in device.read_loop():
        if event.code == 4:         # 检测按键事件
            with lock:
                ec11_SW_value = event.value
                if(ec11_SW_value == 1 and ec11_A_value == 1 and ec11_B_value == 1):     # 按键按下
                    ec11_direction = 5
        elif event.code == 250:     # 检测A相事件
            with lock:
                if event.value == 0 and ec11_B_value == 1:  # 顺时针旋转
                    ec11_A_value = 0            
                    if ec11_SW_value == 1:                  # 按键按下
                        ec11_direction = 3
                    else:
                        ec11_direction = 1
                elif event.value == 1:
                    ec11_A_value = 1
        elif event.code == 251:     # 检测B相事件
            with lock:
                if event.value == 0 and ec11_A_value == 1:  # 逆时针旋转
                    ec11_B_value = 0
                    if ec11_SW_value == 1:                  # 按键按下
                        ec11_direction = 4
                    else:
                        ec11_direction = 2
                elif event.value == 1:
                    ec11_B_value = 1

# 创建旋转编码器检测线程
ec11_SW_obj = threading.Thread(target=ec11_scan_thread, args=(ec11_SW_device,))
ec11_A_obj = threading.Thread(target=ec11_scan_thread, args=(ec11_A_device,))
ec11_B_obj = threading.Thread(target=ec11_scan_thread, args=(ec11_B_device,))
ec11_SW_obj.daemon = True
ec11_A_obj.daemon = True
ec11_B_obj.daemon = True
ec11_SW_obj.start()
ec11_A_obj.start()
ec11_B_obj.start()
#-----------------------------------------------------------------

def main():

    global ec11_direction
    global count
    
    try:
        
        while True:

            if ec11_direction == 0:  
                pass
            elif ec11_direction == 1:  
                count = count + 1
                print("顺时针转:", count)  
            elif ec11_direction == 2:  
                count = count - 1
                print("逆时针转:", count) 
            elif ec11_direction == 3:  
                count = count + 1
                print("按键按下顺时针转:", count)
            elif ec11_direction == 4:  
                count = count - 1
                print("按键按下逆时针转:", count)  
            elif ec11_direction == 5:  
                print("按键按下")

            with lock:
                ec11_direction = 0      # 重置方向状态

            time.sleep(0.01)

    except Exception as e:
        
        print("exit...：", e)
        
if __name__ == "__main__":  
    main()



