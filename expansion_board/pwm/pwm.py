from periphery import PWM
import time

try:
    # 定义占空比递增步长
    step = 0.05
    # 定义range最大范围
    rangeMax = int(1/0.05)
    # 打开对应开发板上PWM外设
    pwm1 = PWM(1,0)
    pwm2 = PWM(2,0)
    # 设置PWM输出频率为 1 kHz
    pwm1.frequency = 1e3
    pwm2.frequency = 1e3
    # 设置占空比为 0%，一个周期内高电平的时间与整个周期时间的比例。
    pwm1.duty_cycle = 0.00
    pwm2.frequency = 1e3
    # 开启PWM输出
    pwm1.enable()
    pwm2.enable()
    while True:
        for i in range(0,rangeMax):
            # 休眠step秒
            time.sleep(step)
            # 设置占空比每次加 step% , 使用 round 避免浮点运算误差
            pwm1.duty_cycle = round(pwm1.duty_cycle+step,2)
            pwm2.duty_cycle = round(pwm2.duty_cycle+step,2)
        # 常灭1秒
        if pwm1.duty_cycle == 0.0:
            time.sleep(1)
        for i in range(0,rangeMax):
            time.sleep(step)
            pwm1.duty_cycle = round(pwm1.duty_cycle-step,2)
            pwm2.duty_cycle = round(pwm2.duty_cycle-step,2)
except:
    print("Some errors occur!\n")
finally:
    # 退出时
    pwm1.duty_cycle = 0.0
    pwm2.duty_cycle = 0.0
    # 释放资源
    pwm1.close()
    pwm2.close()
