# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
# Copyright (C) 2022 - All Rights Reserved by EmbedFire LubanCat

import os
import cv2

# 帧宽，高度，帧率
width = 640
height = 480
fps = 25.0

video = cv2.VideoCapture(0)

# 设置帧宽和高度
video.set(cv2.CAP_PROP_FRAME_WIDTH, width)
video.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

# 定义视频文件的写入格式,未压缩的YUV颜色编码类型(长时间运行该程序，保存的视频文件很大)
# fourcc = cv2.VideoWriter_fourcc(*'MJPG')
fourcc = cv2.VideoWriter_fourcc('I','4','2','0')

# 用于实现多张图像保存成视频文件,第一个参数是需要保存的视频文件名称，第二个函数是编解码器的代码，
# 第三个参数为保存视频的帧率,第四个参数是保存的视频文件的尺寸,一定要与图像的尺寸相同
# out = cv2.VideoWriter('output.mp4',fourcc, fps, (width,height))
out = cv2.VideoWriter('output.avi',fourcc, fps, (width,height))

while(video.isOpened()):
    ret, frame = video.read()
    if ret is True:
        # 窗口显示，名为Lubancat_Camera_video
        cv2.imshow('Lubancat_Camera_video',frame)

        # 帧图像翻转
        frame=cv2.flip(frame,1)
        # 将捕捉到的图像存储，保存的视频是没有声音的
        out.write(frame)

        # 延迟1ms，如果键盘按下“q”，退出
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    else:
        print("Unable to read camera!")
        break

# 释放资源
video.release()
out.release()
cv2.destroyAllWindows()

