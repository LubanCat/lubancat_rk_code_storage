###############################################
#
#  file: main.py
#  update: 2024-11-11
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
import board
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306
import evdev
import threading
from config import ConfigManager 

# 全局变量定义
gkey_value = 0                      # 按键输入的状态值
key_lock = threading.Lock()         # 按键值的线程锁

gtemp = 0                           # 温度值
ghumi = 0                           # 湿度值
dht11_lock = threading.Lock()       # DHT11数据的线程锁

class GPIO:
    """
    GPIO控制类：用于设置GPIO的输入输出模式和操作
    """
    global gpio, gpiochip

    def __init__(self, gpionum, gpiochipx):
        """
        初始化GPIO对象，打开指定的GPIO芯片和引脚
        :param gpionum: GPIO引脚号
        :param gpiochipx: GPIO芯片编号
        """
        self.gpiochip = gpiod.Chip(gpiochipx, gpiod.Chip.OPEN_BY_NUMBER)
        self.gpio = self.gpiochip.get_line(gpionum)

    def set_direction_out(self, val):
        """
        将引脚设置为输出，并设定初始值
        :param val: 输出值（0或1）
        """
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[val])
    
    def set_direction_in(self):
        """
        将引脚设置为输入
        """
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_IN)

    def set(self, val):
        """
        设置引脚的输出值
        :param val: 输出值（0或1）
        """
        self.gpio.set_value(val)

    def get(self):
        """
        获取引脚的输入值
        :return: 输入值（0或1）
        """
        return self.gpio.get_value()

    def release(self):
        """
        释放GPIO引脚资源
        """
        self.gpio.release()

def key1_scan_thread(device):
    """
    扫描按键输入事件，根据事件更新按键值
    :param device: 按键输入事件设备
    """
    global gkey_value

    for event in device.read_loop():

        if event.code == 11 and event.value == 1:

            with key_lock:

                gkey_value = 1

def dht11_thread():
    """
    DHT11传感器读取线程，读取温湿度数据并显示在OLED屏幕上
    """
    global gtemp, ghumi

    while True:

        draw.rectangle((0, 0, width, height), outline=0, fill=0)

        # 获取温湿度值并解析
        data = os.read(dht11_fd, 6)
        temp = float(data[2] + data[3] * 0.01)
        humi = float(data[0] + data[1] * 0.01)
        
        with dht11_lock:
            gtemp = temp
            ghumi = humi

        # print("temp: " + str(temp) + " humi: " + str(humi))

        # 绘制新文本
        draw.text((5, 16), "temp: " + str(temp), font=font, fill=255)
        draw.text((5, 32), "humi: " + str(humi), font=font, fill=255)

        # oled显示图像
        disp.image(image)  
        disp.show() 
        
        time.sleep(1)

def ir_thread():
    """
    红外检测线程，当检测到信号时控制舵机角度
    """
    while True:

        ir_value = IR.get()

        if ir_value == 0:

            print("sg90 turn to 45 degree!\n")
            sg90_PWM.duty_cycle = 0.05
            time.sleep(2)

            print("sg90 turn to 0 degree!\n")
            sg90_PWM.duty_cycle = 0.025
            time.sleep(2)
        
        time.sleep(0.1)

# 通过配置文件获取外设信息
config_file = '../configuration.json' 
config_manager = ConfigManager(config_file)  
if config_manager.inspect_current_environment() is not True:  
    exit()


# OLED初始化
oled_config = config_manager.get_board_config("oled")               # 从配置文件中获取oled相关配置信息 
if oled_config is None:  
    print("can not find 'oled' key in ", config_file)
    exit()

bus_number = oled_config['bus']                                     # 获取键值bus
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
font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 16, encoding="unic")  


# 板载按键初始化
key_config = config_manager.get_board_config("key")                 # 从配置文件中获取按键相关配置信息 
if key_config is None:  
    print("can not find 'key' key in ", config_file)
    exit()

key_event = key_config['event']                                     # 获取按键的输入事件
key1_device = evdev.InputDevice(key_event)

key1_obj = threading.Thread(target=key1_scan_thread, args=(key1_device,))
key1_obj.daemon = True


# DHT11初始化
dht11_fd = os.open('/dev/dht11', os.O_RDONLY)

dht11_obj = threading.Thread(target=dht11_thread)
dht11_obj.daemon = True


# 电机驱动板初始化
motor_config = config_manager.get_board_config("motor-driver-board")    # 从配置文件中获取电机驱动板相关配置信息  
if motor_config is None:  
    print("can not find 'motor-driver-board' key in ", config_file)
    exit()

