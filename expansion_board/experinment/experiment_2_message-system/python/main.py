###############################################
#
#  file: main.py
#  update: 2024-09-04
#  usage: 
#      sudo python main.py
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
import threading
import evdev
import numpy as np
from collections import deque 

from mpu6050 import MPU6050
from max30102 import MAX30102
from config import ConfigManager

# global var
ec11Lock = threading.Lock()
ec11_SW_value   = 0
ec11_A_value    = 1
ec11_B_value    = 1
input_bit       = 0                     # 0:不动作 1:顺时针旋转 2:逆时针旋转 3:按键按下顺时针旋转 4:按键按下逆时针旋转 5:按键按下 7:板载key2按键按下

mpu6050_init_flag   = 1                 # 0:未初始化 1:已初始化
max30102_init_flag  = 1                 # 0:未初始化 1:已初始化

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

class MenuInfo:
    def __init__(self, name, nextMenuItem, action):
        '''
        初始化MenuInfo对象。
        
        参数:
        name (str): 菜单项的名称。
        nextMenuItem (MenuItem or None): 下一个菜单对象，若为None则表示没有下一个菜单对象。
        action (function or None): 进入该菜单项时要执行的函数，若为None则表示不执行函数。
        '''
        self.name = name
        self.nextMenuItem = nextMenuItem
        self.action = action
    
    # 获取当前menuInfo下的action
    def getAction(self):
        return self.action

    # 获取当前menuInfo下的menuItem
    def getMenuItem(self):
        return self.nextMenuItem

    # 获取当前menuInfo的名字
    def getName(self):
        return self.name

