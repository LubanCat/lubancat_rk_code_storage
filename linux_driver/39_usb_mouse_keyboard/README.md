# usb_mouse_keyboard

需要屏蔽内核自带的HID驱动，移除CONFIG_USB_HID配置项。

运行`make`命令后，将会有一个模块：

* usb_mouse_keyboard.ko

加载驱动程序和内核调试信息：

1. usb_mouse_keyboard.ko

```bash
#加载驱动
sudo insmod usb_mouse_keyboard.ko

#信息输出如下
[   23.517215] usbcore: registered new interface driver usb-mouse-keyboard

#接入USB鼠标打印如下
[   79.124976] usb 2-1: new low-speed USB device number 2 using xhci-hcd
[   79.270632] usb 2-1: New USB device found, idVendor=1c4f, idProduct=0034, bcdDevice= 1.10
[   79.270757] usb 2-1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[   79.270807] usb 2-1: Product: Usb Mouse
[   79.270848] usb 2-1: Manufacturer: SIGMACHIP
[   79.292692] input: USB Mouse as /devices/platform/usbhost/fd000000.usb/xhci-hcd.4.auto/usb2/2-1/2-1:1.0/input/input5
[   79.293157] usb-mouse-keyboard 2-1:1.0: USB MouseConnected (Interface Number: 0)

#接入USB键盘打印如下
[  102.006640] usb 5-1: new full-speed USB device number 12 using ohci-platform
[  107.438988] usb 5-1: New USB device found, idVendor=24ae, idProduct=4019, bcdDevice= 0.01
[  107.439112] usb 5-1: New USB device strings: Mfr=1, Product=2, SerialNumber=0
[  107.439315] usb 5-1: Product: Rapoo Gaming Keyboard
[  107.439364] usb 5-1: Manufacturer: RAPOO
[  107.443409] input: USB Keyboard as /devices/platform/fd840000.usb/usb5/5-1/5-1:1.0/input/input6
[  107.444899] usb-mouse-keyboard 5-1:1.0: USB KeyboardConnected (Interface Number: 0)

#运行evtest工具
evtest

#信息打印如下，event5对应USB鼠标，event6对应USB键盘
No device specified, trying to scan all of /dev/input/event*
Available devices:
/dev/input/event0:      hdmi_cec_key
/dev/input/event1:      fdd70030.pwm
/dev/input/event2:      rk805 pwrkey
/dev/input/event3:      rockchip-rk809 Headset
/dev/input/event4:      adc-keys
/dev/input/event5:      USB Mouse
/dev/input/event6:      USB Keyboard
Select the device event number [0-6]: 5   //输入数字5，测试USB鼠标
Input driver version is 1.0.1
Input device ID: bus 0x3 vendor 0x1c4f product 0x34 version 0x110
Input device name: "USB Mouse"
Supported events:
Event type 0 (EV_SYN)   //同步事件，SYN_REPORT表示一帧输入事件全部上报完毕，上层应用统一处理
Event type 1 (EV_KEY)   //按键事件，value=1代表按键按下，value=0代表按键释放
      Event code 272 (BTN_LEFT)   //鼠标左键
      Event code 273 (BTN_RIGHT)  //鼠标右键
      Event code 274 (BTN_MIDDLE) //鼠标中键
      Event code 275 (BTN_SIDE)   //鼠标侧键1
      Event code 276 (BTN_EXTRA)  //鼠标侧键2
Event type 2 (EV_REL)   //相对位移事件，value为本次移动增量，正负代表移动方向
      Event code 0 (REL_X)        //X轴水平相对位移
      Event code 1 (REL_Y)        //Y轴垂直相对位移	
      Event code 8 (REL_WHEEL)    //鼠标滚轮垂直滚动
Properties:
Testing ... (interrupt to exit)     
                                                                        //以下按下和释放左键、右键，移动鼠标和滚动滑轮进行测试
//Event: time 时间戳, type 事件类型, code 事件代码, value 事件值
Event: time 1750950448.672039, type 1 (EV_KEY), code 272 (BTN_LEFT), value 1    // 左键按下
Event: time 1750950448.672039, -------------- SYN_REPORT ------------
Event: time 1750950448.752011, type 2 (EV_REL), code 0 (REL_X), value 1         // 向右移动1个单位
Event: time 1750950448.752011, -------------- SYN_REPORT ------------
Event: time 1750950448.824006, type 1 (EV_KEY), code 272 (BTN_LEFT), value 0    // 左键释放
Event: time 1750950448.824006, -------------- SYN_REPORT ------------
Event: time 1750950448.847947, type 2 (EV_REL), code 0 (REL_X), value -1        // 向左移动1个单位
Event: time 1750950448.847947, -------------- SYN_REPORT ------------
Event: time 1750950449.624032, type 1 (EV_KEY), code 273 (BTN_RIGHT), value 1   // 右键按下
Event: time 1750950449.624032, -------------- SYN_REPORT ------------
Event: time 1750950449.735881, type 1 (EV_KEY), code 273 (BTN_RIGHT), value 0   // 右键释放
Event: time 1750950449.735881, -------------- SYN_REPORT ------------
Event: time 1750950452.856012, type 2 (EV_REL), code 0 (REL_X), value -1
Event: time 1750950452.856012, -------------- SYN_REPORT ------------
Event: time 1750950452.896006, type 2 (EV_REL), code 0 (REL_X), value -2        // 向左移动2个单位
Event: time 1750950452.896006, -------------- SYN_REPORT ------------
Event: time 1750950452.903920, type 2 (EV_REL), code 0 (REL_X), value -1
Event: time 1750950452.903920, -------------- SYN_REPORT ------------
Event: time 1750950452.919879, type 2 (EV_REL), code 0 (REL_X), value -2
Event: time 1750950452.919879, type 2 (EV_REL), code 1 (REL_Y), value -1        // 向上移动1个单位
Event: time 1750950452.919879, -------------- SYN_REPORT ------------
Event: time 1750950452.927941, type 2 (EV_REL), code 0 (REL_X), value -1
Event: time 1750950452.927941, type 2 (EV_REL), code 1 (REL_Y), value -1
Event: time 1750950452.927941, -------------- SYN_REPORT ------------
Event: time 1750950454.695988, type 2 (EV_REL), code 8 (REL_WHEEL), value -1    // 滚轮向下滚动1格
Event: time 1750950454.695988, -------------- SYN_REPORT ------------
Event: time 1750950455.744017, type 2 (EV_REL), code 8 (REL_WHEEL), value 1     // 滚轮向上滚动1格
Event: time 1750950455.744017, -------------- SYN_REPORT ------------


#运行evtest工具
evtest

#信息打印如下，event5对应USB鼠标，event6对应USB键盘
No device specified, trying to scan all of /dev/input/event*
Available devices:
/dev/input/event0:      hdmi_cec_key
/dev/input/event1:      fdd70030.pwm
/dev/input/event2:      rk805 pwrkey
/dev/input/event3:      rockchip-rk809 Headset
/dev/input/event4:      adc-keys
/dev/input/event5:      USB Mouse
/dev/input/event6:      USB Keyboard
Select the device event number [0-6]: 6
Input driver version is 1.0.1
Input device ID: bus 0x3 vendor 0x24ae product 0x4019 version 0x1
Input device name: "USB Keyboard"
Supported events:
Event type 0 (EV_SYN)
Event type 1 (EV_KEY)
      Event code 1 (KEY_ESC)
      Event code 2 (KEY_1)
      Event code 3 (KEY_2)
      Event code 4 (KEY_3)
      Event code 5 (KEY_4)
      Event code 6 (KEY_5)
      Event code 7 (KEY_6)
      Event code 8 (KEY_7)
      Event code 9 (KEY_8)
      Event code 10 (KEY_9)
      Event code 11 (KEY_0)
      Event code 12 (KEY_MINUS)
      Event code 13 (KEY_EQUAL)
      Event code 14 (KEY_BACKSPACE)
      Event code 15 (KEY_TAB)
      Event code 16 (KEY_Q)
      Event code 17 (KEY_W)
      Event code 18 (KEY_E)
      Event code 19 (KEY_R)
      Event code 20 (KEY_T)
      Event code 21 (KEY_Y)
      Event code 22 (KEY_U)
      Event code 23 (KEY_I)
      Event code 24 (KEY_O)
      Event code 25 (KEY_P)
      Event code 26 (KEY_LEFTBRACE)
      Event code 27 (KEY_RIGHTBRACE)
      Event code 28 (KEY_ENTER)
      Event code 29 (KEY_LEFTCTRL)
      Event code 30 (KEY_A)
      Event code 31 (KEY_S)
      Event code 32 (KEY_D)
      Event code 33 (KEY_F)
      Event code 34 (KEY_G)
      Event code 35 (KEY_H)
      Event code 36 (KEY_J)
      Event code 37 (KEY_K)
      Event code 38 (KEY_L)
      Event code 39 (KEY_SEMICOLON)
      Event code 40 (KEY_APOSTROPHE)
      Event code 41 (KEY_GRAVE)
      Event code 42 (KEY_LEFTSHIFT)
      Event code 43 (KEY_BACKSLASH)
      Event code 44 (KEY_Z)
      Event code 45 (KEY_X)
      Event code 46 (KEY_C)
      Event code 47 (KEY_V)
      Event code 48 (KEY_B)
      Event code 49 (KEY_N)
      Event code 50 (KEY_M)
      Event code 51 (KEY_COMMA)
      Event code 52 (KEY_DOT)
      Event code 53 (KEY_SLASH)
      Event code 54 (KEY_RIGHTSHIFT)
      Event code 55 (KEY_KPASTERISK)
      Event code 56 (KEY_LEFTALT)
      Event code 57 (KEY_SPACE)
      Event code 58 (KEY_CAPSLOCK)
      Event code 59 (KEY_F1)
      Event code 60 (KEY_F2)
      Event code 61 (KEY_F3)
      Event code 62 (KEY_F4)
      Event code 63 (KEY_F5)
      Event code 64 (KEY_F6)
      Event code 65 (KEY_F7)
      Event code 66 (KEY_F8)
      Event code 67 (KEY_F9)
      Event code 68 (KEY_F10)
      Event code 69 (KEY_NUMLOCK)
      Event code 70 (KEY_SCROLLLOCK)
      Event code 71 (KEY_KP7)
      Event code 72 (KEY_KP8)
      Event code 73 (KEY_KP9)
      Event code 74 (KEY_KPMINUS)
      Event code 75 (KEY_KP4)
      Event code 76 (KEY_KP5)
      Event code 77 (KEY_KP6)
      Event code 78 (KEY_KPPLUS)
      Event code 79 (KEY_KP1)
      Event code 80 (KEY_KP2)
      Event code 81 (KEY_KP3)
      Event code 82 (KEY_KP0)
      Event code 83 (KEY_KPDOT)
      Event code 85 (KEY_ZENKAKUHANKAKU)
      Event code 86 (KEY_102ND)
      Event code 87 (KEY_F11)
      Event code 88 (KEY_F12)
      Event code 89 (KEY_RO)
      Event code 90 (KEY_KATAKANA)
      Event code 91 (KEY_HIRAGANA)
      Event code 92 (KEY_HENKAN)
      Event code 93 (KEY_KATAKANAHIRAGANA)
      Event code 94 (KEY_MUHENKAN)
      Event code 95 (KEY_KPJPCOMMA)
      Event code 96 (KEY_KPENTER)
      Event code 97 (KEY_RIGHTCTRL)
      Event code 98 (KEY_KPSLASH)
      Event code 99 (KEY_SYSRQ)
      Event code 100 (KEY_RIGHTALT)
      Event code 102 (KEY_HOME)
      Event code 103 (KEY_UP)
      Event code 104 (KEY_PAGEUP)
      Event code 105 (KEY_LEFT)
      Event code 106 (KEY_RIGHT)
      Event code 107 (KEY_END)
      Event code 108 (KEY_DOWN)
      Event code 109 (KEY_PAGEDOWN)
      Event code 110 (KEY_INSERT)
      Event code 111 (KEY_DELETE)
      Event code 113 (KEY_MUTE)
      Event code 114 (KEY_VOLUMEDOWN)
      Event code 115 (KEY_VOLUMEUP)
      Event code 116 (KEY_POWER)
      Event code 117 (KEY_KPEQUAL)
      Event code 119 (KEY_PAUSE)
      Event code 121 (KEY_KPCOMMA)
      Event code 122 (KEY_HANGUEL)
      Event code 123 (KEY_HANJA)
      Event code 124 (KEY_YEN)
      Event code 125 (KEY_LEFTMETA)
      Event code 126 (KEY_RIGHTMETA)
      Event code 127 (KEY_COMPOSE)
      Event code 128 (KEY_STOP)
      Event code 129 (KEY_AGAIN)
      Event code 130 (KEY_PROPS)
      Event code 131 (KEY_UNDO)
      Event code 132 (KEY_FRONT)
      Event code 133 (KEY_COPY)
      Event code 134 (KEY_OPEN)
      Event code 135 (KEY_PASTE)
      Event code 136 (KEY_FIND)
      Event code 137 (KEY_CUT)
      Event code 138 (KEY_HELP)
      Event code 140 (KEY_CALC)
      Event code 142 (KEY_SLEEP)
      Event code 150 (KEY_WWW)
      Event code 152 (KEY_SCREENLOCK)
      Event code 158 (KEY_BACK)
      Event code 159 (KEY_FORWARD)
      Event code 161 (KEY_EJECTCD)
      Event code 163 (KEY_NEXTSONG)
      Event code 164 (KEY_PLAYPAUSE)
      Event code 165 (KEY_PREVIOUSSONG)
      Event code 166 (KEY_STOPCD)
      Event code 173 (KEY_REFRESH)
      Event code 176 (KEY_EDIT)
      Event code 177 (KEY_SCROLLUP)
      Event code 178 (KEY_SCROLLDOWN)
      Event code 183 (KEY_F13)
      Event code 184 (KEY_F14)
      Event code 185 (KEY_F15)
      Event code 186 (KEY_F16)
      Event code 187 (KEY_F17)
      Event code 188 (KEY_F18)
      Event code 189 (KEY_F19)
      Event code 190 (KEY_F20)
      Event code 191 (KEY_F21)
      Event code 192 (KEY_F22)
      Event code 193 (KEY_F23)
      Event code 194 (KEY_F24)
Properties:
Testing ... (interrupt to exit)
Event: time 1750951722.693216, type 1 (EV_KEY), code 30 (KEY_A), value 1    //按下键盘A键
Event: time 1750951722.693216, -------------- SYN_REPORT ------------
Event: time 1750951722.779091, type 1 (EV_KEY), code 30 (KEY_A), value 0    //松开键盘A键
Event: time 1750951722.779091, -------------- SYN_REPORT ------------
Event: time 1750951724.861094, type 1 (EV_KEY), code 48 (KEY_B), value 1    //按下键盘B键
Event: time 1750951724.861094, -------------- SYN_REPORT ------------
Event: time 1750951724.939164, type 1 (EV_KEY), code 48 (KEY_B), value 0    //松开键盘B键
Event: time 1750951724.939164, -------------- SYN_REPORT ------------
Event: time 1750951726.073052, type 1 (EV_KEY), code 46 (KEY_C), value 1    //按下键盘C键
Event: time 1750951726.073052, -------------- SYN_REPORT ------------
Event: time 1750951726.162165, type 1 (EV_KEY), code 46 (KEY_C), value 0    //松开键盘C键
Event: time 1750951726.162165, -------------- SYN_REPORT ------------
Event: time 1750951728.184164, type 1 (EV_KEY), code 42 (KEY_LEFTSHIFT), value 1    //按下键盘左SHIFT键
Event: time 1750951728.184164, -------------- SYN_REPORT ------------
Event: time 1750951728.287140, type 1 (EV_KEY), code 42 (KEY_LEFTSHIFT), value 0    //松开键盘左SHIFT键
Event: time 1750951728.287140, -------------- SYN_REPORT ------------
Event: time 1750951728.856174, type 1 (EV_KEY), code 29 (KEY_LEFTCTRL), value 1     //按下键盘左CTRL键
Event: time 1750951728.856174, -------------- SYN_REPORT ------------
Event: time 1750951728.922137, type 1 (EV_KEY), code 29 (KEY_LEFTCTRL), value 0     //松开键盘左CTRL键
Event: time 1750951728.922137, -------------- SYN_REPORT ------------
Event: time 1750951730.739144, type 1 (EV_KEY), code 56 (KEY_LEFTALT), value 1      //按下键盘左ALT键
Event: time 1750951730.739144, -------------- SYN_REPORT ------------
Event: time 1750951730.831123, type 1 (EV_KEY), code 56 (KEY_LEFTALT), value 0      //松开键盘左ALT键
Event: time 1750951730.831123, -------------- SYN_REPORT ------------
Event: time 1750951736.218098, type 1 (EV_KEY), code 28 (KEY_ENTER), value 1        //按下键盘回车键
Event: time 1750951736.218098, -------------- SYN_REPORT ------------
Event: time 1750951736.272955, type 1 (EV_KEY), code 28 (KEY_ENTER), value 0        //松开键盘回车键
Event: time 1750951736.272955, -------------- SYN_REPORT ------------
```