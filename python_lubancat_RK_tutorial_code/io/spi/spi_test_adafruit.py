""" blinka spi 测试 """
import busio
import board

# 数据发送、接受缓冲区
OutBuffer = [0xAA, 0xBB, 0xCC, 0xDD]
InBuffer = bytearray(4)

try:
    # 申请spi资源
    spi = busio.SPI(board.SCLK, board.MOSI, board.MISO)

    # 配置时先锁定SPI
    spi.try_lock()
    # 配置SPI主机工作速率为1MHz、时钟极性为0、时钟相位为0、单个数据位数为8位
    spi.configure(1000000, 0, 0, 8)
    # 配置操作完成解锁
    spi.unlock()

    # SPI通讯，发送的同时也进行读取，发送的数据存放在OutBuffer，读取的数据存放在InBuffer
    spi.write_readinto(OutBuffer, InBuffer)

    print(
        "(write_readinto)接收到的数据: [0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}]".format(
            *InBuffer
        )
    )

    # SPI通讯，只写使用方法演示
    spi.write(OutBuffer)
    print(
        "(OutBuffer)发送的数据: [0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}]".format(*OutBuffer)
    )

    InBuffer = [0, 0, 0, 0]
    # SPI通讯，只读使用方法演示
    spi.readinto(InBuffer)
    print(
        "(readinto)接收到的数据: [0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}]".format(*InBuffer)
    )
finally:
    # 释放spi资源
    spi.deinit()