###############################################
#
#  file: main.py
#  update: 2025-02-13
#  usage: 
#      sudo python main.py
#
###############################################

import json
import time
import gpiod
import board
import busio
import os
import threading
import signal
import sys
import paho.mqtt.client as mqtt
import adafruit_ads1x15.ads1115 as ADS
from adafruit_ads1x15.analog_in import AnalogIn
from config import ConfigManager 
from MqttSign import AuthIfo
from ringbuff import RingBuffer
from mpu6050 import MPU6050
from gps import GPS
from http_client import HTTPClient
import xml.etree.ElementTree as ET

# set the device info, include product key, device name, and device secret
productKey = "a19UBnDR9bp"
deviceName = "lubancat"
deviceSecret = "48a3b8f5c88e6681166e7b717713ce87"

# Gaode API key
gaodeApiKey = "cc834f3ef25cb1f3a87bc8f14f8a5873"

# set timestamp, clientid, subscribe topic and publish topic
timeStamp = str((int(round(time.time() * 1000))))
clientId = "lubancat"
pubTopic = "/sys/" + productKey + "/" + deviceName + "/thing/event/property/post"
subTopic = "/sys/" + productKey + "/" + deviceName + "/thing/event/property/post_reply"

# set host, port
host = productKey + ".iot-as-mqtt.cn-shanghai.aliyuncs.com"
# instanceId = "***"
# host = instanceId + ".mqtt.iothub.aliyuncs.com"
port = 1883
keepAlive = 300

# calculate the login auth info, and set it into the connection options
m = AuthIfo()
m.calculate_sign_time(productKey, deviceName, deviceSecret, clientId, timeStamp)
client = mqtt.Client(m.mqttClientId)
client.username_pw_set(username=m.mqttUsername, password=m.mqttPassword)

# 全局事件对象
stop_event = threading.Event()

def handle_sigint(signum, frame):
    stop_event.set()  # 设置停止事件
    mpu6050_thread_obj.join()  # 等待 mpu6050 线程退出
    atgm332d_thread_obj.join()  # 等待 atgm332d 线程退出
    client.loop_stop()  # 停止 MQTT 客户端循环
    client.disconnect()  # 断开 MQTT 连接
    os._exit(0)

def create_alink_json(name, value):
    """
    创建符合阿里云 ALINK 协议的 JSON 消息

    :param name: 传感器名称
    :param value: 传感器的测量值
    :return: 生成的 JSON 字符串，如果创建失败返回 None
    """
    # 检查输入参数是否有效
    if not name or not value:
        return None

    # 构建 JSON 结构
    alink_json = {
        "id": "123",
        "version": "1.0",
        "sys": {
            "ack": 0
        },
        "params": {
            name: {
                "value": value
            }
        },
        "method": "thing.event.property.post"
    }

    try:
        # 将字典转换为 JSON 字符串
        result = json.dumps(alink_json)
        return result
    except Exception as e:
        print(f"创建 JSON 时出错: {e}")
        return None

def mpu6050_thread():

    print("mpu6050_thread has been start")

    while not stop_event.is_set():

        accelX, accelY, accelZ = mpu6050.readAcc()
        gyroX, gyroY, gyroZ = mpu6050.readGyro()

        accelXYZStr = str("%.2f"%accelX) + "," + str("%.2f"%accelY) + "," + str("%.2f"%accelZ)
        gyroXYZStr = str("%.2f"%gyroX) + "," + str("%.2f"%gyroY) + "," + str("%.2f"%gyroZ)

        print("accelXYZ :", accelXYZStr, " gyroXYZ :", gyroXYZStr)

        accel_data = {
            "id": "123",
            "version": "1.0",
            "sys": {"ack": 0},
            "params": {"mpu6050_accel": {"value": accelXYZStr}, "mpu6050_gyro": {"value": gyroXYZStr}},
            "method": "thing.event.property.post"
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
        location_data = {
            "id": "123",
            "version": "1.0",
            "sys": {"ack": 0},
            "params": {"gps_lon_lat": {"value": locationStr}},
            "method": "thing.event.property.post"
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
        print("Connect aliyun IoT Cloud Sucess")
    else:
        print("Connect failed...  error code is:" + str(rc))

def on_message(client, userdata, msg):
    """
    MQTT 消息接收回调函数，处理接收到的消息

    :param client: MQTT 客户端对象
    :param userdata: 用户数据
    :param msg: 接收到的消息对象，包含主题和负载
    """
    topic = msg.topic
    payload = msg.payload.decode()
    print("receive message ---------- topic is : " + topic)
    print("receive message ---------- payload is : " + payload)
    print("\n")

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

def subscribe_topic():
    """
    订阅 MQTT 主题
    """
    # subscribe to subTopic("/a1LhUsK****/python***/user/get") and request messages to be delivered
    client.subscribe(subTopic)
    print("subscribe topic: " + subTopic)

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
except OSError as e:
    print(f"OSError: {e}. Make sure the device is connected and the address is correct.")
    mpu6050_init_flag = 0
except ValueError as e:
    print(f"ValueError: {e}. Check the I2C device address.")
    mpu6050_init_flag = 0

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

# Set the on_connect callback function for the MQTT client
client.on_connect = on_connect
# Set the on_message callback function for the MQTT client
client.on_message = on_message
client = connect_mqtt()
# Start the MQTT client loop in a non-blocking manner
client.loop_start()
time.sleep(2)

mpu6050_thread_obj.start()
atgm332d_thread_obj.start()

subscribe_topic()
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