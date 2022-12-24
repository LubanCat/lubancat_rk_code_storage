# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
# Copyright (C) 2022 - All Rights Reserved by EmbedFire LubanCat

""" 使用pygame进行屏幕显示测试 """
import os
import sys
import time
import pygame

# 设置系统环境，无鼠标
os.environ["SDL_NOMOUSE"] = "1"

# 初始化显示设备
pygame.display.init()

# 设置显示窗口的范围为屏幕大小，并返回一个屏幕对象
screen = pygame.display.set_mode((448,418))

# 设置窗口名称
pygame.display.set_caption("test")

# 使用pygame image模块加载图片
ball = pygame.image.load("test.png")

# 将图片按位拷贝到窗口
screen.blit(ball,(0,0))

# 刷新窗口
pygame.display.flip()

time.sleep(5)
sys.exit()