class MenuItem:
    def __init__(self, title):
        '''
        初始化MenuItem对象。
        
        参数:
        title (str): 菜单的标题。
        '''
        self.title = title
        self.menuInfo = []
        self.totalPage = 0
        self.currentPage = 0

        self.preMenuItem = None

    # 添加menuInfo
    def addMenuInfo(self, menuInfo:MenuInfo):
        '''
        向该菜单项中添加一个MenuInfo对象。
        
        参数:
        menuInfo (MenuInfo): 要添加的MenuInfo对象。
        '''
        if not self.menuInfo:
            self.currentPage = 1
        self.menuInfo.append(menuInfo)
        self.totalPage = len(self.menuInfo)

    # 切换到上一级menuItem
    def goPreMenuItem(self):
        '''
        切换到上一级菜单。
        
        返回:
        MenuItem: 上一级菜单对象或自身（如果没有上一级）。
        '''
        if self.preMenuItem != None:
            self.preMenuItem.parseMenu()
            return self.preMenuItem
        self.parseMenu()
        return self
            
    # 切换到下一级menuItem
    def goNextMenuItem(self):
        '''
        切换到下一级菜单或执行动作函数。
        
        返回:
        MenuItem: 下一级菜单对象或自身。
        '''
        nextItem = self.menuInfo[self.currentPage - 1].getMenuItem()
        action = self.menuInfo[self.currentPage - 1].getAction()
        if nextItem != None:    # 如果存在下一级menuItem，则切换
            nextItem.parseMenu()
            nextItem.preMenuItem = self
            return nextItem
        if action != None:      # 如果存在函数，则执行
            action()
        self.parseMenu()
        return self

    # 切换到上一个menuInfo
    def goPreMenuInfo(self):
        '''
        切换到上一个MenuInfo页面。
        '''
        if self.currentPage != 1:
            self.currentPage -= 1
        self.parseMenu()

    # 切换到下一个menuInfo
    def goNextMenuInfo(self):
        '''
        切换到下一个MenuInfo页面。
        '''
        if self.currentPage != self.totalPage:
            self.currentPage = self.currentPage + 1
        self.parseMenu()

    # 解析menuItem
    def parseMenu(self):
        '''
        解析并显示当前菜单项的信息。
        '''
        # 判断当前menuInfo是否是第一个
        isPrePage = self.currentPage > 1
        isNextPage = self.currentPage < self.totalPage

        # oled显示当前menuInfo信息
        self.__menuDisplay(self.menuInfo[self.currentPage - 1].getName(), self.title, self.currentPage, self.totalPage, isPrePage, isNextPage)
        
    def __menuDisplay(self, itemName:str, preItemName:str, currentPage:int, totalPage:int, isPrePage:bool, isNextPage:bool):
        '''
        在OLED屏幕上显示菜单相关的信息。
        
        参数:
        itemName (str): 当前菜单的名称。
        preItemName (str): 当前菜单的菜单项。
        currentPage (int): 当前页码。
        totalPage (int): 总页码。
        isPrePage (bool): 是否有上一页。
        isNextPage (bool): 是否有下一页。
        '''
        fontSize = 16
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        # 显示外框
        rectangleX = 1
        rectangleY = 1
        draw.rectangle((rectangleX, rectangleY, width-rectangleX, height-rectangleY), outline=2, fill=0)

        # 显示标题
        itemNamePixelOffsetY = 5
        itemWidth = draw.textlength(itemName, font=font)  
        x = (image.width - itemWidth) // 2 
        top = ((image.height - fontSize) // 2) - itemNamePixelOffsetY
        draw.text((x, top), itemName, font=font, fill=255) 

        # 显示箭头
        arrowPixelOffsetX = 5
        if isPrePage:
            draw.text((rectangleX + arrowPixelOffsetX, top), "<", font=font, fill=255)
        if isNextPage:
            arrowWidth = draw.textlength(">", font=font)
            draw.text((image.width - arrowWidth - arrowPixelOffsetX, top), ">", font=font, fill=255)

        # 显示页码
        fontSize = 10
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        infoPixelOffsetY = 5
        info = str(currentPage) + " / "  + str(totalPage) 
        infoWidth = draw.textlength(info, font=font)
        x = (image.width - infoWidth) // 2 
        top = (image.height - fontSize) - infoPixelOffsetY
        draw.text((x, top), info, font=font, fill=255)

        # 显示所属的菜单项
        fontSize = 10
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        preItemNamePixelOffsetY = 1
        preItemWidth = draw.textlength(preItemName, font=font)  
        x = (image.width - preItemWidth) // 2 
        top = rectangleX + preItemNamePixelOffsetY
        draw.text((x, top), preItemName, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

def ec11_scan_thread(device):
    '''
    扫描EC11输入事件，并根据事件更新input_bit。
    
    参数:
    device: EC11编码器输入事件。
    
    其它：
    顺时针旋转时，A相会比B相先拉高。所以在当A相输出高电平时，同时检测B相的状态即可判断是否为顺时针旋转。
    逆时针旋转时，B相会比A相先拉高。所以在当B相输出高电平时，同时检测A相的状态即可判断是否为逆时针旋转。
    '''
    global ec11_SW_value, ec11_A_value, ec11_B_value, input_bit
    for event in device.read_loop():
        if event.code == 4:                                 # 检测按键事件
            with ec11Lock:
                ec11_SW_value = event.value
                if(ec11_SW_value == 1 and ec11_A_value == 1 and ec11_B_value == 1):     # 按键按下
                    input_bit = 5
        elif event.code == 250:                             # 检测A相事件
            with ec11Lock:
                if event.value == 0 and ec11_B_value == 1:  # 顺时针旋转
                    ec11_A_value = 0            
                    if ec11_SW_value == 1:                  # 若按键按下
                        input_bit = 3                  # 代表 按键按下且顺时针旋转
                    else:                                   # 若按键未按下
                        input_bit = 1                  # 代表 顺时针旋转
                elif event.value == 1:
                    ec11_A_value = 1
        elif event.code == 251:                             # 检测B相事件
            with ec11Lock:
                if event.value == 0 and ec11_A_value == 1:  # 逆时针旋转
                    ec11_B_value = 0
                    if ec11_SW_value == 1:                  # 若按键按下
                        input_bit = 4                  # 代表 按键按下且逆时针旋转
                    else:                                   # 若按键未按下
                        input_bit = 2                  # 代表 逆时针旋转
                elif event.value == 1:
                    ec11_B_value = 1

def key2_scan_thread(device):
    '''
    扫描按键输入事件，并根据事件更新input_bit。
    
    参数:
    device: 按键输入事件。
    '''
    global input_bit
    for event in device.read_loop():
        if event.code == 2 and event.value == 1:
            with ec11Lock:
                input_bit = 7

def ultrasonicInfoFun():
    '''
    超声波菜单项关联函数。
    在oled上显示超声波测距距离，同时配合蜂鸣器模拟倒车雷达。
    '''
    timeout = 0.2                                   # 超时时间
    titleStr = "超声波"                              # 菜单标题

    delayweight = 0.8                               # 蜂鸣器鸣响的延迟权重
    buffer_size = 3                                 # 定义缓冲区大小，即存储多少次测量结果  
    distance_buffer = deque(maxlen=buffer_size)     # 使用deque作为环形缓冲区

    # 显示标题
    draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)
    font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
    titleStrWidth = draw.textlength(titleStr, font=font) 
    x = (image.width - titleStrWidth) // 2 
    draw.text((x, 2), titleStr, font=font, fill=255)
    disp.image(image)  
    disp.show()

    while True:

        if input_bit == 7:                           
            break

        # 触发信号 10us
        TRIG.set_value(0)
        TRIG.set_value(1)
        time.sleep(0.00001)
        TRIG.set_value(0)
        
        # 等待 ECHO 信号开始
        start_time = time.time()
        while ECHO.get_value() == 0:
            if time.time() - start_time > timeout:
                print("Waiting for the ECHO start signal timed out")
                break

        # 如果超时则跳过这次测量
        if ECHO.get_value() == 0:
            continue

        # 记录 ECHO 信号开始时间
        start_time = time.time()

        # 等待 ECHO 信号结束
        while ECHO.get_value() == 1:
            if time.time() - start_time > timeout:
                print("Waiting for the ECHO end signal timed out")
                break
        
        # 如果超时则跳过这次测量
        if ECHO.get_value() == 1:
            continue
        
        end_time = time.time()
        distance = (end_time - start_time) * 34300 / 2

        # 将新测量结果添加到缓冲区中  
        distance_buffer.append(distance)  
            
        # 计算缓冲区中所有测量结果的平均值  
        avg_dist = sum(distance_buffer) / len(distance_buffer) if distance_buffer else 0

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")

        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        draw.text((x, 2), titleStr, font=font, fill=255)

        # 显示数据
        fontSize = 14
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        distStr = "Dist:" + str("%.2f"%avg_dist) + "cm"
        distStrWidth = draw.textlength(distStr, font=font) 
        x = (image.width - distStrWidth) // 2 
        top = ((image.height - fontSize) // 2)
        draw.text((x, top), distStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        if avg_dist <= 3:
            BUZZER.set_value(1)
        elif avg_dist <= 6:
            BUZZER.set_value(1)
            time.sleep(avg_dist*0.01*delayweight*0.3)
            BUZZER.set_value(0)
            time.sleep(avg_dist*0.01*delayweight*0.3)
        elif avg_dist <= 10:
            BUZZER.set_value(1)
            time.sleep(avg_dist*0.01*delayweight*0.5)
            BUZZER.set_value(0)
            time.sleep(avg_dist*0.01*delayweight*0.5)
        elif avg_dist <= 13:
            BUZZER.set_value(1)
            time.sleep(avg_dist*0.01*delayweight*0.8)
            BUZZER.set_value(0)
            time.sleep(avg_dist*0.01*delayweight*0.8)
        elif avg_dist <= 15:
            BUZZER.set_value(1)
            time.sleep(avg_dist*0.01*delayweight*1.3)
            BUZZER.set_value(0)
            time.sleep(avg_dist*0.01*delayweight*1.3)
        else:
            BUZZER.set_value(0)
            time.sleep(0.001)

def dht11InfoFun():
    '''
    DHT11菜单项关联函数。
    在oled上显示环境温湿度。
    '''
    titleStr = "DHT11"                              # 菜单标题

    while True:

        if input_bit == 7:                           
            break

        # 从设备节点读取6个字节的数据
        data = os.read(dht11_fd, 6)

        # 解析数据并打印
        print(f"Temperature: {data[2]}.{data[3]}℃, Humidity: {data[0]}.{data[1]}")

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        draw.text((x, 2), titleStr, font=font, fill=255)

        # 显示数据
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 14, encoding="unic")
        tempStr = "temp: " + "{:d}.{:d}".format(data[2], data[3])
        humiStr = "humi: " + "{:d}.{:d}".format(data[0], data[1])
        draw.text((5, 15), tempStr, font=font, fill=255)
        draw.text((5, 30), humiStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(1)
        
def mpu6050AccelInfoFun():
    '''
    MPU6050菜单下的加速度菜单项的关联函数。
    在oled上显示加速度相关数据。
    '''
    titleStr = "加速度"

    if mpu6050_init_flag == 0:

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        draw.text((x, 2), titleStr, font=font, fill=255)

        # 显示内容
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 14, encoding="unic")
        draw.text((5, 20), "no sensor...", font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        while True:
            if input_bit == 7:                           
                return
            time.sleep(0.002)

    while True:

        if input_bit == 7:                           
            break

        accelX, accelY, accelZ = mpu6050.readAcc()
        accelXStr = "X: " + str("%.2f"%accelX) + " m/s^2"
        accelYStr = "Y: " + str("%.2f"%accelY) + " m/s^2"
        accelZStr = "Z: " + str("%.2f"%accelZ) + " m/s^2"

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示内容
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 12, encoding="unic")
        draw.text((5, 16), accelXStr, font=font, fill=255)
        draw.text((5, 32), accelYStr, font=font, fill=255)
        draw.text((5, 48), accelZStr, font=font, fill=255)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.002)

def mpu6050GyroInfoFun():
    '''
    MPU6050菜单下的陀螺仪菜单项的关联函数。
    在oled上显示陀螺仪相关数据。
    '''
    titleStr = "陀螺仪"

    if mpu6050_init_flag == 0:

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 显示内容
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 14, encoding="unic")
        draw.text((5, 20), "no sensor...", font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        while True:
            if input_bit == 7:                           
                return
            time.sleep(0.002)

    while True:

        if input_bit == 7:                           
            break
        
        gyroX, gyroY, gyroZ = mpu6050.readGyro()
        gyroXStr = "X: " + str("%.2f"%gyroX) + " deg/s"
        gyroYStr = "Y: " + str("%.2f"%gyroY) + " deg/s"
        gyroZStr = "Z: " + str("%.2f"%gyroZ) + " deg/s"

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示内容
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 12, encoding="unic")
        draw.text((5, 16), gyroXStr, font=font, fill=255)
        draw.text((5, 32), gyroYStr, font=font, fill=255)
        draw.text((5, 48), gyroZStr, font=font, fill=255)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.002)

