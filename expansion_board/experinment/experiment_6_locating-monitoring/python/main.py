import json
import time
import gpiod
import board
import busio
import os
import threading
import signal
import sys
import hmac
import uuid
import paho.mqtt.client as mqtt
import adafruit_ads1x15.ads1115 as ADS
from adafruit_ads1x15.analog_in import AnalogIn
from http_client import HTTPClient
import xml.etree.ElementTree as ET
from hashlib import sha256

from config import ConfigManager 
from ringbuff import RingBuffer
from mpu6050 import MPU6050
from gps import GPS

# Gaode API key
gaodeApiKey = "cc834f3ef25cb1f3a87bc8f14f8a5873"

# 设备信息
DeviceID = '2669d2b95856963bb4alms'         # 替换成自己的DeviceID
DeviceSecret = 'FNjwnHf7sGQCJzhc'           # 替换成自己的DeviceSecret

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

# 全局事件对象
stop_event = threading.Event()

def handle_sigint(signum, frame):
    stop_event.set()            # 设置停止事件
    mpu6050_thread_obj.join()   # 等待 mpu6050 线程退出
    atgm332d_thread_obj.join()  # 等待 atgm332d 线程退出
    client.loop_stop()          # 停止 MQTT 客户端循环
    client.disconnect()         # 断开 MQTT 连接
    os._exit(0)

def mpu6050_thread():

    print("mpu6050_thread has been start")

    while not stop_event.is_set():

        accelX, accelY, accelZ = mpu6050.readAcc()
        gyroX, gyroY, gyroZ = mpu6050.readGyro()

        accelXYZStr = str("%.2f"%accelX) + "," + str("%.2f"%accelY) + "," + str("%.2f"%accelZ)
        gyroXYZStr = str("%.2f"%gyroX) + "," + str("%.2f"%gyroY) + "," + str("%.2f"%gyroZ)

        print("accelXYZ :", accelXYZStr, " gyroXYZ :", gyroXYZStr)

        current_time = int(time.time() * 1000)
        accel_data = {
            "msgId":str(uuid.uuid4()),
            "time":current_time,
            "data":{
                "mpu6050_accel":{
                    "value": accelXYZStr,
                    "time": current_time  
                },
                "mpu6050_gyro":{
                    "value": gyroXYZStr,
                    "time": current_time  
                },
            }
        }

        result = ring_buffer.write(accel_data)
        if result == 0:
            # print("[mpu6050 Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[mpu6050 Thread] 环形缓冲区已满，无法写入数据")
            pass

        time.sleep(1)

    print("mpu6050_thread has been quit")

def atgm332d_thread():

    print("atgm332d_thread has been start")
    url = "https://restapi.amap.com/v3/assistant/coordinate/convert"

    while not stop_event.is_set():

        lat, lon, bjtime = gps.read()
        locationStr = lon + "," + lat
        # print("gps raw lon,lat : " + locationStr)

        params = {
            'locations': locationStr, 
            'coordsys': 'gps', 
            'output': 'xml', 
            'key': gaodeApiKey
        }
        response_xml = http_client.get(url, params=params, headers={})
        if response_xml is not None:
            # 提取各个元素的值
            status = int(response_xml.find('status').text)
            info = response_xml.find('info').text
            infocode = response_xml.find('infocode').text
            if status == 0:
                print("http request to restapi.amap.com failed, errcode : " + infocode)
                locationStr = "null,null"
            else:
                locationStr = response_xml.find('locations').text

        print("location : " + locationStr)
        current_time = int(time.time() * 1000)
        location_data = {
            "msgId":str(uuid.uuid4()),
            "time":current_time,
            "data":{
                "gps_lon_lat":{
                    "value": locationStr,
                    "time": current_time  
                }
            }
        }

        result = ring_buffer.write(location_data)
        if result == 0:
            # print("[atgm332d Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[atgm332d Thread] 环形缓冲区已满，无法写入数据")
            pass

        time.sleep(5)
    
    print("atgm332d_thread has been quit")

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
            # print("[MQTT Send Thread] 从环形缓冲区读取数据，向云平台发送数据:", data)
            client.publish(pubTopic, json.dumps(data))
        else:
            print("[MQTT Send Thread] 环形缓冲区为空，无数据可读取")
        # 模拟 MQTT 发送间隔
        threading.Event().wait(1)

signal.signal(signal.SIGINT, handle_sigint)

# to load config file
config_file = '../configuration.json' 
config_manager = ConfigManager(config_file)  
if config_manager.inspect_current_environment() is not True:  
    exit()

# mpu6050 init
mpu6050_config = config_manager.get_board_config("mpu6050")             # 从配置文件中获取mpu6050相关配置信息 
if mpu6050_config is None:  
    print("can not find 'mpu6050' in ", config_file)
    exit()

bus_number = mpu6050_config['bus']                                      # 获取i2c总线编号
scl_name = f"I2C{bus_number}_SCL"  
sda_name = f"I2C{bus_number}_SDA"

try:                                                    
    scl_pin = getattr(board, scl_name)  
    sda_pin = getattr(board, sda_name)  

    i2c = busio.I2C(scl_pin, sda_pin)
    mpu6050 = MPU6050(i2c)
    mpu6050.start()
except AttributeError:  
    raise ValueError(f"Unsupported I2C bus number: {bus_number}, no pins found for SCL ({scl_name}) or SDA ({sda_name})")
    exit(-1)
except OSError as e:
    print(f"OSError: {e}. Make sure the device is connected and the address is correct.")
    exit(-1)
except ValueError as e:
    print(f"ValueError: {e}. Check the I2C device address.")
    exit(-1)

# atgm332d init
gps_config = config_manager.get_board_config("atgm332d")        # 从配置文件中获取gps atgm332d相关配置信息 
if gps_config is None:  
    print("can not find 'atgm332d' key in ", config_file)
    exit()

gps_uart_dev = gps_config['tty-dev']
gps = GPS(gps_uart_dev, 9600)

# create mpu6050 thread
mpu6050_thread_obj = threading.Thread(target=mpu6050_thread)

# create atgm332d thread
atgm332d_thread_obj = threading.Thread(target=atgm332d_thread)

ring_buffer = RingBuffer(10)

# http client
http_client = HTTPClient()

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

mpu6050_thread_obj.start()
atgm332d_thread_obj.start()

publish_message()

def main():

    try:
        while True:
            mpu6050_thread_obj.join()
            mpu6050_thread_obj.join()
            time.sleep(1)

    except Exception as e:
        
        print("exit...：", e)
        
    finally:

        pass

if __name__ == "__main__":  
    main()