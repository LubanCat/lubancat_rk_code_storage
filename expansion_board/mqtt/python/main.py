import json
import time
import paho.mqtt.client as mqtt
import hmac
from hashlib import sha256
import uuid
import threading
import signal
from ringbuff import RingBuffer

# 待上传的温度值和led开关值
temp_value = 35
led_value = 1

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

def temprature_thread():
    """
    温度传感器线程函数，用于模拟温度传感器数据的生成和写入环形缓冲区
    """
    current_time = int(time.time() * 1000)
    dht11_data = {
        "msgId":str(uuid.uuid4()),
        "time":current_time,
        "data":{
            "temperature":{
                "value": temp_value,
                "time": current_time  
            }
        }
    }

    while True:
        result = ring_buffer.write(dht11_data)
        if result == 0:
            # print("[DHT11 Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[DHT11 Thread] 环形缓冲区已满，无法写入数据")
            pass
        # 模拟传感器数据更新间隔
        threading.Event().wait(2)

def led_thread():
    """
    LED 传感器线程函数，用于模拟 LED 状态数据的生成和写入环形缓冲区
    """
    current_time = int(time.time() * 1000)
    led_data = {
        "msgId":str(uuid.uuid4()),
        "time":current_time,
        "data":{
            "led":{
                "value": led_value,
                "time": current_time  
            }
        }
    }

    while True:
        result = ring_buffer.write(led_data)
        if result == 0:
            # print("[LED Thread] 成功将数据写入环形缓冲区")
            pass
        else:
            print("[LED Thread] 环形缓冲区已满，无法写入数据")
            pass
        # 模拟传感器数据更新间隔
        threading.Event().wait(2)

def on_connect(client, userdata, flags, rc):
    """
    MQTT 连接回调函数，处理连接成功或失败的情况

    :param client: MQTT 客户端对象
    :param userdata: 用户数据
    :param flags: 连接标志
    :param rc: 连接结果码
    """
    if rc == 0:
        print("Connect Tuya IoT Cloud Sucess")
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
            # print("[MQTT Send Thread] 从环形缓冲区读取数据:", data)
            client.publish(pubTopic, json.dumps(data))
            # print("publish ok, data = ", json.dumps(data))
        else:
            print("[MQTT Send Thread] 环形缓冲区为空，无数据可读取")
        # 模拟 MQTT 发送间隔
        threading.Event().wait(1)

def subscribe_topic():
    """
    订阅 MQTT 主题
    """
    pass

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

# 启动temprature和led线程
temprature_thread_obj = threading.Thread(target=temprature_thread)
led_thread_obj = threading.Thread(target=led_thread)

temprature_thread_obj.start()
led_thread_obj.start()

subscribe_topic()
publish_message()

while True:
    time.sleep(1)