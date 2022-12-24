# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
# Copyright (C) 2022 - All Rights Reserved by EmbedFire LubanCat

import os
import cv2

# 帧宽和高度
width = 640
height = 480

num = 1

# 创建一个VideoCapture对象，并打开系统默认的摄像头（也可以打开视频或者指定的设备）
cap = cv2.VideoCapture(0)

# 设置帧宽和高度
cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

while(cap.isOpened()):

    # 返回两个参数，ret表示是否正常打开，frame是图像数组,一帧
    ret,frame  = cap.read()

    # 窗口显示，名为Lubancat_Camera_test
    cv2.imshow("Lubancat_Camera_test", frame)

    # 延迟1ms，并根据键盘输入返回值val
    val = cv2.waitKey(1) & 0xFF

    # 捕获到键盘输入‘s’,图片保存到当前目录下
    if val == ord('s'):
        print("=======================================")
        # 第一个参数是保存为的图片名，第二个参数为待保存图像
        cv2.imwrite("photo" + str(num) + ".jpg", frame)
        print("width = ", width)
        print("height = ", height)
        print("success to save photo: "'photo' + str(num)+".jpg")
        print("=======================================")
        num += 1

    # 若检测到按键 ‘q’，退出
    if val == ord('q'):
        break

# 释放摄像头
cap.release()
# 关闭窗口
cv2.destroyAllWindows()
