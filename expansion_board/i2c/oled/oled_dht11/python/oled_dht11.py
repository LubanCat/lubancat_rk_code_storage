###############################################
#
#  file: oled_dht11.py
#  update: 2024-10-22
#  usage: 
#      sudo python oled_dht11.py
#
###############################################

import time
import gpiod
import os
import struct
import subprocess
import board
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306

# oled初始化
i2c = busio.I2C(board.I2C3_SCL, board.I2C3_SDA)
disp = adafruit_ssd1306.SSD1306_I2C(128, 64, i2c)

width = disp.width  
height = disp.height  
image = Image.new('1', (width, height))  
draw = ImageDraw.Draw(image)
draw.rectangle((0, 0, width, height), outline=0, fill=0)

font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 16, encoding="unic")  
x = 5   
top = 5  
#-----------------------------------------------------------------

# dht11初始化
MAX_TEMPERATURE = 35    
MAX_HUMIDITY = 60
dht11_fd = os.open('/dev/dht11', os.O_RDONLY)
#-----------------------------------------------------------------

def main():
    try:
        while True:
            
            draw.rectangle((0, 0, width, height), outline=0, fill=0)

            # 获取温湿度值并解析
            data = os.read(dht11_fd, 6)
            temp = float(data[2] + data[3] * 0.01)
            humi = float(data[0] + data[1] * 0.01)
            #print("temp: " + str(temp) + " humi: " + str(humi))

            # 绘制新文本
            draw.text((x, top + 16), "temp: " + str(temp) + "℃", font=font, fill=255)
            draw.text((x, top + 32), "humi: " + str(humi) + "%", font=font, fill=255)

            # oled显示图像
            disp.image(image)  
            disp.show() 
            
            time.sleep(1)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:

        # disp clear
        disp.fill(0)
        disp.show() 

        # dht11 close
        os.close(dht11_fd)
        
if __name__ == "__main__":  
    main()