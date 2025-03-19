import gpiod
import board
import busio
import os
import json
import time
import hmac
import uuid
import threading
import signal
import adafruit_ads1x15.ads1115 as ADS
import paho.mqtt.client as mqtt
from hashlib import sha256
from adafruit_ads1x15.analog_in import AnalogIn

from config import ConfigManager 
from bmp280 import Bmx280Spi, MODE_NORMAL
from ringbuff import RingBuffer

# 设备信息
DeviceID = '2632d4478100fda674lxlu'         # 替换成自己的DeviceID
DeviceSecret = 'qxZS1LhriSGzzqiE'           # 替换成自己的DeviceSecret

# MQTT服务器信息
Address = 'm1.tuyacn.com'
Port = 8883
ClientID = 'tuyalink_' + DeviceID

# 认证信息
T = int(time.time())
UserName = f'{DeviceID}|signMethod=hmacSha256,timestamp={T},secureMode=1,accessType=1'
data_for_signature = f'deviceId={DeviceID},timestamp={T},secureMode=1,accessType=1'.encode('utf-8')
appsecret = DeviceSecret.encode('utf-8')
Password = hmac.new(appsecret, data_for_signature, digestmod=sha256).hexdigest()

# 发布主题
pubTopic = f'tylink/{DeviceID}/thing/property/report'

class GPIO:
    global gpio, gpiochip

    # 初始化GPIO对象，打开指定的GPIO芯片和引脚
    def __init__(self, gpionum, gpiochipx):
        self.gpiochip = gpiod.Chip(gpiochipx, gpiod.Chip.OPEN_BY_NUMBER)
        self.gpio = self.gpiochip.get_line(gpionum)

    # 设置引脚为输出方向，并设置初始值
    def set_direction_out(self, val):
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[val])
    
    # 设置引脚为输入方向
    def set_direction_in(self):
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_IN)

    # 设置引脚的输出值
    def set(self, val):
        self.gpio.set_value(val)

    # 获取引脚的输入值
    def get(self):
        return self.gpio.get_value()

    # 释放GPIO引脚
    def release(self):
        self.gpio.release()

def temprature_thread():
    """
    温度传感器线程函数，用于模拟温度传感器数据的生成和写入环形缓冲区
    """
    while True:

        data = os.read(dht11_fd, 6)
        temp_str = "{:d}.{:d}".format(data[2], data[3])
        humi_str = "{:d}.{:d}".format(data[0], data[1])

        current_time = int(time.time() * 1000)
        dht11_data = {
            "msgId":str(uuid.uuid4()),
            "time":current_time,
            "data":{
                "temperature":{
                    "value": temp_str,
                    "time": current_time  
                },
                "Humidity":{
                    "value": humi_str,
                    "time": current_time  
                },
            }
        }
        
        result = ring_buffer.write(dht11_data)
        if result == 0:
            # print("[dht11 Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[dht11 Thread] 环形缓冲区已满，无法写入数据")
            pass
        # 模拟传感器数据更新间隔
        threading.Event().wait(2)

def ldrntc_thread():

    while True:

        value_ldr = LDR.get_value()
        value_ntc = NTC.get_value()

        current_time = int(time.time() * 1000)
        ldrntc_data = {
            "msgId":str(uuid.uuid4()),
            "time":current_time,
            "data":{
                "ldr":{
                    "value": value_ldr,
                    "time": current_time  
                },
                "ntc":{
                    "value": value_ntc,
                    "time": current_time  
                },
            }
        }
        
        result = ring_buffer.write(ldrntc_data)
        if result == 0:
            # print("[ldrntc Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[ldrntc Thread] 环形缓冲区已满，无法写入数据")
            pass

        threading.Event().wait(2)

def bmp280_thread():

    while True:

        pressure = bmx.update_readings().pressure
        pressure_str = str(round(pressure / 1000, 2))

        current_time = int(time.time() * 1000)
        bmp280_data = {
            "msgId":str(uuid.uuid4()),
            "time":current_time,
            "data":{
                "Atmosphere":{
                    "value": pressure_str,
                    "time": current_time  
                }
            }
        }
        
        result = ring_buffer.write(bmp280_data)
        if result == 0:
            # print("[bmp280 Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[bmp280 Thread] 环形缓冲区已满，无法写入数据")
            pass

        threading.Event().wait(5)

def mq135_thread():

    a = 5.06                # a b为甲苯检测中的校准常数
    b = 2.46
    vrl_clean = 0.576028    # 洁净空气下的平均Vrl电压值（就是adc读到的平均电压值）

    while True:

        ad = AnalogIn(ads, ADS.P0)
        ratio = ad.voltage / vrl_clean 
        ppm = a * (ratio ** b)
        ppm_str = str(f"{ppm:.2f}")

        current_time = int(time.time() * 1000)
        mq135_data = {
            "msgId":str(uuid.uuid4()),
            "time":current_time,
            "data":{
                "AQI":{
                    "value": ppm_str,
                    "time": current_time  
                }
            }
        }
        
        result = ring_buffer.write(mq135_data)
        if result == 0:
            # print("[mq135 Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[mq135 Thread] 环形缓冲区已满，无法写入数据")
            pass

        threading.Event().wait(5) 

