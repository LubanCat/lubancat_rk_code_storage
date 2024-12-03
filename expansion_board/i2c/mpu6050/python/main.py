###############################################
#
#  file: mpu6050.py
#  update: 2024-10-22
#  usage: 
#      sudo python mpu6050.py
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
from mpu6050 import MPU6050

# oled初始化
oled_init_flag = 1
try:
    oled_i2c = busio.I2C(board.I2C3_SCL, board.I2C3_SDA)
    disp = adafruit_ssd1306.SSD1306_I2C(128, 64, oled_i2c)

    width = disp.width  
    height = disp.height  
    image = Image.new('1', (width, height))  
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, width, height), outline=0, fill=0)

    font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 12, encoding="unic")  
    x = 5   
    top = 5  
except Exception as e:
    oled_init_flag = 0
#-----------------------------------------------------------------

# MPU6050初始化
mpu6050_i2c = busio.I2C(board.I2C5_SCL, board.I2C5_SDA)
mpu6050 = MPU6050(mpu6050_i2c)
mpu6050.start()
###################################################################

def main():

    try:

        while(1):
            
            accelX, accelY, accelZ = mpu6050.readAcc()      # 读取加速度数据
            gyroX, gyroY, gyroZ = mpu6050.readGyro()        # 读取陀螺仪数据

            accelXStr = str("%.2f"%accelX)                  
            accelYStr = str("%.2f"%accelY)
            accelZStr = str("%.2f"%accelZ)

            gyroXStr = str("%.2f"%gyroX)
            gyroYStr = str("%.2f"%gyroY)
            gyroZStr = str("%.2f"%gyroZ)

            if oled_init_flag != 0:
                draw.rectangle((1, 1, width-1, height-1), outline=1, fill=0)

                # 绘制加速度数据文本
                draw.text((5, 0), "accel", font=font, fill=255)
                draw.text((5, 16), accelXStr, font=font, fill=255)
                draw.text((5, 32), accelYStr, font=font, fill=255)
                draw.text((5, 45), accelZStr, font=font, fill=255)

                # 绘制陀螺仪数据文本
                draw.text((40, 0), "gyro", font=font, fill=255)
                draw.text((40, 16), gyroXStr, font=font, fill=255)
                draw.text((40, 32), gyroYStr, font=font, fill=255)
                draw.text((40, 45), gyroZStr, font=font, fill=255)

                # 刷新显示
                disp.image(image)  
                disp.show() 

            print("accel:", "x:", accelXStr, "y:", accelYStr, "z:", accelZStr, "m/s^2")
            print("gyro:", "x:", gyroXStr, "y:", gyroYStr, "z:", gyroZStr, "deg/s")
            print("\n")

            time.sleep(0.3)

    except Exception as e:
        
        print("exit...：", e)
        
    finally:
    
        mpu6050.close()

        if oled_init_flag != 0:
            # oled清屏
            disp.fill(0)
            disp.show() 

if __name__ == "__main__":  
    main()