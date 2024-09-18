###############################################
#
#  file: ec11.py
#  update: 2024-08-29
#  usage: 
#      sudo python ec11.py
#
###############################################

import evdev
import threading
import time

ec11_SW_event = '/dev/input/event6'
ec11_A_event = '/dev/input/event5'
ec11_B_event = '/dev/input/event3'

ec11_SW_value = 0
ec11_A_value = 1
ec11_B_value = 1

ec11_direction = 0      # 0:不动作 1:顺时针旋转 2:逆时针旋转 3:按键按下顺时针旋转 4:按键按下逆时针旋转 5:按键按下
lock = threading.Lock() 

def ec11_scan_SW(device):
    global ec11_SW_value, ec11_A_value, ec11_B_value, ec11_direction
    for event in device.read_loop():
        if event.code == 4:
            with lock:
                ec11_SW_value = event.value
                if(ec11_SW_value == 1 and ec11_A_value == 1 and ec11_B_value == 1):     # 按键按下
                    ec11_direction = 5
        elif event.code == 250:
            with lock:
                if event.value == 0 and ec11_B_value == 1:  # 顺时针旋转
                    ec11_A_value = 0            
                    if ec11_SW_value == 1:                  # 按键按下
                        ec11_direction = 3
                    else:
                        ec11_direction = 1
                elif event.value == 1:
                    ec11_A_value = 1
        elif event.code == 252:
            with lock:
                if event.value == 0 and ec11_A_value == 1:  # 逆时针旋转
                    ec11_B_value = 0
                    if ec11_SW_value == 1:                  # 按键按下
                        ec11_direction = 4
                    else:
                        ec11_direction = 2
                elif event.value == 1:
                    ec11_B_value = 1

ec11_SW_device = evdev.InputDevice(ec11_SW_event)
ec11_SW = threading.Thread(target=ec11_scan_SW, args=(ec11_SW_device,))
ec11_SW.daemon = True

ec11_A_device = evdev.InputDevice(ec11_A_event)
ec11_A = threading.Thread(target=ec11_scan_SW, args=(ec11_A_device,))
ec11_A.daemon = True

ec11_B_device = evdev.InputDevice(ec11_B_event)
ec11_B = threading.Thread(target=ec11_scan_SW, args=(ec11_B_device,))
ec11_B.daemon = True

ec11_SW.start()
ec11_A.start()
ec11_B.start()

def main():

    try:
        
        global ec11_direction

        while(1):

            if ec11_direction == 0:  
                pass
            elif ec11_direction == 1:  
                print("顺时针转")  
            elif ec11_direction == 2:  
                print("逆时针转") 
            elif ec11_direction == 3:  
                print("按键按下顺时针转")
            elif ec11_direction == 4:  
                print("按键按下逆时针转")  
            elif ec11_direction == 5:  
                print("按键按下")

            with lock:
                ec11_direction = 0

            time.sleep(0.1)

    except Exception as e:
        
        print("exit...：", e)
        
if __name__ == "__main__":  
    main()



