import json
import time
import paho.mqtt.client as mqtt
from MqttSign import AuthIfo
from ringbuff import RingBuffer
import threading

# set the device info, include product key, device name, and device secret
productKey = "a1pwoLHW8Tl"
deviceName = "lubancat"
deviceSecret = "160cacffcba7b83f2eb896403688dac5"

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

temp_value = 35
led_value = 1

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

def temprature_thread():
    """
    温度传感器线程函数，用于模拟温度传感器数据的生成和写入环形缓冲区
    """
    dht11_data = {
        "id": "123",
        "version": "1.0",
        "sys": {"ack": 0},
        "params": {"temperature": {"value": "34"}},
        "method": "thing.event.property.post"
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
    led_data = {
        "id": "123",
        "version": "1.0",
        "sys": {"ack": 0},
        "params": {"led": {"value": "0"}},
        "method": "thing.event.property.post"
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
            # print("[MQTT Send Thread] 从环形缓冲区读取数据:", data)
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

ring_buffer = RingBuffer(10)

# Set the on_connect callback function for the MQTT client
client.on_connect = on_connect
# Set the on_message callback function for the MQTT client
client.on_message = on_message
client = connect_mqtt()
# Start the MQTT client loop in a non-blocking manner
client.loop_start()
time.sleep(2)

temprature_thread_obj = threading.Thread(target=temprature_thread)
temprature_thread_obj.daemon = True
temprature_thread_obj.start()

led_thread_obj = threading.Thread(target=led_thread)
led_thread_obj.daemon = True
led_thread_obj.start()

subscribe_topic()
publish_message()

while True:
    time.sleep(1)