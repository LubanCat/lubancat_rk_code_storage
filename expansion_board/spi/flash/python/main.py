###############################################
#
#  file: main.py
#  update: 2024-11-14
#  usage: 
#      sudo python main.py
#
###############################################

import spidev
import gpiod
import time
import evdev
import threading
import random
from w25qxx import W25QXX

spi_bus   = 3                       # spi总线编号
key_event = "/dev/input/event9"     # 按键输入设备
cs_chip   = "6"                     # cs脚, gpiochip
cs_pin    = 11                      # cs脚, gpionum

gkey_value = 0                      # 按键输入的状态值，1：写入随机值，2：读取值
key_lock = threading.Lock()         # 按键值的线程锁

def key_scan_thread(device):
    """
    扫描按键输入事件，根据事件更新按键值
    :param device: 按键输入事件设备
    """
    global gkey_value
    for event in device.read_loop():
        with key_lock:
            if event.code == 11 and event.value == 1:       # 按键 key1 按下
                gkey_value = 1
            elif event.code == 2 and event.value == 1:      # 按键 key2 按下
                gkey_value = 2
            elif event.code == 3 and event.value == 1:      # 按键 key3 按下
                gkey_value = 3

# 初始化按键设备并启动监听线程
device = evdev.InputDevice("/dev/input/event9")             # 指定按键事件设备
thread_key = threading.Thread(target=key_scan_thread, args=(device,))
thread_key.daemon = True
thread_key.start()

# 初始化 W25QXX 芯片
w25qxx = W25QXX(spi_bus, cs_pin, cs_chip)  
if w25qxx.w25qxx_init() != 0:
    print("w25qxx init error!")
    exit(0)

def main():

    global gkey_value
    key_input = 0

    try:

        read_data = w25qxx.w25qxx_read_byte_data(0x000000, 1)
        print("read one byte : ", hex(read_data[0]))

        while True:
            
            with key_lock:
                key_input = gkey_value

            if key_input == 1:      # 按键 key1 ：写入随机值
                data_to_write = [random.randint(0, 255)]    # 生成 0-255 的随机值
                w25qxx.w25qxx_npage_write(0x000000, data_to_write, len(data_to_write))
                print("write one byte : ", hex(data_to_write[0]))
            elif key_input == 2:    # 按键 key2 ：读取值
                read_data = w25qxx.w25qxx_read_byte_data(0x000000, 1)
                print("read one byte : ", hex(read_data[0]))

            with key_lock:
                gkey_value = 0      # 清除按键状态

            time.sleep(0.1)

    except Exception as e:
        
        print("exit...：", e)

if __name__ == "__main__":  
    main()
