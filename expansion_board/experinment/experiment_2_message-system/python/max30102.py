import time 
import smbus
import numpy as np

# 寄存器地址定义
REG_INTR_STATUS_1   = 0x00  # 中断状态寄存器 1
REG_INTR_STATUS_2   = 0x01  # 中断状态寄存器 2
REG_INTR_ENABLE_1   = 0x02  # 中断使能寄存器 1
REG_INTR_ENABLE_2   = 0x03  # 中断使能寄存器 2

REG_FIFO_WR_PTR     = 0x04  # FIFO写指针寄存器
REG_OVF_COUNTER     = 0x05  # 溢出计数寄存器
REG_FIFO_RD_PTR     = 0x06  # FIFO读指针寄存器
REG_FIFO_DATA       = 0x07  # FIFO数据寄存器

REG_FIFO_CONFIG     = 0x08  # FIFO配置寄存器
REG_MODE_CONFIG     = 0x09  # 模式配置寄存器
REG_SPO2_CONFIG     = 0x0A  # 血氧配置寄存器
REG_LED1_PA         = 0x0C  # LED1驱动电流寄存器
REG_LED2_PA         = 0x0D  # LED2驱动电流寄存器

REG_PILOT_PA        = 0x10  # 引导LED驱动电流寄存器
REG_MULTI_LED_CTRL1 = 0x11  # 多LED控制寄存器 1
REG_MULTI_LED_CTRL2 = 0x12  # 多LED控制寄存器 2

REG_TEMP_INTR       = 0x1F  # 温度中断寄存器
REG_TEMP_FRAC       = 0x20  # 温度小数部分寄存器
REG_TEMP_CONFIG     = 0x21  # 温度配置寄存器

REG_PROX_INT_THRESH = 0x30  # 近距离检测阈值寄存器

REG_REV_ID          = 0xFE  # 版本ID寄存器
REG_PART_ID         = 0xFF  # 部件ID寄存器