def max30102InfoFun():
    '''
    MAX30102菜单项的关联函数。
    在oled上显示心率和血氧饱和度数据。
    '''
    titleStr = "心率血氧"

    if max30102_init_flag == 0:

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        draw.text((x, 2), titleStr, font=font, fill=255)

        # 显示内容
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 14, encoding="unic")
        draw.text((5, 20), "no sensor...", font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        while True:
            if input_bit == 7:                           
                return
            time.sleep(0.002)

    # 初始化数据缓存
    CACHE_NUMS = 150
    PPG_DATA_THRESHOLD = 100000
    ppg_data_cache_ir = np.zeros(CACHE_NUMS)                    # 红外光缓存区
    ppg_data_cache_red = np.zeros(CACHE_NUMS)                   # 红光缓存区
    cache_counter = 0                                           # 缓存计数器

    draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)
    # 先显示一次标题
    font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
    titleStrWidth = draw.textlength(titleStr, font=font) 
    x = (image.width - titleStrWidth) // 2 
    draw.text((x, 2), titleStr, font=font, fill=255)

    # 先显示一次内容           
    font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 14, encoding="unic")
    draw.text((5, 16), "BMP: 0", font=font, fill=255)
    draw.text((5, 32), "SpO2: 0.00", font=font, fill=255)

    disp.image(image)  
    disp.show()

    print("Start sensor ...")

    while True:

        if input_bit == 7:                           
            break
        
        # 读取FIFO数据
        ir_data, red_data = max30102.read_fifo()
        filter_ir_data, filter_red_data = max30102.low_pass_filter(ir_data, red_data)
        filter_ir_data, filter_red_data = max30102.average_filter(filter_ir_data, filter_red_data)
        #print("raw data :", ir_data, red_data)
        #print("filter data :", filter_ir_data, filter_red_data)

        # 收集有效数据
        if filter_ir_data > PPG_DATA_THRESHOLD and filter_red_data > PPG_DATA_THRESHOLD:
            ppg_data_cache_ir[cache_counter] = filter_ir_data       # 缓存红外光数据
            ppg_data_cache_red[cache_counter] = filter_red_data     # 缓存红光数据
            cache_counter += 1                                      # 更新缓存计数器
        else:
            print("未检测到手指！")
            pass

        # 计算血氧和心率
        if cache_counter >= CACHE_NUMS:                             # 当缓存满时进行计算
            bmp = max30102.get_heart_rate(ppg_data_cache_ir, cache_counter)  
            spo2 = max30102.get_spo2(ppg_data_cache_ir, ppg_data_cache_red, cache_counter)
            print(f"心率：{bmp:.0f} 次/min\t血氧：{spo2:.2f}%")

            # 更新OLED显示心率和血氧
            draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)
            font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 14, encoding="unic")
            draw.text((5, 16), "BMP: "+str(int(bmp)), font=font, fill=255)                 # 心率显示
            draw.text((5, 32), "SpO2: "+str(round(spo2, 2)), font=font, fill=255)           # 血氧显示

            # 显示标题
            font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
            titleStrWidth = draw.textlength(titleStr, font=font) 
            x = (image.width - titleStrWidth) // 2 
            draw.text((x, 2), titleStr, font=font, fill=255)

            disp.image(image)  
            disp.show() 

            cache_counter = 0       # 重置缓存计数器

        time.sleep(0.012)           # 读取间隔

