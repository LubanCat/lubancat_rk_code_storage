import time
import board
import busio
import adafruit_mpu6050
import math
import numpy as np
import threading

global k_roll , k_pitch,acc,gyro

class MPU6050(object):

    global i #计算偏移量时的循环次数
    global ax_offset,ay_offset #x,y轴的加速度偏移量
    global gx_offset,gy_offset #x,y轴的角速度偏移量

    #参数
    global rad2deg #弧度到角度的换算系数
    global roll, pitch #储存角度

    #定义微分时间
    global now, lasttime, dt        #now当前，lasttime上一时刻，作差得到时间增量dt

    #三个状态，先验状态，观测状态，最优估计状态
    global gyro_roll, gyro_pitch        #陀螺仪积分计算出的角度，先验估计状态
    global acc_roll, acc_pitch          #加速度计观测出的角度，观测状态
    global k_roll , k_pitch              #卡尔曼滤波后估计出最优角度，最优估计状态

    #误差协方差矩阵P
    global e_P           #误差协方差矩阵，这里的e_P既是先验估计的P，也是最后更新的P

    #卡尔曼增益K
    global k_k           #这里的卡尔曼增益矩阵K是一个2X2的方阵

    global lobal ,i2c, mpu,acc,gyro

    def mpu6050_init():
        global i #计算偏移量时的循环次数
        global ax_offset,ay_offset #x,y轴的加速度偏移量
        global gx_offset,gy_offset #x,y轴的角速度偏移量

        #参数
        global rad2deg #弧度到角度的换算系数
        global roll, pitch #储存角度

        #定义微分时间
        global now, lasttime, dt        #now当前，lasttime上一时刻，作差得到时间增量dt

        #三个状态，先验状态，观测状态，最优估计状态
        global gyro_roll, gyro_pitch        #陀螺仪积分计算出的角度，先验估计状态
        global acc_roll, acc_pitch          #加速度计观测出的角度，观测状态
        global k_roll , k_pitch              #卡尔曼滤波后估计出最优角度，最优估计状态

        #误差协方差矩阵P
        global e_P           #误差协方差矩阵，这里的e_P既是先验估计的P，也是最后更新的P

        #卡尔曼增益K
        global k_k           #这里的卡尔曼增益矩阵K是一个2X2的方阵

        global lobal ,i2c, mpu,acc,gyro

        i2c = busio.I2C(board.I2C5_SCL, board.I2C5_SDA)  # uses board.SCL and board.SDA
        mpu = adafruit_mpu6050.MPU6050(i2c)
    
        #初始化
        i=0
        ax_offset , ay_offset = 0.0,0.0
        gx_offset , gy_offset = 0.0,0.0 
        rad2deg = 57.29578
        now = lasttime , dt = time.time() , 0
        gyro_roll , gyro_pitch = 0.0,0.0      
        acc_roll , acc_pitch = 0.0,0.0         
        k_roll , k_pitch = 0.0,0.0
        e_P=k_k=np.mat(np.random.rand(2,2))
        e_P[0,0],e_P[0,1],e_P[1,0],e_P[1,1] = 1,0,0,1
        k_k[0,0]=k_k[0,1]=k_k[1,0]=k_k[1,1]=0
        for i in range(100):
            acc=mpu.acceleration
            gyro=mpu.gyro
            ax_offset = ax_offset + acc[0] #计算x轴加速度的偏移总量
            ay_offset = ay_offset + acc[1] #计算y轴加速度的偏移总量
            gx_offset = gx_offset + gyro[0] #角速度
            gy_offset = gy_offset + gyro[1]
            ax_offset = ax_offset / 100 #计算x轴加速度的偏移量
            ay_offset = ay_offset / 100 #计算y轴加速度的偏移量
            gx_offset = gx_offset / 100
            gy_offset = gy_offset / 100

    def mpu6050_Kalman():  
        global i #计算偏移量时的循环次数
        global ax_offset,ay_offset #x,y轴的加速度偏移量
        global gx_offset,gy_offset #x,y轴的角速度偏移量
        #参数
        global rad2deg #弧度到角度的换算系数
        global roll, pitch #储存角度
        #定义微分时间
        global now, lasttime, dt        #now当前，lasttime上一时刻，作差得到时间增量dt
        #三个状态，先验状态，观测状态，最优估计状态
        global gyro_roll, gyro_pitch        #陀螺仪积分计算出的角度，先验估计状态
        global acc_roll, acc_pitch          #加速度计观测出的角度，观测状态
        global k_roll , k_pitch              #卡尔曼滤波后估计出最优角度，最优估计状态
        #误差协方差矩阵P
        global e_P           #误差协方差矩阵，这里的e_P既是先验估计的P，也是最后更新的P
        #卡尔曼增益K
        global k_k           #这里的卡尔曼增益矩阵K是一个2X2的方阵
        global lobal ,i2c, mpu,acc,gyro
        now_enter=time.time()

        while (True):
            now_quit=time.time()
            if(now_quit-now_enter>0.2):
                break

            #计算微分时间
            now=time.time() #获取当前时间 S
            dt = now-lasttime
            lasttime = now

            #获取角速度和加速度 
            acc=mpu.acceleration
            gyro=mpu.gyro

            #step1:计算先验状态
            #计算x,y轴上的角速度
            roll_v = (gyro[0]-gx_offset) + ((math.sin(k_pitch)*math.sin(k_roll))/math.cos(k_pitch))*(gyro[1]-gy_offset) + ((math.sin(k_pitch)*math.cos(k_roll))/math.cos(k_pitch))*gyro[2] #roll轴的角速度
            pitch_v = math.cos(k_roll)*(gyro[1]-gy_offset) - math.sin(k_roll)*gyro[2]  #pitch轴的角速度
            gyro_roll = k_roll + dt*roll_v    # 先验roll角度
            gyro_pitch = k_pitch + dt*pitch_v # 先验pitch角度

            #step2:计算先验误差协方差矩阵P
            e_P[0,0] = e_P[0,0] +0.0025 #这里的Q矩阵是一个对角阵且对角元均为0.0025
            e_P[0,1] = e_P[0,1] +0
            e_P[1,0] = e_P[1,0] +0
            e_P[1,1] = e_P[1,1] +0.0025

            #step3:更新卡尔曼增益K
            k_k[0,0] = e_P[0,0]/(e_P[0,0]+0.3)
            k_k[0,1] =0
            k_k[1,0] =0
            k_k[1,1] = e_P[1,1]/(e_P[1,1]+0.3)

            #step4:计算最优估计状态
            #观测状态
            #roll角度
            acc_roll = math.atan((acc[1] - ay_offset) / (acc[2]))*rad2deg
            #pitch角度
            acc_pitch = -1*math.atan((acc[0] - ax_offset) / math.sqrt(math.pow((acc[1] - ay_offset),2) + math.pow((acc[2]),2)))*rad2deg
            #最优估计状态
            k_roll = gyro_roll + k_k[0,0]*(acc_roll - gyro_roll)
            k_pitch = gyro_pitch + k_k[1,1]*(acc_pitch - gyro_pitch)

            #step5:更新协方差矩阵P
            e_P[0,0] = (1 - k_k[0,0])*e_P[0,0]
            e_P[0,1] =0
            e_P[1,0] =0
            e_P[1,1] = (1 - k_k[1,1])*e_P[1,1]
        time.sleep(0.1)
        #print("roll:%.2f,pitch:%.2f"%(k_roll,k_pitch))

    def mpu6050_Kalman_threading():   #mpu6050定时器
        MPU6050.mpu6050_Kalman()

        threading.Timer(0,MPU6050.mpu6050_Kalman_threading).start()

    def mpu6050_close():
        global i2c, mpu
        i2c.deinit()
        #mpu.release()

def main():
    try:
        MPU6050.mpu6050_init()
        MPU6050.mpu6050_Kalman_threading()
        while True:
            #print(666)
            print("Acceleration: X:%.2f, Y: %.2f, Z: %.2f m/s^2"%(acc))
            print("Gyro X:%.2f, Y: %.2f, Z: %.2f degrees/s"%(gyro))
            print("roll:%.2f,pitch:%.2f"%(k_roll , k_pitch))
            print("")
            time.sleep(0.02)
    finally:
        MPU6050.mpu6050_close()
main()