class MAX30102():

    def __init__(self, i2c_bus=1, address=0x57, filter_size=10, alpha=0.1):
        """
        初始化MAX30102传感器
        @param i2c_bus: I2C通道
        @param address: I2C地址
        @param filter_size: 滤波器大小
        @param alpha: EWMA的平滑系数
        """
        self.filter_size = filter_size
        self.alpha = alpha                          # 指数加权平均的系数

        self.filter_size = filter_size
        self.data_buffer_ir = [0] * filter_size     # 存储红外信号的缓冲区
        self.data_buffer_red = [0] * filter_size    # 存储红光信号的缓冲区
        self.index_ir = 0                           # 红外信号索引
        self.index_red = 0                          # 红光信号索引

        # 初始化 last_ir 和 last_red 用于指数加权移动平均（EWMA）
        self.last_ir = 0
        self.last_red = 0

        # 初始化I2C总线
        self.address = address
        self.channel = i2c_bus
        self.bus = smbus.SMBus(self.channel)

        # 读取部件ID
        id = self.bus.read_byte_data(self.address, REG_PART_ID)
        if id == 0x15:
            print("Find the MAX30102! id :", hex(id))           # 成功找到MAX30102
        else:
            print("Can not find the MAX30102! id :", hex(id))
            exit(0)

        # 初始化MAX30102
        self.setup()
        
    def setup(self):
        """
        设置MAX30102的寄存器配置。
        """
        self.bus.write_byte_data(self.address, REG_MODE_CONFIG, 0x40)        # 重置max30102
        time.sleep(0.05)  

        self.bus.write_byte_data(self.address, REG_INTR_ENABLE_1, 0xE0)      # 使能中断：FIFO几乎满标志、新FIFO数据准备就绪、环境光消除溢出、供电准备就绪
        self.bus.write_byte_data(self.address, REG_INTR_ENABLE_2, 0x00)      # 使能内部温度标准标志

        self.bus.write_byte_data(self.address, REG_FIFO_WR_PTR, 0x00)        # 清除FIFO写指针寄存器
        self.bus.write_byte_data(self.address, REG_OVF_COUNTER, 0x00)        # 清除FIFO溢出计数器，重置为0
        self.bus.write_byte_data(self.address, REG_FIFO_RD_PTR, 0x00)        # 清除FIFO读指针寄存器

        self.bus.write_byte_data(self.address, REG_FIFO_CONFIG, 0x4F)        # FIFO配置：样本平均值(4)，FIFO满时循环(0)，FIFO几乎满值(当剩余15个空样本时产生中断)

        self.bus.write_byte_data(self.address, REG_MODE_CONFIG, 0x03)        # 设置为SpO2模式

        self.bus.write_byte_data(self.address, REG_SPO2_CONFIG, 0x2A)        # SpO2配置：电流15.63pA，采样率200Hz，LED脉宽215us

        self.bus.write_byte_data(self.address, REG_LED1_PA, 0x2F)            # IR LED电流
        self.bus.write_byte_data(self.address, REG_LED2_PA, 0x2F)            # RED LED电流
        
        self.bus.write_byte_data(self.address, REG_TEMP_CONFIG, 0x01)        # 配置温度传感器，设置为自动清除

        self.bus.read_byte_data(self.address, REG_INTR_STATUS_1)             # 清除中断状态
        self.bus.read_byte_data(self.address, REG_INTR_STATUS_2)             # 清除中断状态

    def read_fifo(self):
        """
        从FIFO读取红光和红外信号数据
        @return: 红光和红外信号的值
        """
        red_led = None
        ir_led = None

        # 检查数据就绪状态
        temp_data = self.bus.read_byte_data(self.address, REG_INTR_STATUS_1)
        while (temp_data & 0x40) != 0x40:   # 等待数据就绪
            temp_data = self.bus.read_byte_data(self.address, REG_INTR_STATUS_1)

        # 读取FIFO数据
        d = self.bus.read_i2c_block_data(self.address, REG_FIFO_DATA, 6)

        # 解析红外和红光数据
        ir_led = (d[0] << 16 | d[1] << 8 | d[2]) & 0x03FFFF
        red_led = (d[3] << 16 | d[4] << 8 | d[5]) & 0x03FFFF

        return ir_led, red_led

    def low_pass_filter(self, current_ir, current_red):
        # 一阶低通滤波器
        self.last_ir = (1 - self.alpha) * self.last_ir + self.alpha * current_ir
        self.last_red = (1 - self.alpha) * self.last_red + self.alpha * current_red
        return self.last_ir, self.last_red

    def average_filter(self, new_data_ir, new_data_red):
        """
        对新采集的红外和红光信号进行平均滤波
        @param new_data_ir: 新的红外信号
        @param new_data_red: 新的红光信号
        @return: 滤波后的红外和红光信号
        """
        # 更新红外信号的数据缓冲区
        self.data_buffer_ir[self.index_ir] = new_data_ir
        self.index_ir = (self.index_ir + 1) % self.filter_size      # 环形缓冲区

        # 更新红光信号的数据缓冲区
        self.data_buffer_red[self.index_red] = new_data_red
        self.index_red = (self.index_red + 1) % self.filter_size    # 环形缓冲区

        # 计算红外信号的移动平均
        sum_ir = sum(self.data_buffer_ir)
        avg_data_ir = round(sum_ir / self.filter_size, 2)

        # 计算红光信号的移动平均
        sum_red = sum(self.data_buffer_red)
        avg_data_red = round(sum_red / self.filter_size, 2)

        return avg_data_ir, avg_data_red
    
    def low_pass_filter(self, current_ir, current_red):
        """
        一阶低通滤波器
        @param current_ir: 当前红外信号
        @param current_red: 当前红光信号
        @return: 滤波后的红外和红光信号
        """
        self.last_ir = (1 - self.alpha) * self.last_ir + self.alpha * current_ir
        self.last_red = (1 - self.alpha) * self.last_red + self.alpha * current_red
        return self.last_ir, self.last_red

    def get_heart_rate(self, input_data, cache_nums):
        """
        计算心率
        @param input_data: 输入数据数组
        @param cache_nums: 样本数量
        @return: 计算得到的心率
        """ 
        input_data_sum_aver = np.mean(input_data[:cache_nums])
        temp = 0  # 用于存储两个相邻心跳峰值之间的样本数

        peaks = []  # 存储心跳峰值的位置

        # 查找心跳峰值
        for i in range(1, cache_nums - 1):
            if input_data[i] > input_data[i - 1] and input_data[i] > input_data[i + 1] and input_data[i] > input_data_sum_aver:
                peaks.append(i)
        
        # 如果峰值数量较少，无法计算心率
        if len(peaks) < 2:
            return 0

        # 计算相邻峰值之间的间隔
        intervals = []
        for i in range(1, len(peaks)):
            intervals.append(peaks[i] - peaks[i - 1])
        
        # 计算平均间隔
        avg_interval = np.mean(intervals)

        # 根据平均间隔计算心率
        if 14 < avg_interval < 100:  # 限制间隔范围，避免异常数据影响
            # 计算心率（单位：次/分钟）
            return 3000 / avg_interval  # 3000是（200Hz/4） * 60，200Hz为SpO2采样频率，4为FIFO平均过滤值
        else:
            return 0  # 无效心率，返回0

    def get_spo2(self, ir_data, red_data, cache_nums):
        """
        计算血氧
        @param ir_data 红外信号输入数据
        @param red_data 红光信号输入数据
        @param cache_nums: 样本数量
        @return: 计算得到的血氧
        """ 
        ir_data = ir_data[:cache_nums]
        red_data = red_data[:cache_nums]

        ir_max = np.max(ir_data)
        ir_min = np.min(ir_data)
        red_max = np.max(red_data)
        red_min = np.min(red_data)

        # 计算比值 R
        R = ((ir_max - ir_min) * red_min) / ((red_max - red_min) * ir_min)
        return (-45.060 * R * R + 30.354 * R + 94.845)  # 根据比值 R 计算 SpO2 值
