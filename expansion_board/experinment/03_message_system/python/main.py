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

from gps import GPS
from mpu6050 import MPU6050
from config import ConfigManager

# Menu菜单框架
class MenuInfo:
    def __init__(self, name, nextMenuItem, action):
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
        self.title = title
        self.menuInfo = []
        self.totalPage = 0
        self.currentPage = 0

        self.preMenuItem = None

    # 添加menuInfo
    def addMenuInfo(self, menuInfo:MenuInfo):
        if not self.menuInfo:
            self.currentPage = 1
        self.menuInfo.append(menuInfo)
        self.totalPage = len(self.menuInfo)

    # 切换到上一级menuItem
    def goPreMenuItem(self):
        if self.preMenuItem != None:
            self.preMenuItem.parseMenu()
            return self.preMenuItem
        self.parseMenu()
        return self
            
    # 切换到下一级menuItem
    def goNextMenuItem(self):
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
        if self.currentPage != 1:
            self.currentPage -= 1
        self.parseMenu()

    # 切换到下一个menuInfo
    def goNextMenuInfo(self):
        if self.currentPage != self.totalPage:
            self.currentPage = self.currentPage + 1
        self.parseMenu()

    # 解析menuItem
    def parseMenu(self):
        # 判断当前menuInfo是否是第一个
        isPrePage = self.currentPage > 1
        isNextPage = self.currentPage < self.totalPage

        # oled显示当前menuInfo信息
        self.__menuDisplay(self.menuInfo[self.currentPage - 1].getName(), self.title, self.currentPage, self.totalPage, isPrePage, isNextPage)
        
    def __menuDisplay(self, itemName:str, preItemName:str, currentPage:int, totalPage:int, isPrePage:bool, isNextPage:bool):

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

###################################################################

# 通过配置文件获取外设信息
config_file = '../configuration.json' 
config_manager = ConfigManager(config_file)  
if config_manager.inspect_current_environment() is not True:  
    exit()

# oled初始化
oled_config = config_manager.get_board_config("oled")           # 从配置文件中获取oled相关配置信息 
if oled_config is None:  
    print("can not find 'oled' key in ", config_file)
    exit()

bus_number = oled_config['bus']                                 # 获取i2c总线编号
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")

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
#-----------------------------------------------------------------

# EC11初始化
ec11_config = config_manager.get_board_config("ec11")           # 从配置文件中获取ec11相关配置信息 
if ec11_config is None:  
    print("can not find 'ec11' key in ", config_file)
    exit()

ec11_SW_event = ec11_config['sw-event']
ec11_A_event  = ec11_config['a-event']
ec11_B_event  = ec11_config['b-event']

ec11_SW_value = 0
ec11_A_value  = 1
ec11_B_value  = 1

ec11_direction = 0      # 0:不动作 1:顺时针旋转 2:逆时针旋转 3:按键按下顺时针旋转 4:按键按下逆时针旋转 5:按键按下 7:板载key2按键按下
ec11Lock = threading.Lock() 

def ec11_scan_SW(device):
    global ec11_SW_value, ec11_A_value, ec11_B_value, ec11_direction
    for event in device.read_loop():
        if event.code == 4:
            ec11_SW_value = event.value
            if(ec11_SW_value == 1 and ec11_A_value == 1 and ec11_B_value == 1):     # 按键按下
                with ec11Lock:
                    ec11_direction = 5

def ec11_scan_A(device):
    global ec11_SW_value, ec11_A_value, ec11_B_value, ec11_direction
    for event in device.read_loop():
        if event.code == 250:
            if event.value == 0 and ec11_B_value == 1:  # 顺时针旋转
                ec11_A_value = 0            
                if ec11_SW_value == 1:                  # 按键按下顺时针旋转
                    with ec11Lock:
                        ec11_direction = 3
                else:
                    with ec11Lock:
                        ec11_direction = 1
            elif event.value == 1:
                ec11_A_value = 1

def ec11_scan_B(device):
    global ec11_SW_value, ec11_A_value, ec11_B_value, ec11_direction
    for event in device.read_loop():
        if event.code == 251:
            if event.value == 0 and ec11_A_value == 1:  # 逆时针旋转
                ec11_B_value = 0
                if ec11_SW_value == 1:                  # 按键按下逆时针旋转
                    with ec11Lock:
                        ec11_direction = 4
                else:
                    with ec11Lock:
                        ec11_direction = 2
            elif event.value == 1:
                ec11_B_value = 1

