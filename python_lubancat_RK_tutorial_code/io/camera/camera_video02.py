# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
# Copyright (C) 2022 - All Rights Reserved by EmbedFire LubanCat

import cv2

# 选择播放的视频文件
cap = cv2.VideoCapture('output.avi')

while(cap.isOpened()):
    ret, frame = cap.read()

    # 窗口显示，名为Lubancat_Camera_video
    cv2.imshow('Lubancat_Camera_video',frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()

