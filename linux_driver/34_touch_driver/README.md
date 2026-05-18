# touch_driver

运行`make`命令后，将会有一个模块：

* touch_goodix.ko

加载驱动程序和内核调试信息：

1. touch_goodix.ko

```bash
# insmod touch_goodix.ko

   [  337.426992] goodix 1-005d: ID 911, version: 1060
   [  337.446017] input: Goodix Capacitive TouchScreen as /devices/platform/fe5a0000.i2c/i2c-1/1-005d/input/input3

# evtest

   No device specified, trying to scan all of /dev/input/event*
   Available devices:
   /dev/input/event0:      fdd70030.pwm
   /dev/input/event1:      rk805 pwrkey
   /dev/input/event2:      rk-headset
   /dev/input/event3:      Goodix Capacitive TouchScreen
   Select the device event number [0-3]: 3
   Input driver version is 1.0.1
   Input device ID: bus 0x18 vendor 0x416 product 0x38f version 0x1060
   Input device name: "Goodix Capacitive TouchScreen"
   Supported events:
   Event type 0 (EV_SYN)
   Event type 1 (EV_KEY)
      Event code 330 (BTN_TOUCH)
   Event type 3 (EV_ABS)
      Event code 0 (ABS_X)
         Value    333
         Min        0
         Max     1079
      Event code 1 (ABS_Y)
         Value   1770
         Min        0
         Max     1919
      Event code 47 (ABS_MT_SLOT)
         Value      0
         Min        0
         Max        9
      Event code 48 (ABS_MT_TOUCH_MAJOR)
         Value      0
         Min        0
         Max      255
      Event code 50 (ABS_MT_WIDTH_MAJOR)
         Value      0
         Min        0
         Max      255
      Event code 53 (ABS_MT_POSITION_X)
         Value      0
         Min        0
         Max     1079
      Event code 54 (ABS_MT_POSITION_Y)
         Value      0
         Min        0
         Max     1919
      Event code 57 (ABS_MT_TRACKING_ID)
         Value      0
         Min        0
         Max    65535
   Properties:
   Property type 1 (INPUT_PROP_DIRECT)
   Testing ... (interrupt to exit)

# apt update
# apt install libts-bin
# export TSLIB_TSDEVICE=/dev/input/event3
# ts_test
```