###############################################
#
#  file: spi.py
#  update: 2024-08-12
#  usage: 
#      sudo python spi.py
#
###############################################

from periphery import SPI

# 待发送数据列表
data_out = [0xAA, 0xBB, 0xCC, 0xDD]

def main():
    
    try:
  
        # 申请SPI资源，打开 spidev3.0 控制器，配置SPI主机为工作模式0、工作速率为1MHz
        spi = SPI("/dev/spidev3.0", 0, 1000000)

        # 发送数据，同时接收数据到data_in列表中
        data_in = spi.transfer(data_out)

        # 打印发送的数据内容
        print("发送的数据: [0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}]".format(*data_out))
        print("接收到的数据: [0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}]".format(*data_in))

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        # 关闭申请的SPI资源
        spi.close()
        
if __name__ == "__main__":  
    main()
