import json  
import os

class ConfigManager:  
    def __init__(self, config_file):  
        self.config_file = config_file  
        self.config = None  
        self.board_name = None
        self.__load_config()  
  
    def __load_config(self):  
        try:  
            with open(self.config_file, 'r') as file:  
                self.config = json.load(file)  
        except FileNotFoundError:  
            print(f"Configuration file {self.config_file} not found.")  
            self.config = None  
    
    def __get_board_name_from_devicetree(self, devicetree_model_path='/proc/device-tree/model'):  

        if os.path.exists(devicetree_model_path):  
            try:  
                with open(devicetree_model_path, 'r') as file:  
                    model_info = file.read().strip()  
                    board_name = model_info.split('EmbedFire ', 1)[-1]   
                    return board_name  
            except Exception as e:  
                print(f"Failed to read or parse model from device tree: {e}")  
        return None
    
    def inspect_current_environment(self):
        if self.config is not None:
            self.board_name = self.__get_board_name_from_devicetree().strip('\0')
            if self.board_name is not None:
                print(f"The board name : {self.board_name}")
            else:
                return False

            if self.get_board_config() is not None:
                print(f"Successfully Find the {self.board_name} property in {self.config_file}!")
            else:
                print(f"Error: Can not find the {self.board_name} property in {self.config_file}!")
                return False
            return True

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
  