ec11_SW_device = evdev.InputDevice(ec11_SW_event)
ec11_A_device  = evdev.InputDevice(ec11_A_event)
ec11_B_device  = evdev.InputDevice(ec11_B_event)

ec11_SW_thread = threading.Thread(target=ec11_scan_SW, args=(ec11_SW_device,))
ec11_A_thread  = threading.Thread(target=ec11_scan_A, args=(ec11_A_device,))
ec11_B_thread  = threading.Thread(target=ec11_scan_B, args=(ec11_B_device,))

ec11_SW_thread.daemon = True
ec11_A_thread.daemon  = True
ec11_B_thread.daemon  = True

ec11_SW_thread.start()
ec11_A_thread.start()
ec11_B_thread.start()
###################################################################

# 板载按键初始化
def key2_scan(device):
    global ec11_direction
    for event in device.read_loop():
        if event.code == 2 and event.value == 1:
            with ec11Lock:
                ec11_direction = 7

key_config = config_manager.get_board_config("key")             # 从配置文件中获取按键相关配置信息 
if key_config is None:  
    print("can not find 'key' key in ", config_file)
    exit()

key_event = key_config['event']                                 # 获取按键的输入事件

key2_device = evdev.InputDevice(key_event)
key2_thread = threading.Thread(target=key2_scan, args=(key2_device,))
key2_thread.daemon = True
key2_thread.start()
###################################################################

# 超声波初始化
hcsr04_config = config_manager.get_board_config("hcsr04")       # 从配置文件中获取超声波模块相关配置信息 
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

def ultrasonicInfoFun():

    timeout = 0.2
    titleStr = "超声波"

    while True:

        if ec11_direction == 7:                           
            break

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

        distance = (end_time - start_time) * 34300 / 2
        distStr = "Dist:" + str("%.2f"%distance) + "cm"

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示数据
        fontSize = 14
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        distStrWidth = draw.textlength(distStr, font=font) 
        x = (image.width - distStrWidth) // 2 
        top = ((image.height - fontSize) // 2)
        draw.text((x, top), distStr, font=font, fill=255)

        # 显示标题
        fontSize = 10
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.002)
###################################################################

# MPU6050初始化
mpu6050_config = config_manager.get_board_config("mpu6050")     # 从配置文件中获取mpu6050相关配置信息 
if mpu6050_config is None:  
    print("can not find 'mpu6050' key in ", config_file)
    exit()

bus_number = mpu6050_config['bus']                              # 获取i2c总线编号
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")

i2c = busio.I2C(scl_pin, sda_pin)
mpu6050 = MPU6050(i2c)
mpu6050.start()

def mpu6050AccelInfoFun():

    titleStr = "加速度"

    while True:

        if ec11_direction == 7:                           
            break
        
        accelX, accelY, accelZ = mpu6050.readAcc()
        accelXStr = "X: " + str("%.2f"%accelX) + " m/s^2"
        accelYStr = "Y: " + str("%.2f"%accelY) + " m/s^2"
        accelZStr = "Z: " + str("%.2f"%accelZ) + " m/s^2"

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示内容
        fontSize = 12
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        draw.text((5, 16), accelXStr, font=font, fill=255)
        draw.text((5, 32), accelYStr, font=font, fill=255)
        draw.text((5, 48), accelZStr, font=font, fill=255)

        # 显示标题
        fontSize = 10
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.002)

def mpu6050GyroInfoFun():

    titleStr = "陀螺仪"

    while True:

        if ec11_direction == 7:                           
            break
        
        gyroX, gyroY, gyroZ = mpu6050.readGyro()
        gyroXStr = "X: " + str("%.2f"%gyroX) + " deg/s"
        gyroYStr = "Y: " + str("%.2f"%gyroY) + " deg/s"
        gyroZStr = "Z: " + str("%.2f"%gyroZ) + " deg/s"

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示内容
        fontSize = 12
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        draw.text((5, 16), gyroXStr, font=font, fill=255)
        draw.text((5, 32), gyroYStr, font=font, fill=255)
        draw.text((5, 48), gyroZStr, font=font, fill=255)

        # 显示标题
        fontSize = 10
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.002)
###################################################################

# GPS初始化
gps_config = config_manager.get_board_config("atgm332d")        # 从配置文件中获取gps atgm332d相关配置信息 
if gps_config is None:  
    print("can not find 'atgm332d' key in ", config_file)
    exit()

gps_uart_dev = gps_config['tty-dev']
gps = GPS(gps_uart_dev, 9600)

