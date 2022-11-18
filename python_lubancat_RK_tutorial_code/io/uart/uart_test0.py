""" pyserial uart 测试 """
import serial

# 打开uart3，设置串口波特率为115200，数据位为8，无校验位，停止位为1，不使用流控制，以非阻塞模式打开串口，等待时间为3s
with serial.Serial(
    "/dev/ttyS3",
    baudrate=115200,
    bytesize=serial.EIGHTBITS,
    stopbits=serial.STOPBITS_ONE,
    parity=serial.PARITY_NONE,
    timeout=3,
) as uart3:
    # 使用申请的串口发送字节流数据 "Hello World!\n"
    uart3.write(b"Hello World!\n")

    # 以非阻塞的方式打开的串口，在读取串口接收的数据时，该函数返回条件二者满足其一，一、读取到128个字节，二、读取时间超过1秒
    buf = uart3.read(128)

    # 注：Python读取出来的数据类型为：bytes
    # 打印原始数据
    print("原始数据:\n", buf)

    # 转码为gbk字符串，可以显示中文
    data_strings = buf.decode("gbk")

    # 打印读取的数据量及数据内容
    print("读取到 {:d} 个字节 , 以字符串形式打印:\n {:s}".format(len(buf), data_strings)) 

