###############################################
#
#  file: pwm.py
#  update: 2024-08-12
#  usage: 
#      sudo python pwm.py
#
###############################################

from periphery import PWM
import time

def main():

    pwm1 = PWM(1, 0)                # pwmchip1, channel0
    pwm2 = PWM(2, 0)                # pwmchip2, channel0

    pwm1.frequency = 50             # 频率 50Hz
    pwm2.frequency = 50

    pwm1.duty_cycle = 0.025         # 占空比（%），范围：0.0-1.0
    pwm2.duty_cycle = 0.025

    pwm1.enable()                   # 使能
    pwm2.enable()

    try:
        while True:
            
            pwm1.duty_cycle = 0.025         # 2.5%（0.5ms），0度
            pwm2.duty_cycle = 0.025

            time.sleep(1)

            pwm1.duty_cycle = 0.05          # 5%（1ms），45度
            pwm2.duty_cycle = 0.05

            time.sleep(1)

            pwm1.duty_cycle = 0.075         # 7.5%（1.5ms），90度
            pwm2.duty_cycle = 0.075

            time.sleep(1)

            pwm1.duty_cycle = 0.1           # 10%（2ms），135度
            pwm2.duty_cycle = 0.1

            time.sleep(1)

            pwm1.duty_cycle = 0.125         # 12.5%（2.5ms），180度
            pwm2.duty_cycle = 0.125

            time.sleep(1)

    except Exception as e:
        
        print("exit...：", e)
    
    finally:

        pwm1.duty_cycle = 0.025         # 2.5%（0.5ms），0度
        pwm2.duty_cycle = 0.025
        
        pwm1.close()
        pwm2.close()
        
if __name__ == "__main__":  
    main()
