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
from config import ConfigManager 

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

bus_number = oled_config['bus']                                 # 获取键值bus
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")

i2c = busio.I2C(scl_pin, sda_pin)
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

# 电机驱动板初始化
motor_config = config_manager.get_board_config("motor-driver-board")    # 从配置文件中获取电机驱动板相关配置信息  
if motor_config is None:  
    print("can not find 'motor-driver-board' key in ", config_file)
    exit()

gpionum_motor_STBY = motor_config['stby_pin_num']               # 获取电机驱动板引脚信息
gpionum_motor_AIN1 = motor_config['ain1_pin_num']          
gpionum_motor_AIN2 = motor_config['ain2_pin_num']           
gpionum_motor_BIN1 = motor_config['bin1_pin_num']          
gpionum_motor_BIN2 = motor_config['bin2_pin_num']          

gpiochip_motor_STBY = motor_config['stby_pin_chip']       
gpiochip_motor_AIN1 = motor_config['ain1_pin_chip']       
gpiochip_motor_AIN2 = motor_config['ain2_pin_chip']       
gpiochip_motor_BIN1 = motor_config['bin1_pin_chip']       
gpiochip_motor_BIN2 = motor_config['bin2_pin_chip']       

motor_STBY = GPIO(gpionum_motor_STBY, gpiochip_motor_STBY, 0)   # 初始化电机驱动板STBY引脚, 初始电平为低电平，驱动板不工作
motor_AIN1 = GPIO(gpionum_motor_AIN1, gpiochip_motor_AIN1, 0)   # 初始化电机驱动板AIN1引脚, 初始电平为低电平
motor_AIN2 = GPIO(gpionum_motor_AIN2, gpiochip_motor_AIN2, 1)   # 初始化电机驱动板AIN2引脚, 初始电平为高电平
motor_BIN1 = GPIO(gpionum_motor_BIN1, gpiochip_motor_BIN1, 0)   # 初始化电机驱动板BIN1引脚, 初始电平为低电平
motor_BIN2 = GPIO(gpionum_motor_BIN2, gpiochip_motor_BIN2, 1)   # 初始化电机驱动板BIN2引脚, 初始电平为高电平

pwma_chip = motor_config['pwma_chip']                           # 获取电机驱动板pwm信息
pwmb_chip = motor_config['pwmb_chip']

motor_PWMA = PWM(pwma_chip, 0)                                  # 默认使用channel0
motor_PWMB = PWM(pwmb_chip, 0)                                  # 默认使用channel0

motor_PWMA.frequency = 1e3                                      # 频率 1kHz
motor_PWMA.duty_cycle = 0.4                                     # 占空比（%），范围：0.0-1.0
motor_PWMA.polarity = "normal"                                  # 极性
motor_PWMA.enable()                                             # 使能

motor_PWMB.frequency = 1e3
motor_PWMB.duty_cycle = 0.4
motor_PWMB.polarity = "normal"
motor_PWMB.enable()
#-----------------------------------------------------------------

# led初始化
led_config = config_manager.get_board_config("led")             # 从配置文件中获取led相关配置信息 
if led_config is None:  
    print("can not find 'led' key in ", config_file)
    exit()

r_led_chip = led_config['r_pin_chip']
g_led_chip = led_config['g_pin_chip']

r_led_num = led_config['r_pin_num']
g_led_num = led_config['g_pin_num']

red_led = GPIO(r_led_num, r_led_chip, 1)                        # 初始化红色led, 初始电平为高电平，灯灭
green_led = GPIO(g_led_num, g_led_chip, 1)                      # 初始化绿色led, 初始电平为高电平，灯灭
#-----------------------------------------------------------------

# 蜂鸣器初始化
buzzer_config = config_manager.get_board_config("buzzer")       # 从配置文件中获取buzzer相关配置信息 
if buzzer_config is None:  
    print("can not find 'buzzer' key in ", config_file)
    exit()

buzzer_chip = buzzer_config['pin_chip']
buzzer_num = buzzer_config['pin_num']

buzzer = GPIO(buzzer_num, buzzer_chip, 0)                       # 初始化蜂鸣器, 初始电平为低电平，蜂鸣器不鸣响
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