def gpsPosInfoFun():

    titleStr = "经纬度"
    oldLat = oldLog = ""

    while True:

        if ec11_direction == 7:                           
            break
        
        lat, log, bjtime = gps.read()

        if lat == "0" or log == "0":
            lat = oldLat
            log = oldLog
        else:
            oldLat = lat
            oldLog = log

        latStr = "lat:" + lat
        logStr = "log:" + log

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)

        # 显示内容
        fontSize = 16
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        latStrWidth = draw.textlength(latStr, font=font) 
        x = (image.width - latStrWidth) // 2 
        draw.text((x, 16), latStr, font=font, fill=255)

        logStrWidth = draw.textlength(logStr, font=font) 
        x = (image.width - logStrWidth) // 2 
        draw.text((x, 32), logStr, font=font, fill=255)

        # 显示标题
        fontSize = 10
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")

        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)

        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.1)

def gpsTimeInfoFun():

    titleStr = "北京时间"

    oldbjtime = "Nil-Nil-Nil,Nil:Nil"

    while True:
        
        if ec11_direction == 7:                           
            break
        
        lat, log, bjtime = gps.read()
        if oldbjtime == bjtime:
            time.sleep(0.1)
            continue
        
        if bjtime == "":
            bjtime = oldbjtime
        else:
            oldbjtime = bjtime

        draw.rectangle((1, 1, width-1, height-1), outline=2, fill=0)
        
        data = bjtime.split(',')

        # 显示内容
        fontSize = 16
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")
        
        bjtimeStrWidth = draw.textlength(data[0], font=font) 
        x = (image.width - bjtimeStrWidth) // 2 
        draw.text((x, 16), data[0], font=font, fill=255)

        dataStrWidth = draw.textlength(data[1], font=font)
        x = (image.width - dataStrWidth) // 2 
        draw.text((x, 32), data[1], font=font, fill=255)
        
        # 显示标题
        fontSize = 10
        font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", fontSize, encoding="unic")
        
        titleStrWidth = draw.textlength(titleStr, font=font) 
        x = (image.width - titleStrWidth) // 2 
        top = 2
        draw.text((x, top), titleStr, font=font, fill=255)
        
        # 刷新显示
        disp.image(image)  
        disp.show() 

        time.sleep(0.1)
###################################################################

def main():

    try:
        global ec11_direction

        # GPS子菜单
        GPSMenu = MenuItem("GPS")
        posInfo = MenuInfo("经纬度", None, gpsPosInfoFun)
        mesInfo = MenuInfo("北京时间", None, gpsTimeInfoFun)
        GPSMenu.addMenuInfo(posInfo)
        GPSMenu.addMenuInfo(mesInfo)

        # MPU6050子菜单
        MPU6050Menu = MenuItem("MPU6050")
        accelInfo = MenuInfo("加速度", None, mpu6050AccelInfoFun)
        gyroInfo = MenuInfo("陀螺仪", None, mpu6050GyroInfoFun)
        MPU6050Menu.addMenuInfo(accelInfo)
        MPU6050Menu.addMenuInfo(gyroInfo)

        # 总菜单
        mesMenu = MenuItem("信息菜单")
        ultrasonicInfo = MenuInfo("超声波", None, ultrasonicInfoFun)
        MPU6050Info = MenuInfo("MPU6050", MPU6050Menu, None)
        GPSInfo = MenuInfo("GPS", GPSMenu, None)
        mesMenu.addMenuInfo(ultrasonicInfo)
        mesMenu.addMenuInfo(MPU6050Info)
        mesMenu.addMenuInfo(GPSInfo)

        menuHost = mesMenu
        menuHost.parseMenu()

        
        while(1):
            
            if ec11_direction == 0:  
                pass
            elif ec11_direction == 1:                           # ec11左旋转，切换上一项
                menuHost.goNextMenuInfo()
            elif ec11_direction == 2:                           # ec11右旋转，切换下一项
                menuHost.goPreMenuInfo()
            elif ec11_direction == 7:                           # 板载key2按下，返回上一级菜单
                menuHost = menuHost.goPreMenuItem()
            elif ec11_direction == 5:                           # ec11按键按下，进入下一级菜单
                menuHost = menuHost.goNextMenuItem()

            with ec11Lock:
                ec11_direction = 0
            
            time.sleep(0.002)

    except Exception as e:
        
        print("exit...：", e)
        
    finally:
        
        gps.close()
        mpu6050.close()

        # oled清屏
        disp.fill(0)
        disp.show() 

if __name__ == "__main__":  
    main()