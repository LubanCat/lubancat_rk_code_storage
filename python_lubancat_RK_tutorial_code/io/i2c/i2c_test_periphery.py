""" periphery i2c 测试 使用0.96寸OLED模块 """
import time
from periphery import I2C

# 打开 i2c-3 控制器
i2c = I2C("/dev/i2c-3")

# 设备从机地址0x3c，即OLED模块地址
I2CSLAVEADDR = 0x3C

# 使用periphery i2c库读取功能测试
def i2c_read_reg(devregaddr):
    """
    使用periphery i2c库读取功能测试
    """
    # 构造数据结构
    # 第一个Message为要读取的设备地址
    # 第二个Message用于存放读取回来的消息，注意其中添加的read=True
    msgs = [I2C.Message([devregaddr]), I2C.Message([0x00], read=True)]
    # 发送消息，发送到i2cSlaveAddr
    i2c.transfer(I2CSLAVEADDR, msgs)
    print("从寄存器 0x{:02x} 读取出: 0x{:02x}".format(devregaddr, msgs[1].data[0]))


def oled_write_cmd(cmd):
    """
    使用periphery i2c库发送功能测试
    """
    msgs = [I2C.Message([0x00, cmd])]
    i2c.transfer(I2CSLAVEADDR, msgs)


def oled_write_data(data):
    """
    使用periphery i2c库发送功能测试
    """
    msgs = [I2C.Message([0x40, data])]
    i2c.transfer(I2CSLAVEADDR, msgs)


def oled_init():
    """
    使用OLED模块进行代码功能测试 0.96寸OLED模块初始化
    """
    time.sleep(1)
    oled_write_cmd(0xAE)
    oled_write_cmd(0x20)
    oled_write_cmd(0x10)
    oled_write_cmd(0xB0)
    oled_write_cmd(0xC8)
    oled_write_cmd(0x00)
    oled_write_cmd(0x10)
    oled_write_cmd(0x40)
    oled_write_cmd(0x81)
    oled_write_cmd(0xFF)
    oled_write_cmd(0xA1)
    oled_write_cmd(0xA6)
    oled_write_cmd(0xA8)
    oled_write_cmd(0x3F)
    oled_write_cmd(0xA4)
    oled_write_cmd(0xD3)
    oled_write_cmd(0x00)
    oled_write_cmd(0xD5)
    oled_write_cmd(0xF0)
    oled_write_cmd(0xD9)
    oled_write_cmd(0x22)
    oled_write_cmd(0xDA)
    oled_write_cmd(0x12)
    oled_write_cmd(0xDB)
    oled_write_cmd(0x20)
    oled_write_cmd(0x8D)
    oled_write_cmd(0x14)
    oled_write_cmd(0xAF)


def oled_fill(filldata):
    """
    清空OLED屏幕
    """
    for i in range(8):
        oled_write_cmd(0xB0 + i)
        # page0-page1
        oled_write_cmd(0x00)
        # low column start address
        oled_write_cmd(0x10)
        # high column start address
        for j in range(128):
            oled_write_data(filldata)


# 代码测试
try:
    # 初始化OLED屏幕，SSD1306必要
    oled_init()
    # 清空OLED屏幕
    oled_fill(0xFF)
    # 读取寄存器测试
    i2c_read_reg(0x10)
finally:
    print("测试正常结束")
    # 释放资源
    i2c.close()    