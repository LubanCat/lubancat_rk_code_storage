# blockio drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* blockio.ko

* blockio_app

加载驱动程序和内核调试信息：

1. blockio.ko

```bash
# insmod blockio.ko

    [32587.054367] led platform driver init
    [32587.054991] led platform driver probe
    [32587.055124] major=236, minor=0

# ls /dev/blockio

    /dev/blockio

# ./blockio_app /dev/blockio

    [  306.822824] pdrv_led open
    [  314.431509] 按键已按下，LED状态翻转
    button_state = 0
    [  314.541511] 按键已按下，LED状态翻转
    button_state = 0
    [  316.618298] 按键已按下，LED状态翻转
    button_state = 0
```