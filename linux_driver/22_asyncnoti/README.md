# asyncnoti drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* asyncnoti.ko

* asyncnoti_app

加载驱动程序和内核调试信息：

1. asyncnoti.ko

```bash
# insmod asyncnoti.ko

    [ 1080.529177] led platform driver init
    [ 1080.529672] led platform driver probe
    [ 1080.529886] major=241, minor=0
    [ 1080.531367] last_button_state = 1

# ls /dev/asyncnoti

    /dev/asyncnoti

# ./asyncnoti_app /dev/asyncnoti

    [  402.756463] 按键已按下，LED状态翻转
    button_state = 0
    [  403.043123] 按键已松开
    button_state = 1
    [  403.843110] 按键已按下，LED状态翻转
    button_state = 0
    [  404.109927] 按键已松开
    button_state = 1
```
