###############################################
#
#  file: gpio.py
#  update: 2024-08-12
#  usage: 
#      sudo python gpio.py
#
###############################################

import time
import gpiod

gpionum8 = 8
gpionum9 = 9

gpiochip6 = gpiod.Chip("6", gpiod.Chip.OPEN_BY_NUMBER)

# output set
gpio6_8 = gpiochip6.get_line(gpionum8)
gpio6_8.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])

# input set
gpio6_9 = gpiochip6.get_line(gpionum9)
gpio6_9.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_IN)

def main():
    try:
        while True:
            
            gpio6_8.set_value(1)
            print(gpio6_9.get_value())

            time.sleep(1)

            gpio6_8.set_value(0)
            print(gpio6_9.get_value())

            time.sleep(1)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        gpio6_8.release()
        gpio6_9.release()
        
if __name__ == "__main__":  
    main()