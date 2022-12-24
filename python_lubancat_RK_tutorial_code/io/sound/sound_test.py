# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
# Copyright (C) 2022 - All Rights Reserved by EmbedFire LubanCat

""" pygame 音频播放测试 """
import sys
import time
import threading

if len(sys.argv) == 2:
    # 从命令行参数中获取文件名
    m_filename = sys.argv[1]
else:
    print(
        """使用方法:
    python3 sound_test.py <music_name.mp3>
    example: python3 sound_test.py test.mp3"""
    )
    sys.exit()

# pylint: disable=C0413
import pygame

exitright = "e"


def get_quitc():
    "获取退出字符"
    # 全局变量
    # pylint: disable=W0603
    global exitright
    # 获取用户输入
    exitright = input("输入字母q后，按下回车以退出播放\n")


try:
    # 初始化音频设备
    pygame.mixer.init()
    # 检查音频设备初始化情况
    if pygame.mixer.get_init() is None:
        raise RuntimeError(
            "音频资源未正确初始化，请检查音频设备是否可用",
        )
    # 加载音频文件，此处未播放mp3文件，因为pygam对MP3格式的播放支持是有限的
    pygame.mixer.music.load(m_filename)
    # 设置音量为100%
    pygame.mixer.music.set_volume(1)
    # 播放音频文件1次
    pygame.mixer.music.play(0)

    # 开启子线程，等待用户输入退出字符
    quitthread = threading.Thread(target=get_quitc, daemon=True)
    quitthread.start()
    # 获取音频播放状态，如果正在播放，等待播放完毕
    while pygame.mixer.music.get_busy():
        # 空闲延时，释放cpu
        if exitright in ("q", "Q"):
            break
        time.sleep(1)
except pygame.error as error:
    print("执行程序时出现异常:", error, "\n等待音频资源被释放")
    # 尝试停止播放
    pygame.mixer.music.stop()
    # 等待音频资源空闲
    while pygame.mixer.music.get_busy():
        time.sleep(1)
finally:
    # 卸载初始化的音频设备
    print("退出播放")
    pygame.mixer.quit()
    sys.exit()
