import threading

class RingBuffer:
    def __init__(self, size):
        self.size = size
        self.buffer = [None] * size
        self.read_index = 0
        self.write_index = 0
        self.count = 0
        self.mutex = threading.Lock()
        self.write_event = threading.Event()
        self.read_event = threading.Event()

    def is_full(self):
        """
        判断环形缓冲区是否已满
        :return: 如果已满返回 True，否则返回 False
        """
        return self.count == self.size

    def is_empty(self):
        """
        判断环形缓冲区是否为空
        :return: 如果为空返回 True，否则返回 False
        """
        return self.count == 0

    def write(self, data):
        """
        向环形缓冲区写入数据
        :param data: 要写入的数据
        :return: 写入成功返回 0，缓冲区已满返回 -1
        """
        with self.mutex:
            while self.is_full():
                self.write_event.clear()
                self.read_event.set()
                self.mutex.release()
                self.write_event.wait()
                self.mutex.acquire()
            self.buffer[self.write_index] = data
            self.write_index = (self.write_index + 1) % self.size
            self.count += 1
            self.read_event.set()
            return 0

    def read(self):
        """
        从环形缓冲区读取数据
        :return: 读取到的数据，如果缓冲区为空返回 None
        """
        with self.mutex:
            while self.is_empty():
                self.read_event.clear()
                self.write_event.set()
                self.mutex.release()
                self.read_event.wait()
                self.mutex.acquire()
            data = self.buffer[self.read_index]
            self.read_index = (self.read_index + 1) % self.size
            self.count -= 1
            self.write_event.set()
            return data