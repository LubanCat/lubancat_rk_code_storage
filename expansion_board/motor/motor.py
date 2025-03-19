###############################################
#
#  file: motor.py
#  update: 2024-08-10
#  usage: 
#      sudo python motor.py
#
###############################################

from periphery import PWM
import time
import gpiod

class GPIO:
    global gpio, gpiochip

    def __init__(self, gpionum, gpiochipx, val):
        self.gpiochip = gpiod.Chip(gpiochipx, gpiod.Chip.OPEN_BY_NUMBER)
        self.gpio = self.gpiochip.get_line(gpionum)
        self.gpio.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[val])
    
    def set(self, val):
        self.gpio.set_value(val)

    def release(self):
        self.gpio.release()

# motor controller
# motor controller gpionum
#
# A-D : 0-4
# number = group * 8 + x
# e.g. : B0 = 1 * 8 + 0 = 8
#	     C4 = 2 * 8 + 4 = 20 
#
gpionum_motor_STBY = 8          # GPIO1_B0
gpionum_motor_AIN1 = 20         # GPIO4_C4
gpionum_motor_AIN2 = 8          # GPIO3_B0
gpionum_motor_BIN1 = 9          # GPIO1_B1
gpionum_motor_BIN2 = 10         # GPIO1_B2
# motor controller gpiochip
gpiochip_motor_STBY = "1"       # gpiochip1
gpiochip_motor_AIN1 = "4"       # gpiochip4
gpiochip_motor_AIN2 = "3"       # gpiochip3
gpiochip_motor_BIN1 = "1"       # gpiochip1
gpiochip_motor_BIN2 = "1"       # gpiochip1
# motor controller pwmchip, pwm channel
motor_PWMA = PWM(1, 0)          # pwmchip1, channel0
motor_PWMB = PWM(2, 0)          # pwmchip2, channel0

# motor init
motor_STBY = GPIO(gpionum_motor_STBY, gpiochip_motor_STBY, 0)   # 初始化电机驱动板STBY引脚, 初始电平为低电平，驱动板不工作
motor_AIN1 = GPIO(gpionum_motor_AIN1, gpiochip_motor_AIN1, 0)   # 初始化电机驱动板AIN1引脚, 初始电平为低电平
motor_AIN2 = GPIO(gpionum_motor_AIN2, gpiochip_motor_AIN2, 1)   # 初始化电机驱动板AIN2引脚, 初始电平为高电平
motor_BIN1 = GPIO(gpionum_motor_BIN1, gpiochip_motor_BIN1, 0)   # 初始化电机驱动板BIN1引脚, 初始电平为低电平
motor_BIN2 = GPIO(gpionum_motor_BIN2, gpiochip_motor_BIN2, 1)   # 初始化电机驱动板BIN2引脚, 初始电平为高电平

motor_PWMA.frequency = 1e3          # 频率 1kHz
motor_PWMA.duty_cycle = 0.4         # 占空比（%），范围：0.0-1.0
motor_PWMA.polarity = "normal"      # "normal"：正常极性，"inversed"：反向极性
motor_PWMA.enable()                 # 使能

motor_PWMB.frequency = 1e3
motor_PWMB.duty_cycle = 0.4
motor_PWMB.polarity = "normal"
motor_PWMB.enable()

def main():
    try:
        while True:
            
            motor_STBY.set(1)       # 电机驱动板使能

            time.sleep(2)

            motor_STBY.set(0)       # 电机驱动板关闭

            time.sleep(2)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:
        
        # motor off
        motor_STBY.set(0)
        
        # motor gpio release
        motor_STBY.release()
        motor_AIN1.release()
        motor_AIN2.release()
        motor_BIN1.release()
        motor_BIN2.release()
        
if __name__ == "__main__":  
    main()
