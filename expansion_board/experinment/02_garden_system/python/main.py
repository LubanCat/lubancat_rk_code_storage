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

# 人体红外
gpionum_infrared = 8
gpiochip_infrared = gpiod.Chip("6", gpiod.Chip.OPEN_BY_NUMBER)
gpio_infrared = gpiochip_infrared.get_line(gpionum_infrared)
gpio_infrared.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_IN)
#-----------------------------------------------------------------

# oled
i2c_oled = busio.I2C(board.I2C3_SCL, board.I2C3_SDA)
disp = adafruit_ssd1306.SSD1306_I2C(128, 64, i2c_oled)

width = disp.width  
height = disp.height  
image = Image.new('1', (width, height))  
draw = ImageDraw.Draw(image)
draw.rectangle((0, 0, width, height), outline=0, fill=0)

font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 16, encoding="unic")  
x = 5   
top = 0  
#-----------------------------------------------------------------

# ADS1115
i2c_ads1115 = busio.I2C(board.I2C5_SCL, board.I2C5_SDA)
ads = ADS.ADS1115(i2c_ads1115)
#-----------------------------------------------------------------

# SG90舵机
pwm2 = PWM(1, 0)                    # pwmchip1, channel0
pwm2.frequency = 50                 # 频率 50Hz
pwm2.duty_cycle = 0.025             # 占空比（%），范围：0.0-1.0
pwm2.enable()
#-----------------------------------------------------------------

# MQ135（测量甲苯浓度）
a = 5.06                            # a b为甲苯检测中的校准常数
b = 2.46
vrl_clean = 0.576028                # 洁净空气下的平均Vrl电压值（就是adc读到的平均电压值）
def mq135_cal_ppm(vrl_gass, vrl_clean, a, b):
    ratio = vrl_gass / vrl_clean    # 利用实际测量时的Vrl与洁净空气下的Vrl来代替Rs/R0
    ppm = a * (ratio ** b)          # ppm = a*(Rs/R0)^b
    return ppm
#-----------------------------------------------------------------

# BMP280
bmx = Bmx280Spi(spiBus=3, cs_chip=6, cs_pin=11)
bmx.set_power_mode(MODE_NORMAL)
bmx.set_sleep_duration_value(3)
bmx.set_temp_oversample(1)
bmx.set_pressure_oversample(1)
bmx.set_filter(0)
#-----------------------------------------------------------------

# 按键线程，控制遮阳帘（舵机）角度
stop_event = threading.Event()
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

device = evdev.InputDevice('/dev/input/event7')
thread_key = threading.Thread(target=read_keyboard_events, args=(device,))
thread_key.daemon = True
thread_key.start()
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
        
        # reset the SG90
        pwm2.duty_cycle = 0.025
        pwm2.close()

        # gpio release
        gpio_infrared.release()

        # disp clear
        disp.fill(0)
        disp.show() 

if __name__ == "__main__":  
    main()