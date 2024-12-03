###############################################
#
#  file: w25qxx.py
#  update: 2024-11-14
#  usage: 
#      sudo python w25qxx.py
#
###############################################

import time
import spidev
import gpiod

W25QXX_80_ID = 0xEF13
W25QXX_16_ID = 0xEF14
W25QXX_32_ID = 0xEF15
W25QXX_64_ID = 0xEF16
W25QXX_128_ID = 0xEF17

W25QXX_PAGE_SIZE = 256
W25QXX_SECTOR_SIZE = 4096

W25QXX_WRITE_ENABLE = 0x06
W25QXX_SECTOR_ERASE_4KB = 0x20
W25QXX_PAGE_PROGRAM = 0x02
W25QXX_READ_DATA = 0x03
W25QXX_READ_STATUS_REGISTER_1 = 0x05

# W25QXX类
class W25QXX:
    def __init__(self, spi_bus, cs_pin, cs_chip):
        self.spi_bus = spi_bus
        self.cs_pin = cs_pin
        self.cs_chip = cs_chip
        self.gpiochip = gpiod.Chip(cs_chip, gpiod.Chip.OPEN_BY_NUMBER)                              # 选择正确的GPIO芯片
        self.cs_line = self.gpiochip.get_line(self.cs_pin)                                          # 设置CS引脚
        self.cs_line.request(consumer="w25qxx_spi", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[1])  # 拉高CS，默认不选择

        self.spi = spidev.SpiDev()          # 创建SPI对象
        self.spi.open(spi_bus, 0)           # 设置SPI总线

        self.spi.max_speed_hz = 1000000     # 设置SPI速度
        self.spi.mode = 0b00                # SPI模式0

        self.device_id = None

    def w25qxx_init(self):
        """初始化W25QXX并获取设备ID"""
        device_id = self.w25qxx_read_id()
        if device_id in [W25QXX_80_ID, W25QXX_16_ID, W25QXX_32_ID, W25QXX_64_ID, W25QXX_128_ID]:
            self.device_id = device_id
            print(f"Device ID: {hex(device_id)}")
            return 0
        else:
            print("Unknown device ID")
            return -1

    def w25qxx_read_id(self):
        """读取设备ID"""
        send_buf = [0x90, 0x00, 0x00, 0x00]         # 发送命令
        self.cs_line.set_value(0)                   # 拉低CS，开始SPI通信
        self.spi.xfer(send_buf)                     # 发送数据
        receive_data = self.spi.readbytes(2)        # 读取2字节数据
        self.cs_line.set_value(1)                   # 拉高CS，结束SPI通信
        device_id = (receive_data[0] << 8) | receive_data[1]
        return device_id

    def w25qxx_wait_state_free(self):
        """等待W25QXX状态寄存器的忙标志位清除"""
        while True:
            self.cs_line.set_value(0)                       # 拉低CS，开始SPI通信
            self.spi.xfer([W25QXX_READ_STATUS_REGISTER_1])  # 发送命令
            recv_buf = self.spi.readbytes(1)                # 读状态
            self.cs_line.set_value(1)                       # 拉高CS，结束SPI通信
            if recv_buf[0] & 0x01 == 0:                     # 判断忙标志位
                break
            time.sleep(0.01)

    def w25qxx_write_enable(self):
        """使能W25QXX写操作"""
        self.cs_line.set_value(0)
        self.spi.xfer([W25QXX_WRITE_ENABLE])                # 发送写使能命令
        self.cs_line.set_value(1)

    def w25qxx_sector_erase(self, addr):
        """对指定地址进行4KB扇区擦除"""
        self.w25qxx_write_enable()
        send_buf = [W25QXX_SECTOR_ERASE_4KB, addr >> 16, addr >> 8, addr]
        self.cs_line.set_value(0)
        self.spi.xfer(send_buf)                             # 发送扇区擦除命令
        self.cs_line.set_value(1)
        self.w25qxx_wait_state_free()

    def w25qxx_page_write(self, addr, write_buff):
        """向指定页写入数据"""
        if len(write_buff) < 1 or len(write_buff) > W25QXX_PAGE_SIZE:
            print("Page write error, invalid data length. len = ", len(write_buff))
            return
        self.w25qxx_write_enable()
        send_buf = [W25QXX_PAGE_PROGRAM, addr >> 16, addr >> 8, addr]
        send_buf.extend(write_buff)
        self.cs_line.set_value(0)
        self.spi.xfer(send_buf)
        self.cs_line.set_value(1)
        self.w25qxx_wait_state_free()

    def w25qxx_npage_write(self, addr, write_buff, write_len):
        """执行多页写入操作，考虑到可能的跨页和跨扇区写入"""
        page_size = W25QXX_PAGE_SIZE            # 每页的字节数
        sector_size = W25QXX_SECTOR_SIZE        # 每个扇区的字节数

        offset = 0                              # 当前写入的偏移量
        total_len = write_len                   # 总写入长度

        sector = -1                             # 上次写入的扇区编号
        current_sector = 0                      # 当前要写入的扇区编号

        if write_len < 1:
            print("Page write error, data length is invalid!")
            return

        while total_len > 0:
            # 计算当前数据的目标扇区编号
            current_sector = (addr + offset) // sector_size

            if sector != current_sector:
                sector = current_sector
                # print(f"Erasing sector {sector * sector_size}...")
                self.w25qxx_sector_erase(sector * sector_size)  # 擦除目标扇区

            # 每次最多写入 page_size 字节，剩余数据长度不超过 page_size 时，调整写入长度
            write_bytes = min(page_size, total_len)

            # 调用 page_write 进行单页写入
            # print(f"Writing data to address {hex(addr + offset)}: {write_buff[offset:offset + write_len]}")
            self.w25qxx_page_write(addr + offset, write_buff[offset:offset + write_bytes])

            # 更新偏移量和剩余数据长度
            offset += write_bytes
            total_len -= write_bytes

    def w25qxx_read_byte_data(self, addr, read_len):
        """从指定地址读取数据"""
        send_buf = [W25QXX_READ_DATA, addr >> 16, addr >> 8, addr]
        self.cs_line.set_value(0)  
        self.spi.xfer(send_buf)  
        receive_data = self.spi.readbytes(read_len)  
        self.cs_line.set_value(1)  
        return receive_data


