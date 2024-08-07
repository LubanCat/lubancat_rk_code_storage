from periphery import PWM
import time
import gpiod

class car_pwm(object):
    global PWMA,PWMB
    global gpio4_C4,gpio3_B0,gpio1_B1,gpio1_B2,gpio1_B0_STYB
    def pwm_init():
        global PWMA,PWMB
        global gpio4_C4,gpio3_B0,gpio1_B1,gpio1_B2
        AIN1 = 20
        AIN2 = 8
        
        BIN1 = 9
        BIN2 = 10
		
        STBY = 8
        #创建了一个chip ID为0的gpiod.Chip对象chip0
        chip0 = gpiod.Chip("4", gpiod.Chip.OPEN_BY_NUMBER)
        chip1 = gpiod.Chip("3", gpiod.Chip.OPEN_BY_NUMBER)
        chip2 = gpiod.Chip("1", gpiod.Chip.OPEN_BY_NUMBER)
        chip3 = gpiod.Chip("1", gpiod.Chip.OPEN_BY_NUMBER)
        chip4 = gpiod.Chip("1", gpiod.Chip.OPEN_BY_NUMBER)
        #设置使用chip0对象的AIN1作为引脚
        gpio4_C4 = chip0.get_line(AIN1)
        gpio3_B0 = chip1.get_line(AIN2)
        gpio1_B1 = chip2.get_line(BIN1)
        gpio1_B2 = chip3.get_line(BIN2)
        gpio1_B0_STYB= chip3.get_line(STBY)

        #设置AIN1、AIN2默认为1、0正转
        gpio4_C4.request(consumer="AIN1", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])
        gpio3_B0.request(consumer="AIN2", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[1])
        gpio1_B1.request(consumer="BIN1", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])
        gpio1_B2.request(consumer="BIN2", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[1])  
        gpio1_B0_STYB.request(consumer="STBY", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[1])  	

        # 定义占空比递增步长
        step = 0.05
        # 定义range最大范围
        rangeMax = int(1/0.05)
        # 打开 PWMA, channel 0 ,对应开发板上PWM8外设
        PWMA = PWM(1, 0)
        PWMB = PWM(2, 0)
        # 设置PWM输出B频率为 1 kHz
        PWMA.frequency = 1e3
        PWMB.frequency = 1e3
        # 设置占空比为 0%，一个周期内高电平的时间与整个周期时间的比例。
    
        PWMA.duty_cycle = 0.2
        PWMB.duty_cycle = 0.2
        # 开启PWM输出
        PWMA.enable()
        PWMB.enable()

    def pwm_set_duty_cycle_and_Diversion(left_pwm,right_pwm):
        global PWMA,PWMB
        global gpio4_C4,gpio3_B0,gpio1_B1,gpio1_B2
        global AIN1,AIN2,BIN1,BIN2
        #if(-1<=left_pwm<=1 and -1<=right_pwm<=1):
        #    if(left_pwm>=0):
        #        PWMA.duty_cycle = left_pwm
        #    else:
        #        PWMA.duty_cycle = -left_pwm
        #    if(right_pwm>=0):
        #        PWMB.duty_cycle = right_pwm
        #    else:
        #        PWMB.duty_cycle = -right_pwm
        #else :
        #    print("value error!")

        if(left_pwm>=0):
            gpio4_C4.set_value(0)
            gpio3_B0.set_value(1)
        else:
            gpio4_C4.set_value(1)
            gpio3_B0.set_value(0)
        if(1-(0.05+abs(left_pwm)*1.17)>0):
            PWMA.duty_cycle=1-(0.05+abs(left_pwm)*1.17)
        else:
            PWMA.duty_cycle=0.0

        if(right_pwm>=0):
            gpio1_B1.set_value(0)
            gpio1_B2.set_value(1)
        else:
            gpio1_B1.set_value(1)
            gpio1_B2.set_value(0) 
        if(1-(0.05+abs(right_pwm)*1.17)>0):
            PWMB.duty_cycle=1-(0.05+abs(right_pwm)*1.17)
        else:
            PWMB.duty_cycle=0.0


    def pwm_set_frequency(A_value,B_value):
        global PWMA,PWMB
        PWMA.frequency = A_value
        PWMB.frequency = B_value

    def pwm_close():
        global PWMA,PWMB
        global gpio4_C4,gpio3_B0,gpio1_B1,gpio1_B2
        # 退出时
        PWMA.duty_cycle = 1.00
        PWMB.duty_cycle = 1.00
        # 释放资源
        PWMA.close()
        PWMB.close()
        gpio4_C4.release()
        gpio3_B0.release()
        gpio1_B1.release()
        gpio1_B2.release()
    
def main():
    try:
        car_pwm.pwm_init()
        while True:
            car_pwm.pwm_set_duty_cycle_and_Diversion(0.2,0.2)
            time.sleep(1)
            car_pwm.pwm_set_duty_cycle_and_Diversion(-0.2,-0.2)
            time.sleep(1)
            #print(abs(-5))
    except:
        car_pwm.pwm_close()
        print("except!\n")


main()



