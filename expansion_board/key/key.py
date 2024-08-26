###############################################
#
#  file: key.py
#  update: 2024-08-12
#  usage: 
#      sudo python key.py
#
###############################################

import evdev

def main():
    try:
        
        device = evdev.InputDevice('/dev/input/event7')
        print(device)
        for event in device.read_loop():
            print(event)

    except Exception as e:
        
        print("exit...：", e)
        
if __name__ == "__main__":  
    main()