def ldrntcInfoFun():
    '''
    光敏热敏电阻模块菜单项的关联函数。
    在oled上显示光敏和热敏模块的状态。
    '''
    titleStr = "光敏热敏"

    while True:

        if input_bit == 7:                           
            break
        
        value_ldr = LDR.get_value()
        value_ntc = NTC.get_value()

        res_ldr = ("过亮" if value_ldr == 0 else "正常")
        res_ntc = ("过热" if value_ntc == 0 else "正常")

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示内容
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 14, encoding="unic")
        draw.text((5, 16), "光敏: "+res_ldr, font=font, fill=255)
        draw.text((5, 32), "热敏:"+res_ntc, font=font, fill=255)

        # 显示标题
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 10, encoding="unic")
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.3)

# to load config file
config_file = '../configuration.json' 
config_manager = ConfigManager(config_file)  
if config_manager.inspect_current_environment() is not True:  
    exit()

# oled init
oled_config = config_manager.get_board_config("oled")               # 从配置文件中获取oled相关配置信息 
if oled_config is None:  
    print("can not find 'oled' key in ", config_file)
    exit()

bus_number = oled_config['bus']                                     # 获取i2c总线编号
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")

try:
    i2c_oled = busio.I2C(scl_pin, sda_pin)                              
    disp = adafruit_ssd1306.SSD1306_I2C(128, 64, i2c_oled)

    width = disp.width  
    height = disp.height  
    image = Image.new('1', (width, height))  
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, width, height), outline=0, fill=0)

    font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 16, encoding="unic")  
    x = 0   
    top = 0  
