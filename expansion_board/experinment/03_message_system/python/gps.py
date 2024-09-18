import serial

class GPS:
    def __init__(self, port, baudrate):
        self.serial = serial.Serial(  
            port=port,  
            baudrate=baudrate,  
            bytesize=serial.EIGHTBITS,      # 数据位  
            parity=serial.PARITY_NONE,      # 校验位  
            stopbits=serial.STOPBITS_ONE,   # 停止位  
            timeout=1                       # 超时时间  
        )

    def __parse_loc_val(self, val, d):  
        v = float(val)/100
        v = int(v) + (v-int(v))*100/60
        if d=='S' or d=='W':
            v = v * -1
        return v

    def read(self):
        lat = lon = 0
        bj_time = "Nil-Nil-Nil,Nil:Nil"

        while True:

            if self.serial.in_waiting > 0:  
            
                raw_byte = self.serial.readline()

                if raw_byte.startswith(b"$"):
                    
                    raw_line = raw_byte.decode("utf-8").strip()
                    # print("raw : ", raw_line)
                    
                    if raw_line.startswith('$GNRMC'):
                        
                        GNRMC = raw_line.split(',') 

                        if GNRMC[0] == '$GNRMC' and GNRMC[2] == 'A':  # 信号有效  
                            
                            # 解析经纬度  
                            lat = round(self.__parse_loc_val(GNRMC[3], GNRMC[4]), 5)  
                            lon = round(self.__parse_loc_val(GNRMC[5], GNRMC[6]), 5)  

                            # 解析utctime并转换为北京时间  
                            # 时分秒
                            utc_time_int = int(GNRMC[1].split('.')[0])  
                            hours = utc_time_int // 10000  
                            minutes = (utc_time_int // 100) % 100  
                            # seconds = utc_time_int % 100  
                            bj_hours = (hours + 8) % 24  

                            # 年月日
                            date_str = GNRMC[9]    
                            day = int(date_str[:2])  
                            month = int(date_str[2:4])  
                            year = int(date_str[4:]) + 2000 

                            bj_time = f"{year}-{month:02d}-{day:02d},{bj_hours:02d}:{minutes:02d}"

                            # 返回解析后的数据  
                            return (str(lat), str(lon), str(bj_time))  
                    else:
                        continue
                else:
                    continue
            else:
                return (str(lat), str(lon), str(bj_time))
    
    def close(self):
        self.serial.close()