from periphery import PWM
import time

try:
    # 定义占空比递增步长
    step = 0.05
    # 定义range最大范围
    rangeMax = int(1/0.05)
    # 打开 PWM 8, channel 0 ,对应开发板上PWM8外设
    pwm = PWM(1, 0)
    # 设置PWM输出频率为 1 kHz
    pwm.frequency = 1e3
    # 设置占空比为 0%，一个周期内高电平的时间与整个周期时间的比例。
    pwm.duty_cycle = 0.00
    # 开启PWM输出
    pwm.enable()
    while True:
        for i in range(0,rangeMax):
            # 休眠step秒
            time.sleep(step)
            # 设置占空比每次加 step% , 使用 round 避免浮点运算误差
            pwm.duty_cycle = round(pwm.duty_cycle+step,2)
        # 常灭1秒
        if pwm.duty_cycle == 0.0:
            time.sleep(1)
        for i in range(0,rangeMax):
            time.sleep(step)
            pwm.duty_cycle = round(pwm.duty_cycle-step,2)
except:
    print("Some errors occur!\n")
finally:
    # 退出时熄灭LED
    pwm.duty_cycle = 0.0
    # 释放资源
    pwm.close()