except Exception as e:
    print(e)
    exit(1)

# ec11 ecoder init
ec11_config = config_manager.get_board_config("ec11")               # 从配置文件中获取ec11相关配置信息 
if ec11_config is None:  
    print("can not find 'ec11' key in ", config_file)
    exit()

ec11_SW_event = ec11_config['sw-event']
ec11_A_event  = ec11_config['a-event']
ec11_B_event  = ec11_config['b-event']
ec11_SW_device = evdev.InputDevice(ec11_SW_event)
ec11_A_device  = evdev.InputDevice(ec11_A_event)
ec11_B_device  = evdev.InputDevice(ec11_B_event)

ec11_SW_thread = threading.Thread(target=ec11_scan_thread, args=(ec11_SW_device,))
ec11_A_thread  = threading.Thread(target=ec11_scan_thread, args=(ec11_A_device,))
ec11_B_thread  = threading.Thread(target=ec11_scan_thread, args=(ec11_B_device,))

ec11_SW_thread.daemon = True
ec11_A_thread.daemon  = True
ec11_B_thread.daemon  = True

ec11_SW_thread.start()
ec11_A_thread.start()
ec11_B_thread.start()

# key2 init
key_config = config_manager.get_board_config("key")                 # 从配置文件中获取按键相关配置信息 
if key_config is None:  
    print("can not find 'key' key in ", config_file)
    exit()

