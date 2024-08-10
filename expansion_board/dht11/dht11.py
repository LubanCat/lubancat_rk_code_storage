###############################################
#
#  file: dht11.py
#  update: 2024-08-10
#  usage: 
#      sudo python dht11.py
#
###############################################

import os
import struct
import time

# 打开设备节点
dht11_fd = os.open('/dev/dht11', os.O_RDONLY)

def main():
    try:
        while True:
            
            # 从设备节点读取6个字节的数据
            data = os.read(dht11_fd, 6)

            # 解析数据并打印
            print(f"Temperature: {data[2]}.{data[3]}℃, Humidity: {data[0]}.{data[1]}")

            time.sleep(1)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        os.close(dht11_fd)
        
if __name__ == "__main__":  
    main()