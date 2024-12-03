from max30102 import MAX30102
import time
import numpy as np
import board
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306

PPG_DATA_THRESHOLD = 100000     # PPG数据检测阈值，低于此值将忽略数据
CACHE_NUMS = 150                # 缓存数，存储红外和红光信号的样本数

# max30102初始化
max30102 = MAX30102(i2c_bus=5)
#-----------------------------------------------------------------

# oled初始化
oled_init_flag = 1
try:
    # 设置I2C接口并初始化OLED显示屏
    oled_i2c = busio.I2C(board.I2C3_SCL, board.I2C3_SDA)
    disp = adafruit_ssd1306.SSD1306_I2C(128, 64, oled_i2c)

    width = disp.width  
    height = disp.height  
    image = Image.new('1', (width, height))  
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, width, height), outline=0, fill=0)

    # 设置字体
    font = ImageFont.truetype("NotoSerifCJKhk-VF.ttf", 12, encoding="unic")  
    x = 5   
    top = 5  
except Exception as e:
    oled_init_flag = 0
#-----------------------------------------------------------------

def main():

    # 初始化数据缓存
    ppg_data_cache_ir = np.zeros(CACHE_NUMS)            # 红外光缓存区
    ppg_data_cache_red = np.zeros(CACHE_NUMS)           # 红光缓存区
    cache_counter = 0                                   # 缓存计数器

    # 如果OLED初始化成功，显示初始值
    if oled_init_flag != 0:                             
        draw.rectangle((1, 1, width-1, height-1), outline=0, fill=0)
        draw.text((5, 16), "0 BMP", font=font, fill=255)
        draw.text((5, 32), "0.00 %", font=font, fill=255)
        disp.image(image)  
        disp.show()

    print("Start sensor ...")

    try:

        while True:

            # 读取FIFO数据
            ir_data, red_data = max30102.read_fifo()
            filter_ir_data, filter_red_data = max30102.low_pass_filter(ir_data, red_data)
            filter_ir_data, filter_red_data = max30102.average_filter(filter_ir_data, filter_red_data)
            #print("raw data :", ir_data, red_data)
            #print("filter data :", filter_ir_data, filter_red_data)

            # 收集有效数据
            if filter_ir_data > PPG_DATA_THRESHOLD and filter_red_data > PPG_DATA_THRESHOLD:
                ppg_data_cache_ir[cache_counter] = filter_ir_data       # 缓存红外光数据
                ppg_data_cache_red[cache_counter] = filter_red_data     # 缓存红光数据
                cache_counter += 1                                      # 更新缓存计数器
            else:
                print("未检测到手指！")
                pass

            # 计算血氧和心率
            if cache_counter >= CACHE_NUMS:                             # 当缓存满时进行计算
                bmp = max30102.get_heart_rate(ppg_data_cache_ir, cache_counter)  
                spo2 = max30102.get_spo2(ppg_data_cache_ir, ppg_data_cache_red, cache_counter)
                print(f"心率：{bmp:.0f} 次/min\t血氧：{spo2:.2f}%")

                # 更新OLED显示心率和血氧
                if oled_init_flag != 0:
                    draw.rectangle((1, 1, width-1, height-1), outline=0, fill=0)
                    draw.text((5, 16), str(int(bmp))+" BMP", font=font, fill=255)               # 心率显示
                    draw.text((5, 32), str(round(spo2, 2))+" %", font=font, fill=255)           # 血氧显示
                    disp.image(image)  
                    disp.show() 

                cache_counter = 0       # 重置缓存计数器

            #time.sleep(0.012)           # 读取间隔

    except Exception as e:
        
        print("exit...：", e)
        
    finally:

        if oled_init_flag != 0:
            # oled清屏
            disp.fill(0)
            disp.show() 

        exit(0)

if __name__ == "__main__":  
    main()
