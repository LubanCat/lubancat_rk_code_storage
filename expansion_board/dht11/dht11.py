import os
import struct
import time

# 打开设备节点
dht11_fd = os.open('/dev/dht11', os.O_RDONLY)

# 循环读取温湿度数据
while True:
    try:
        # 从设备节点读取6个字节的数据
        data = os.read(dht11_fd, 6)

        # 解析数据并打印
        print(f"Temperature: {data[2]}.{data[3]}℃, Humidity: {data[0]}.{data[1]}")
    

        # 等待1秒钟再尝试读取数据
        time.sleep(1)
        
    except OSError as e:
        # 出现错误时打印错误信息并关闭设备节点
        print('Error reading data from DHT11 sensor.', e)
        os.close(dht11_fd)
        break

# 关闭设备节点
os.close(dht11_fd)