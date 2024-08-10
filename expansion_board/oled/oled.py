###############################################
#
#  file: oled.py
#  update: 2024-08-10
#  usage: 
#      sudo python oled.py
#
###############################################

import time
import subprocess

import board
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306

i2c = busio.I2C(board.I2C3_SCL, board.I2C3_SDA)
disp = adafruit_ssd1306.SSD1306_I2C(128, 64, i2c)

width = disp.width  
height = disp.height  
image = Image.new('1', (width, height))  
draw = ImageDraw.Draw(image)
draw.rectangle((0, 0, width, height), outline=0, fill=0)

font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 16, encoding="unic")
top = 0
x = 0

def main():
    try:
        while True:
            
            text = "Lubuncat"
            width = draw.textlength(text, font=font)
            x = (image.width - width) // 2  # 计算左边距离
            draw.text((x, top + 0), text, font=font, fill=255)

            text = "野火科技"
            width = draw.textlength(text, font=font)
            x = (image.width - width) // 2  # 计算左边距离
            draw.text((x, top + 16), text, font=font, fill=255)

            text = "hyw666"
            width = draw.textlength(text, font=font)
            x = (image.width - width) // 2  # 计算左边距离
            draw.text((x, top + 32), text, font=font, fill=255)

            # Display image.
            disp.image(image)
            disp.show()
            time.sleep(0.1)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        # disp clear
        disp.fill(0)
        disp.show()
        
if __name__ == "__main__":  
    main()