key_event = key_config['event']                                     # 获取按键的输入事件
key2_device = evdev.InputDevice(key_event)

key2_thread = threading.Thread(target=key2_scan_thread, args=(key2_device,))
key2_thread.daemon = True
key2_thread.start()

# hcsr04 init
hcsr04_config = config_manager.get_board_config("hcsr04")           # 从配置文件中获取超声波模块相关配置信息 
if hcsr04_config is None:  
    print("can not find 'hcsr04' key in ", config_file)
    exit()

gpionum_trig = hcsr04_config['trig_pin_num']
gpionum_echo = hcsr04_config['echo_pin_num']

gpiochip_num_trig = hcsr04_config['trig_pin_chip']
gpiochip_num_echo = hcsr04_config['echo_pin_chip']

gpiochip_trig = gpiod.Chip(gpiochip_num_trig, gpiod.Chip.OPEN_BY_NUMBER)
gpiochip_echo = gpiod.Chip(gpiochip_num_echo, gpiod.Chip.OPEN_BY_NUMBER)

TRIG=gpiochip_trig.get_line(gpionum_trig)
TRIG.request(consumer="TRIG", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

ECHO=gpiochip_echo.get_line(gpionum_echo)
ECHO.request(consumer="ECHO", type=gpiod.LINE_REQ_DIR_IN)

# buzzer init
buzzer_config = config_manager.get_board_config("buzzer")               # 从配置文件中获取蜂鸣器相关配置信息 
if buzzer_config is None:  
    print("can not find 'buzzer' in ", config_file)
    exit()

gpiochip_num_buzzer = buzzer_config['pin_chip']
gpionum_buzzer = buzzer_config['pin_num']

gpiochip_buzzer = gpiod.Chip(gpiochip_num_buzzer, gpiod.Chip.OPEN_BY_NUMBER)
BUZZER=gpiochip_buzzer.get_line(gpionum_buzzer)
BUZZER.request(consumer="BUZZER", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

# dht11 init
dht11_config = config_manager.get_board_config("dht11")                 # 从配置文件中获取dht11相关配置信息 
if dht11_config is None:  
    print("can not find 'dht11' in ", config_file)
    exit()
dht11_dev = dht11_config['devname']
dht11_fd = os.open(dht11_dev, os.O_RDONLY)

# mpu6050 init
mpu6050_config = config_manager.get_board_config("mpu6050")             # 从配置文件中获取mpu6050相关配置信息 
if mpu6050_config is None:  
    print("can not find 'mpu6050' in ", config_file)
    exit()

bus_number = mpu6050_config['bus']                                      # 获取i2c总线编号
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  

    i2c = busio.I2C(scl_pin, sda_pin)
    mpu6050 = MPU6050(i2c)
    mpu6050.start()
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")
except OSError as e:
    print(f"OSError: {e}. Make sure the device is connected and the address is correct.")
    mpu6050_init_flag = 0
except ValueError as e:
    print(f"ValueError: {e}. Check the I2C device address.")
    mpu6050_init_flag = 0

# max30102 init
max30102_config = config_manager.get_board_config("max30102")               # 从配置文件中获取max30102相关配置信息 
if max30102_config is None:  
    print("can not find 'max30102' in ", config_file)
    exit()

bus_number = max30102_config['bus']                                          # 获取i2c总线编号
try:                                                    
    max30102 = MAX30102(i2c_bus=bus_number)
except OSError as e:
    print(f"OSError: {e}. Make sure the device is connected and the address is correct.")
    max30102_init_flag = 0
except ValueError as e:
    print(f"ValueError: {e}. Check the I2C device address.")
    max30102_init_flag = 0

# ldr ntc init
ldr_config = config_manager.get_board_config("ldr")                   # 从配置文件中获取ldr相关配置信息 
if ldr_config is None:  
    print("can not find 'ldr' in ", config_file)
    exit()

ntc_config = config_manager.get_board_config("ntc")                   # 从配置文件中获取ntc相关配置信息 
if ntc_config is None:  
    print("can not find 'ntc' in ", config_file)
    exit()

# ldr init
gpiochip_num_ldr = ldr_config['ldr_pin_chip']
gpionum_ldr = ldr_config['ldr_pin_num']

gpiochip_ldr = gpiod.Chip(gpiochip_num_ldr, gpiod.Chip.OPEN_BY_NUMBER)
LDR=gpiochip_ldr.get_line(gpionum_ldr)
LDR.request(consumer="LDR", type=gpiod.LINE_REQ_DIR_IN)

# ntc init
gpiochip_num_ntc = ntc_config['ntc_pin_chip']
gpionum_ntc = ntc_config['ntc_pin_num']

gpiochip_ntc = gpiod.Chip(gpiochip_num_ntc, gpiod.Chip.OPEN_BY_NUMBER)
NTC=gpiochip_ntc.get_line(gpionum_ntc)
NTC.request(consumer="NTC", type=gpiod.LINE_REQ_DIR_IN)

def main():

    try:
        global input_bit

        # MPU6050子菜单
        MPU6050Menu = MenuItem("MPU6050")                                   # 创建"MPU6050"菜单
        accelInfo = MenuInfo("加速度", None, mpu6050AccelInfoFun)           # 创建"加速度"菜单项，同时关联mpu6050AccelInfoFun函数
        gyroInfo = MenuInfo("陀螺仪", None, mpu6050GyroInfoFun)             # 创建"陀螺仪"菜单项，同时关联mpu6050GyroInfoFun函数
        MPU6050Menu.addMenuInfo(accelInfo)                                  # 为"MPU6050"菜单添加"加速度"菜单项
        MPU6050Menu.addMenuInfo(gyroInfo)                                   # 为"MPU6050"菜单添加"陀螺仪"菜单项

        # 总菜单
        mesMenu = MenuItem("信息菜单")                                      # 创建"信息菜单"菜单
        ultrasonicInfo = MenuInfo("超声波", None, ultrasonicInfoFun)        # 创建"超声波"菜单项，同时关联ultrasonicInfoFun函数
        dht11Info = MenuInfo("DHT11", None, dht11InfoFun)                   # 创建"DHT11"菜单项，同时关联dht11InfoFun函数
        MPU6050Info = MenuInfo("MPU6050", MPU6050Menu, None)                # 创建"MPU6050"菜单项，同时关联"MPU6050"菜单
        MAX30102Info = MenuInfo("血氧心率", None, max30102InfoFun)          # 创建"血氧心率"菜单项，同时关联max30102InfoFun函数
        ldrntcInfo = MenuInfo("光敏热敏", None, ldrntcInfoFun)              # 创建"光敏热敏"菜单项，同时关联ldrntcInfoFun函数
        mesMenu.addMenuInfo(ultrasonicInfo)                                 # 为"信息菜单"菜单添加"超声波"菜单项
        mesMenu.addMenuInfo(dht11Info)                                      # 为"信息菜单"菜单添加"DHT11"菜单项
        mesMenu.addMenuInfo(MPU6050Info)                                    # 为"信息菜单"菜单添加"MPU6050"菜单项
        mesMenu.addMenuInfo(MAX30102Info)                                   # 为"信息菜单"菜单添加"血氧心率"菜单项
        mesMenu.addMenuInfo(ldrntcInfo)                                     # 为"信息菜单"菜单添加"光敏热敏"菜单项

        menuHost = mesMenu                                                   
        menuHost.parseMenu()                                                # 显示菜单

        while(1):
            
            if input_bit == 0:  
                pass
            elif input_bit == 1:                           # ec11左旋转，切换上一项
                menuHost.goNextMenuInfo()
            elif input_bit == 2:                           # ec11右旋转，切换下一项
                menuHost.goPreMenuInfo()
            elif input_bit == 7:                           # 板载key2按下，返回上一级菜单
                menuHost = menuHost.goPreMenuItem()
            elif input_bit == 5:                           # ec11按键按下，进入下一级菜单
                menuHost = menuHost.goNextMenuItem()

            with ec11Lock:
                input_bit = 0
            
            time.sleep(0.002)

    except Exception as e:
        
        print("exit...：", e)
        
    finally:

        # oled清屏
        disp.fill(0)
        disp.show() 

if __name__ == "__main__":  
    main()