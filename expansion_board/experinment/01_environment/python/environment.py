###############################################
#
#  file: environment.py
#  update: 2024-08-09
#  usage: 
#      sudo python environment.py
#
###############################################

import time
import gpiod
from periphery import PWM
import os
import struct
import subprocess
import board
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306

# led 
# led gpionum
gpionum_r_led = 0
gpionum_g_led = 1
# led gpiochip
gpiochip_led = "6"

# buzzer 
# buzzer gpionum
gpionum_buzzer = 6
# buzzer gpiochip
gpiochip_buzzer = "6"

# motor controller
# motor controller gpionum
#
# A-D : 0-4
# number = group * 8 + x
# e.g. : B0 = 1 * 8 + 0 = 8
#	     C4 = 2 * 8 + 4 = 20 
#
gpionum_motor_STBY = 8          # GPIO1_B0
gpionum_motor_AIN1 = 20         # GPIO4_C4
gpionum_motor_AIN2 = 8          # GPIO3_B0
gpionum_motor_BIN1 = 9          # GPIO1_B1
gpionum_motor_BIN2 = 10         # GPIO1_B2
# motor controller gpiochip
gpiochip_motor_STBY = "1"       # gpiochip1
gpiochip_motor_AIN1 = "4"       # gpiochip4
gpiochip_motor_AIN2 = "3"       # gpiochip3
gpiochip_motor_BIN1 = "1"       # gpiochip1
gpiochip_motor_BIN2 = "1"       # gpiochip1
# motor controller pwmchip, pwm channel
motor_PWMA = PWM(1, 0)          # pwmchip1, channel0
motor_PWMB = PWM(2, 0)          # pwmchip2, channel0

# 温湿度阈值
MAX_TEMPERATURE = 35    
MAX_HUMIDITY = 60       

class GPIO:
    global gpio, gpiochip

    def __init__(self, gpionum, gpiochipx, val):
        self.gpiochip = gpiod.Chip(gpiochipx, gpiod.Chip.OPEN_BY_NUMBER)
        self.gpio = self.gpiochip.get_line(gpionum)
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[val])
    
    def set(self, val):
        self.gpio.set_value(val)

    def release(self):
        self.gpio.release()

# motor init
motor_STBY = GPIO(gpionum_motor_STBY, gpiochip_motor_STBY, 0)   # 初始化电机驱动板STBY引脚, 初始电平为低电平，驱动板不工作
motor_AIN1 = GPIO(gpionum_motor_AIN1, gpiochip_motor_AIN1, 0)   # 初始化电机驱动板AIN1引脚, 初始电平为低电平
motor_AIN2 = GPIO(gpionum_motor_AIN2, gpiochip_motor_AIN2, 1)   # 初始化电机驱动板AIN2引脚, 初始电平为高电平
motor_BIN1 = GPIO(gpionum_motor_BIN1, gpiochip_motor_BIN1, 0)   # 初始化电机驱动板BIN1引脚, 初始电平为低电平
motor_BIN2 = GPIO(gpionum_motor_BIN2, gpiochip_motor_BIN2, 1)   # 初始化电机驱动板BIN2引脚, 初始电平为高电平

motor_PWMA.frequency = 1e3          # 频率 1kHz
motor_PWMA.duty_cycle = 0.4         # 占空比（%），范围：0.0-1.0
motor_PWMA.polarity = "normal"      # 极性
motor_PWMA.enable()                 # 使能

motor_PWMB.frequency = 1e3
motor_PWMB.duty_cycle = 0.4
motor_PWMB.polarity = "normal"
motor_PWMB.enable()
#-----------------------------------------------------------------

# led init
red_led = GPIO(gpionum_r_led, gpiochip_led, 1)          # 初始化红色led, 初始电平为高电平，灯灭
green_led = GPIO(gpionum_g_led, gpiochip_led, 1)        # 初始化绿色led, 初始电平为高电平，灯灭
#-----------------------------------------------------------------

# buzzer init
buzzer = GPIO(gpionum_buzzer, gpiochip_buzzer, 0)       # 初始化蜂鸣器, 初始电平为低电平，蜂鸣器不鸣响
#-----------------------------------------------------------------

# oled init
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

# dht11 init
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
            print("temp: " + str(temp) + " humi: " + str(humi))

            # 环境温湿度超过阈值
            if (temp >= MAX_TEMPERATURE or humi >= MAX_HUMIDITY):   
                red_led.set(0)          # 红灯亮
                green_led.set(1)        # 绿灯灭
                buzzer.set(1)           # 蜂鸣器鸣响
                motor_STBY.set(1)       # 风扇开启
            else:
                red_led.set(1)          # 红灯灭
                green_led.set(0)        # 绿灯亮
                buzzer.set(0)           # 蜂鸣器关闭
                motor_STBY.set(0)       # 风扇关闭

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
        
        # led off
        red_led.set(1)
        green_led.set(1)
        
        # buzzer off
        buzzer.set(0)

        # motor off
        motor_STBY.set(0)
        
        # gpio release
        green_led.release()
        green_led.release()
        buzzer.release()
        motor_STBY.release()
        motor_AIN1.release()
        motor_AIN2.release()
        motor_BIN1.release()
        motor_BIN2.release()

        # pwm close
        motor_PWMA.close()
        motor_PWMB.close()

        # disp clear
        disp.fill(0)
        disp.show() 

        # dht11 close
        os.close(dht11_fd)
        
if __name__ == "__main__":  
    main()