def on_connect(client, userdata, flags, rc):
    """
    MQTT 连接回调函数，处理连接成功或失败的情况

    :param client: MQTT 客户端对象
    :param userdata: 用户数据
    :param flags: 连接标志
    :param rc: 连接结果码
    """
    if rc == 0:
        print("Connect tuya IoT Cloud Sucess")
    else:
        print("Connect failed...  error code is:" + str(rc))

def connect_mqtt():
    """
    连接到 MQTT 服务器

    :return: MQTT 客户端对象
    """
    client.connect(host, port, keepAlive)
    return client

def publish_message():
    """
    从环形缓冲区读取数据并发布到 MQTT 服务器的函数
    """
    while True:
        data = ring_buffer.read()
        if data is not None:
            print("[MQTT Send Thread] 从环形缓冲区读取数据，向云平台发送数据:", data)
            client.publish(pubTopic, json.dumps(data))
        else:
            print("[MQTT Send Thread] 环形缓冲区为空，无数据可读取")
        # 模拟 MQTT 发送间隔
        threading.Event().wait(1)

# to load config file
config_file = '../configuration.json' 
config_manager = ConfigManager(config_file)  
if config_manager.inspect_current_environment() is not True:  
    exit()

# dht11 init
dht11_config = config_manager.get_board_config("dht11")                 # 从配置文件中获取dht11相关配置信息 
if dht11_config is None:  
    print("can not find 'dht11' in ", config_file)
    exit()
dht11_dev = dht11_config['devname']
dht11_fd = os.open(dht11_dev, os.O_RDONLY)

# ldr ntc init
ldr_config = config_manager.get_board_config("ldr")                   # 从配置文件中获取ldr相关配置信息 
if ldr_config is None:  
    print("can not find 'ldr' in ", config_file)
    exit()

ntc_config = config_manager.get_board_config("ntc")                   # 从配置文件中获取ntc相关配置信息 
if ntc_config is None:  
    print("can not find 'ntc' in ", config_file)
    exit()

# ldr init
gpiochip_num_ldr = ldr_config['pin_chip']
gpionum_ldr = ldr_config['pin_num']

gpiochip_ldr = gpiod.Chip(gpiochip_num_ldr, gpiod.Chip.OPEN_BY_NUMBER)
LDR=gpiochip_ldr.get_line(gpionum_ldr)
LDR.request(consumer="LDR", type=gpiod.LINE_REQ_DIR_IN)

# ntc init
gpiochip_num_ntc = ntc_config['pin_chip']
gpionum_ntc = ntc_config['pin_num']

gpiochip_ntc = gpiod.Chip(gpiochip_num_ntc, gpiod.Chip.OPEN_BY_NUMBER)
NTC=gpiochip_ntc.get_line(gpionum_ntc)
NTC.request(consumer="NTC", type=gpiod.LINE_REQ_DIR_IN)

# bmp280 init
bmp280_config = config_manager.get_board_config("bmp280")        
if bmp280_config is None:  
    print("can not find 'bmp280' key in ", config_file)
    exit()

bus_number = bmp280_config['bus']                               
cs_gpiochip = bmp280_config['cs_chip']                          
cs_gpionum = bmp280_config['cs_pin']                            

bmx = Bmx280Spi(spiBus=bus_number, cs_chip=cs_gpiochip, cs_pin=cs_gpionum)
bmx.set_power_mode(MODE_NORMAL)
bmx.set_sleep_duration_value(3)
bmx.set_temp_oversample(1)
bmx.set_pressure_oversample(1)
bmx.set_filter(0)

# ADS1115 init
ads1115_config = config_manager.get_board_config("ads1115")      
if ads1115_config is None:  
    print("can not find 'ads1115' key in ", config_file)
    exit()

bus_number = ads1115_config['bus']                              
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")

i2c_ads1115 = busio.I2C(scl_pin, sda_pin)
ads = ADS.ADS1115(i2c_ads1115)

# create dht11 thread
temprature_thread_obj = threading.Thread(target=temprature_thread)
temprature_thread_obj.daemon = True

# create ldr ntc thread
ldrntc_thread_obj = threading.Thread(target=ldrntc_thread)
ldrntc_thread_obj.daemon = True

# create bmp280 thread
bmp280_thread_obj = threading.Thread(target=bmp280_thread)
bmp280_thread_obj.daemon = True

# create mq135 thread
mq135_thread_obj = threading.Thread(target=mq135_thread)
mq135_thread_obj.daemon = True

ring_buffer = RingBuffer(10)

# 创建MQTT客户端
client = mqtt.Client(ClientID)
client.username_pw_set(UserName, Password)
client.tls_set()  				# 必须启用TLS
client.on_connect = on_connect  # 设置连接回调函数（可选）

# 连接到MQTT服务器
client.connect(Address, Port, 60)

# 等待连接建立
client.loop_start()
time.sleep(2)  

temprature_thread_obj.start()
ldrntc_thread_obj.start()
bmp280_thread_obj.start()
mq135_thread_obj.start()

publish_message()

def main():

    try:
        while True:
            time.sleep(1)

    except Exception as e:
        
        print("exit...：", e)
        
    finally:

        pass

if __name__ == "__main__":  
    main()