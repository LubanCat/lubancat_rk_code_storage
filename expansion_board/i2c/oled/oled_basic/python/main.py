###############################################
#
#  file: oled_basic.py
#  update: 2024-10-22
#  usage: 
#      sudo python oled_basic.py
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

"""绘制矩形""" 
# top_left (x, y) 起始点坐标
# bottom_right (width, height) 矩形长宽
def draw_rectangle(draw, top_left, bottom_right, outline=255, fill=0):  
    draw.rectangle([top_left, bottom_right], outline=outline, fill=fill)  

"""绘制圆形"""
# center (x, y) 圆心坐标
# radius 半径
def draw_circle(draw, center, radius, outline=255, fill=0):  
    draw.ellipse([center[0] - radius, center[1] - radius, center[0] + radius, center[1] + radius], outline=outline, fill=fill)  

"""绘制三角形""" 
# points（x, y） 、（x2, y2）、（x3, y3）三个顶点坐标
def draw_triangle(draw, points, outline=255, fill=0):  
    draw.polygon(points, outline=outline, fill=fill)

def main():
    try:
        while True:
            
            # 绘制矩形  
            draw_rectangle(draw, (5, 5), (35, 35), outline=255, fill=0)  

            # 绘制圆形  
            draw_circle(draw, (55, 20), 15, outline=255, fill=0)  
  
            # 绘制三角形  
            draw_triangle(draw, [(75, 35), (75+30, 35), (75+30/2, 5)], outline=255, fill=0)

            # 绘制文字
            text = "野火Lubuncat"
            draw.text((5, 40), text, font=font, fill=255)

            # 显示图像
            disp.image(image)
            disp.show()
            time.sleep(0.1)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        # 清屏
        disp.fill(0)
        disp.show()
        
if __name__ == "__main__":  
    main()