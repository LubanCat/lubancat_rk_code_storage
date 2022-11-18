""" blinka i2c 测试 使用0.96寸OLED模块 """
import busio
import board

# 初始化OLED命令字节数组
OledInitBuf = bytes(
    [
        0xAE,
        0x20,
        0x10,
        0xB0,
        0xC8,
        0x00,
        0x10,
        0x40,
        0x81,
        0xFF,
        0xA1,
        0xA6,
        0xA8,
        0x3F,
        0xA4,
        0xD3,
        0x00,
        0xD5,
        0xF0,
        0xD9,
        0x22,
        0xDA,
        0x12,
        0xDB,
        0x20,
        0x8D,
        0x14,
        0xAF,
    ]
)

# 数据发送、接受缓冲区
OutBuffer = bytearray(1)
InBuffer = bytearray(1)

try:
    # 申请i2c资源
    i2c = busio.I2C(board.I2C3_SCL, board.I2C3_SDA)
    # 扫描I2C设备地址，测试
    print("挂载I2C总线上的I2C设备地址有")
    for i in i2c.scan():
        print("0x%02x " % i)

    # IIC发送命令测试(writeto)，初始化OLED
    # 初始化OLED，初始命令存放于字节数组oledInitBuf中
    i2c.writeto(0x3C, OledInitBuf)

    # IIC读取命令测试(writeto_then_readfrom)，读取OLED的0x10寄存器
    # 发送数据，发送数据完成后不产生停止信号，并重新生产起始信号进行读取。

    # 发送数据内容存放在字节数组outBuffer中，内容为OLED寄存器地址：0x10
    OutBuffer[0] = 0x10
    # 发送，发送寄存器地址0x10，并将寄存器地址0x10下的内容读取到inBuffer中
    i2c.writeto_then_readfrom(0x3C, OutBuffer, InBuffer)
    # 打印数据信息
    print("(writeto_then_readfrom)读取到的内容为：0x%02x" % InBuffer[0])

    # IIC读取测试(readfrom_into)，从设备内部的地址读，则不需要指定读取的地址
    i2c.readfrom_into(0x3C, InBuffer)
    # 打印数据信息
    print("(readfrom_into)读取到的内容为：0x%02x" % InBuffer[0])

    # OLED清屏
    for i in range(8):
        i2c.writeto(0x3C, bytearray([0x00, 0xB0 + i]))  # page0-page1
        i2c.writeto(0x3C, bytearray([0x00, 0x00]))  # low column start address
        i2c.writeto(0x3C, bytearray([0x00, 0x10]))  # high column start address
        for j in range(128):
            i2c.writeto(0x3C, bytearray([0x40, 0xFF]))

finally:
    # 释放i2c总线资源
    i2c.deinit()