gpionum_motor_STBY = motor_config['stby_pin_num']                       # 获取电机驱动板引脚信息
gpionum_motor_AIN1 = motor_config['ain1_pin_num']          
gpionum_motor_AIN2 = motor_config['ain2_pin_num']           
gpionum_motor_BIN1 = motor_config['bin1_pin_num']          
gpionum_motor_BIN2 = motor_config['bin2_pin_num']          

gpiochip_motor_STBY = motor_config['stby_pin_chip']       
gpiochip_motor_AIN1 = motor_config['ain1_pin_chip']       
gpiochip_motor_AIN2 = motor_config['ain2_pin_chip']       
gpiochip_motor_BIN1 = motor_config['bin1_pin_chip']       
gpiochip_motor_BIN2 = motor_config['bin2_pin_chip']       

motor_STBY = GPIO(gpionum_motor_STBY, gpiochip_motor_STBY)      # 初始化电机驱动板STBY引脚
motor_AIN1 = GPIO(gpionum_motor_AIN1, gpiochip_motor_AIN1)      # 初始化电机驱动板AIN1引脚
motor_AIN2 = GPIO(gpionum_motor_AIN2, gpiochip_motor_AIN2)      # 初始化电机驱动板AIN2引脚
motor_BIN1 = GPIO(gpionum_motor_BIN1, gpiochip_motor_BIN1)      # 初始化电机驱动板BIN1引脚
motor_BIN2 = GPIO(gpionum_motor_BIN2, gpiochip_motor_BIN2)      # 初始化电机驱动板BIN2引脚

motor_STBY.set_direction_out(0)
motor_AIN1.set_direction_out(0)
motor_AIN2.set_direction_out(1)
motor_BIN1.set_direction_out(0)
motor_BIN2.set_direction_out(1)

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

# GPIO引脚初始化，用于红外信号检测
ir_config = config_manager.get_board_config("ir")               # 从配置文件中获取超声波模块相关配置信息 
if ir_config is None:  
    print("can not find 'ir' key in ", config_file)
    exit()

gpionum_ir = ir_config['ir_pin_num']
gpiochip_ir = ir_config['ir_pin_chip']

IR = GPIO(gpionum_ir, gpiochip_ir)
IR.set_direction_in()

ir_obj = threading.Thread(target=ir_thread)
ir_obj.daemon = True


# 舵机PWM初始化
sg90_config = config_manager.get_board_config("sg90")           # 从配置文件中获取sg90舵机相关配置信息 
if sg90_config is None:  
    print("can not find 'sg90' key in ", config_file)
    exit()

pwm_chip = sg90_config['pwm_chip']                              # 获取pwmchip

sg90_PWM = PWM(pwm_chip, 0)                                     # pwmchip, channel
sg90_PWM.frequency = 50                                         # 频率 50Hz
sg90_PWM.duty_cycle = 0.025                                     # 占空比（%），范围：0.0-1.0
sg90_PWM.enable()


# 启动线程
key1_obj.start()                                                # 启动key1线程
dht11_obj.start()                                               # 启动dht11线程
ir_obj.start()                                                  # 启动红外线程

def main():

    global gtemp, ghumi, gkey_value
    motor_sw = 0

    try:

        while True:
            
            # 获取温湿度数据
            with dht11_lock:
                temp, humi = gtemp, ghumi
            
            # 获取按键值并切换电机状态
            with key_lock:
                if gkey_value == 1:
                    motor_sw ^= gkey_value == 1
                    motor_STBY.set(1 if motor_sw else 0)
                    print("motor enable" if motor_sw else "motor disabled")
                gkey_value = 0
            
            # 根据湿度调整电机速度
            duty_cycle = 0.4 if humi > 50 else 0.3
            motor_PWMA.duty_cycle = duty_cycle
            motor_PWMB.duty_cycle = duty_cycle

            time.sleep(0.1)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:

        # 关闭DHT11
        os.close(dht11_fd)

        # 电机驱动板反初始化
        motor_STBY.set(0)
        motor_STBY.release()
        motor_AIN1.release()
        motor_AIN2.release()
        motor_BIN1.release()
        motor_BIN2.release()
        motor_PWMA.close()
        motor_PWMB.close()

        # 红外模块反初始化
        IR.release()

        # 舵机PWM反初始化
        sg90_PWM.close()

        # oled清屏
        disp.fill(0)
        disp.show() 

if __name__ == "__main__":  
    main()