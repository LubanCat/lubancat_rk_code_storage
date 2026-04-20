# blockio drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* noblockio.ko

* noblockio_app

加载驱动程序和内核调试信息：

1. noblockio.ko

```bash
# insmod noblockio.ko

    [  235.399876] led platform driver init
    [  235.400457] led platform driver probe
    [  235.400603] major=236, minor=0

# ls /dev/noblockio

    /dev/noblockio

# ./noblockio_app /dev/noblockio

    [  565.982410] pdrv_led open
    [  570.619617] 按键已按下，LED状态翻转
    button_state = 0
    [  572.229723] 按键已按下，LED状态翻转
    button_state = 0
    [  573.036543] 按键已按下，LED状态翻转
    button_state = 0
```