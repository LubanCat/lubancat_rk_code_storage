###############################################
#
#  file: main.py
#  update: 2024-08-23
#  usage: 
#      sudo python main.py
#
###############################################

import time
import gpiod
from periphery import PWM
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
import adafruit_ads1x15.ads1115 as ADS
from adafruit_ads1x15.analog_in import AnalogIn
from bmp280 import Bmx280Spi, MODE_NORMAL
from config import ConfigManager

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
x = 5   
top = 5  
#-----------------------------------------------------------------

# ADS1115初始化
ads1115_config = config_manager.get_board_config("ads1115")     # 从配置文件中获取ads1115相关配置信息 
if ads1115_config is None:  
    print("can not find 'ads1115' key in ", config_file)
    exit()

bus_number = ads1115_config['bus']                              # 获取i2c总线编号
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")

i2c_ads1115 = busio.I2C(scl_pin, sda_pin)
ads = ADS.ADS1115(i2c_ads1115)
#-----------------------------------------------------------------

# SG90舵机初始化
sg90_config = config_manager.get_board_config("sg90")           # 从配置文件中获取sg90舵机相关配置信息 
if sg90_config is None:  
    print("can not find 'sg90' key in ", config_file)
    exit()

pwm_chip = sg90_config['pwm_chip']                              # 获取pwmchip

pwm2 = PWM(pwm_chip, 0)             # pwmchip, channel
pwm2.frequency = 50                 # 频率 50Hz
pwm2.duty_cycle = 0.025             # 占空比（%），范围：0.0-1.0
pwm2.enable()
#-----------------------------------------------------------------

# BMP280初始化
bmp280_config = config_manager.get_board_config("bmp280")       # 从配置文件中获取bmp280相关配置信息 
if bmp280_config is None:  
    print("can not find 'bmp280' key in ", config_file)
    exit()

bus_number = bmp280_config['bus']                               # 获取SPI总线编号
cs_gpiochip = bmp280_config['cs_chip']                          # 获取cs脚的gpiochip
cs_gpionum = bmp280_config['cs_pin']                            # 获取cs脚的gpionum

bmx = Bmx280Spi(spiBus=bus_number, cs_chip=cs_gpiochip, cs_pin=cs_gpionum)
bmx.set_power_mode(MODE_NORMAL)
bmx.set_sleep_duration_value(3)
bmx.set_temp_oversample(1)
bmx.set_pressure_oversample(1)
bmx.set_filter(0)
#-----------------------------------------------------------------

# 按键初始化
stop_event = threading.Event()                                  # 按键线程，控制遮阳帘（舵机）角度
def read_keyboard_events(device):
    for event in device.read_loop():
        if event.code == 11 and event.value == 1:
            print("turn s90 motor to 0 degree")
            pwm2.duty_cycle = 0.025
        elif event.code == 2 and event.value == 1:
            print("turn s90 motor to 45 degree")
            pwm2.duty_cycle = 0.05
        elif event.code == 3 and event.value == 1:
            print("turn s90 motor to 90 degree")
            pwm2.duty_cycle = 0.075

key_config = config_manager.get_board_config("key")             # 从配置文件中获取按键相关配置信息 
if key_config is None:  
    print("can not find 'key' key in ", config_file)
    exit()

key_event = key_config['event']                                 # 获取按键的输入事件

device = evdev.InputDevice(key_event)
thread_key = threading.Thread(target=read_keyboard_events, args=(device,))
thread_key.daemon = True
thread_key.start()
#-----------------------------------------------------------------

# hcsr501人体红外初始化
hcsr501_config = config_manager.get_board_config("hcsr501")     # 从配置文件中获取人体红外模块相关配置信息 
if hcsr501_config is None:  
    print("can not find 'hcsr501' key in ", config_file)
    exit()

hcsr501_chip = hcsr501_config['dout_chip']
hcsr501_pin = hcsr501_config['dout_pin']

gpiochip_infrared = gpiod.Chip(hcsr501_chip, gpiod.Chip.OPEN_BY_NUMBER)
gpio_infrared = gpiochip_infrared.get_line(hcsr501_pin)
gpio_infrared.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_IN)
#-----------------------------------------------------------------

# MQ135（测量甲苯浓度）
a = 5.06                                                        # a b为甲苯检测中的校准常数
b = 2.46
vrl_clean = 0.576028                                            # 洁净空气下的平均Vrl电压值（就是adc读到的平均电压值）
def mq135_cal_ppm(vrl_gass, vrl_clean, a, b):
    ratio = vrl_gass / vrl_clean                                # 利用实际测量时的Vrl与洁净空气下的Vrl来代替Rs/R0
    ppm = a * (ratio ** b)                                      # ppm = a*(Rs/R0)^b
    return ppm
#-----------------------------------------------------------------

def main():

    try:

        while True:
            
            draw.rectangle((0, 0, width, height), outline=0, fill=0)

            # 获取ad电压值，计算ppm
            ad = AnalogIn(ads, ADS.P0)
            ppm = mq135_cal_ppm(ad.voltage, vrl_clean, a, b)
            ppm_text = f"{ppm:.2f}"

            # 获取大气压值
            pressure = bmx.update_readings().pressure
            pressure_text = round(pressure / 1000, 2)

            # 人体红外
            if gpio_infrared.get_value() == 1:
                slogan_text = "欢迎您来"
            else:
                slogan_text = ""

            width_slogan = draw.textlength(slogan_text, font=font)  
            x = (image.width - width_slogan) // 2  
            draw.text((x, top + 0), slogan_text, font=font, fill=255)  
            draw.text((0, top + 16), "ppm : " + str(ppm_text), font=font, fill=255)
            draw.text((0, top + 32), "prs : " + str(pressure_text) + "KPa", font=font, fill=255)

            # oled显示图像
            disp.image(image)  
            disp.show() 
            
            time.sleep(1)

    except Exception as e:
        
        print("exit...：", e)
        
    finally:
        
        # 复位舵机至0度
        pwm2.duty_cycle = 0.025
        pwm2.close()

        # 释放gpio
        gpio_infrared.release()

        # oled清屏
        disp.fill(0)
        disp.show() 

if __name__ == "__main__":  
    main()