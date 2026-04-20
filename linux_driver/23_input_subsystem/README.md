# input_subsystem drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* input_subsystem.ko

* input_subsystem_app

加载驱动程序和内核调试信息：

1. input_subsystem.ko

```bash
# insmod input_subsystem.ko

    [ 5846.950693] led platform driver init
    [ 5846.951231] led platform driver probe
    [ 5846.951367] major=236, minor=0
    [ 5846.952030] input: lubancat-key-input as /devices/platform/led_test/input/input3

# ls /dev/input/by-path/ -l

    total 0
    lrwxrwxrwx 1 root root 9 Jun 18  2024 platform-fdd40000.i2c-platform-rk805-pwrkey-event -> ../event1
    lrwxrwxrwx 1 root root 9 Jun 18  2024 platform-fdd70030.pwm-event -> ../event0
    lrwxrwxrwx 1 root root 9 Apr  3 16:26 platform-led_test-event -> ../event3
    lrwxrwxrwx 1 root root 9 Jun 18  2024 platform-rk-headset-event -> ../event2

# evtest

    No device specified, trying to scan all of /dev/input/event*
    Available devices:
    /dev/input/event0:      fdd70030.pwm
    /dev/input/event1:      rk805 pwrkey
    /dev/input/event2:      rk-headset
    /dev/input/event3:      lubancat-key-input
    Select the device event number [0-3]: 3
    Input driver version is 1.0.1
    Input device ID: bus 0x0 vendor 0x0 product 0x0 version 0x0
    Input device name: "lubancat-key-input"
    Supported events:
    Event type 0 (EV_SYN)
    Event type 1 (EV_KEY)
        Event code 2 (KEY_1)
    Properties:
    Testing ... (interrupt to exit)
    Event: time 1775206083.362980, type 1 (EV_KEY), code 2 (KEY_1), value 1
    Event: time 1775206083.362980, -------------- SYN_REPORT ------------
    Event: time 1775206083.689373, type 1 (EV_KEY), code 2 (KEY_1), value 0
    Event: time 1775206083.689373, -------------- SYN_REPORT ------------
    Event: time 1775206084.992803, type 1 (EV_KEY), code 2 (KEY_1), value 1
    Event: time 1775206084.992803, -------------- SYN_REPORT ------------
    Event: time 1775206085.369374, type 1 (EV_KEY), code 2 (KEY_1), value 0
    Event: time 1775206085.369374, -------------- SYN_REPORT ------------

#./input_subsystem_app /dev/input/event3

    Start reading input event...
    Event: type=1, code=2 value=1
    === 按键按下 ===
    Event: type=1, code=2 value=0
    === 按键松开 ===
    Event: type=1, code=2 value=1
    === 按键按下 ===
    Event: type=1, code=2 value=0
    === 按键松开 ===
```