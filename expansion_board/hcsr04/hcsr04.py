import gpiod  
import time  
import math  
from collections import deque  
  
# gpionum计算方法  
# group: 0-3 (代表A-D)  
# gpionum = group * 8 + x  
# 举例 : C1 = 2 * 8 + 1 = 17  
#        B6 = 1 * 8 + 6 = 14   
#  
gpiochip_num_trig = "3"  
gpionum_trig = 17  
  
gpiochip_num_echo = "3"  
gpionum_echo = 14  
  
gpiochip_trig = gpiod.Chip(gpiochip_num_trig, gpiod.Chip.OPEN_BY_NUMBER)  
gpiochip_echo = gpiod.Chip(gpiochip_num_echo, gpiod.Chip.OPEN_BY_NUMBER)  
  
TRIG = gpiochip_trig.get_line(gpionum_trig)  
TRIG.request(consumer="TRIG", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])  
  
ECHO = gpiochip_echo.get_line(gpionum_echo)  
ECHO.request(consumer="ECHO", type=gpiod.LINE_REQ_DIR_IN)  
  
timeout = 0.2  
buffer_size = 10                                # 定义缓冲区大小，即存储多少次测量结果  
distance_buffer = deque(maxlen=buffer_size)     # 使用deque作为环形缓冲区  
  
def main():  
    try:  
        while True:  
            
            # 触发信号 10us
            TRIG.set_value(0)
            TRIG.set_value(1)
            time.sleep(0.00001)
            TRIG.set_value(0)
            
            # 等待 ECHO 信号开始
            start_time = time.time()
            while ECHO.get_value() == 0:
                if time.time() - start_time > timeout:
                    print("Waiting for the ECHO start signal timed out")
                    break
            
            # 如果超时则跳过这次测量
            if ECHO.get_value() == 0:
                continue

            # 记录 ECHO 信号开始时间
            start_time = time.time()

            # 等待 ECHO 信号结束
            while ECHO.get_value() == 1:
                if time.time() - start_time > timeout:
                    print("Waiting for the ECHO end signal timed out")
                    break
            
            # 如果超时则跳过这次测量
            if ECHO.get_value() == 1:
                continue
            
            end_time = time.time()
            dist = (end_time - start_time) * 34300 / 2

            # 将新测量结果添加到缓冲区中  
            distance_buffer.append(dist)  
              
            # 计算缓冲区中所有测量结果的平均值  
            avg_dist = sum(distance_buffer) / len(distance_buffer) if distance_buffer else 0  
              
            print("Average Distance: %.2f cm" % avg_dist)  
              
            time.sleep(0.1)  
  
    except Exception as e:  
        print("exit...：", e)  
      
    finally:  
        TRIG.release()  
        ECHO.release()  
  
if __name__ == "__main__":    
    main()