import json  
import os
import re

class ConfigManager:  
    def __init__(self, config_file):  
        self.config_file = config_file  
        self.config = None  
        self.board_name = None
        self.__load_config()  
  
    # 加载配置文件
    def __load_config(self):  
        try:  
            with open(self.config_file, 'r') as file:  
                self.config = json.load(file)  
        except FileNotFoundError:  
            print(f"Configuration file {self.config_file} not found.")  
            self.config = None  
    
    # 从设备树中获取板卡名
    def __get_board_name_from_devicetree(self, devicetree_model_path='/proc/device-tree/compatible'):  

        if os.path.exists(devicetree_model_path):  
            try:  
                with open(devicetree_model_path, 'r') as file: 
                    model_info = file.read().strip()  
                    parts = model_info.split(',')

                    for part in parts:  
                        match = re.search(r'lubancat-(.*?)(?=rockchip|$)', part)  
                        if match:  
                            self.board_name = match.group(0)[:-1]
                            return self.board_name
                        
            except Exception as e:  
                print(f"Failed to read or parse model from device tree: {e}")  
        else:
            print(f"can not find path {devicetree_model_path}")

        return None
    
    # 检查配置文件中是否存在目标板卡配置
    def inspect_current_environment(self):
        if self.config is not None:
            self.board_name = self.__get_board_name_from_devicetree()
            print(f"The board name : {self.board_name}")
            if self.board_name is None:
                print("can not get the board name in devicetree !")
                return False

            if self.get_board_config() is not None:
                print(f"Successfully Find the {self.board_name} property in {self.config_file}!")
            else:
                print(f"Error: Can not find the {self.board_name} property in {self.config_file}!")
                return False
            return True

    # 获取配置
    def get_board_config(self, sub_config_name=None):  
        if self.board_name in self.config:  
            if sub_config_name is None:  
                return self.config[self.board_name]  
            elif sub_config_name in self.config[self.board_name]:  
                return self.config[self.board_name][sub_config_name]  
            else:  
                return None  
        else:  
            return None  
  
