# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
# Copyright (C) 2022 - All Rights Reserved by EmbedFire LubanCat

""" 使用pygame进行屏幕测试 """
import os
import sys
import time
import pygame

class PyScope:
    """ 定义一个PyScope类，进行屏幕测试 """

    screen = None

    def __init__(self):
        "PyScope类的初始化方法，使用framebuffer构造pygame会使用到的图像缓冲区"

        # 设置系统环境为无鼠标模式
        # os.environ["SDL_NOMOUSE"] = "1"
        pygame.display.init()

        # 创建,设置pygame使用的窗口大小
        self.screen = pygame.display.set_mode((600,600))

        # 设置窗口标题
        pygame.display.set_caption('lcd_test2.py')

        # 填充背景色为灰色
        self.screen.fill((156,156,156))

        # 初始化字体库
        pygame.font.init()

        # 使用默认字体显示英文
        font=pygame.font.Font(None,32)
        text=font.render("Hello!",True,(255,255,255))
        self.screen.blit(text,(100,100))

        # 使用默认字体显示文字
        text=font.render("Display Test!",True,(255,0,0))
        self.screen.blit(text,(100,150))

        # 更新屏幕内容
        pygame.display.flip()

    def __del__(self):
        "退出pygame库的时候会调用该方法，可以在此添加资源释放操作"
        pygame.display.quit()

    def test(self):
        while True:
        # 循环获取事件，监听事件
            for event in pygame.event.get():
            # 判断用户鼠标是否点了关闭按钮
                if event.type == pygame.QUIT:
                    # 卸载所有模块
                    pygame.quit()
                    # 终止程序
                    sys.exit()
            # 更新屏幕内容
            #pygame.display.flip()

# 创建一个测试实例，开始测试
scope = PyScope()
# 调用scope类的测试方法
scope.test()

