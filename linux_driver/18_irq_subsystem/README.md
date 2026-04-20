# irq_subsystem drivers

运行`make`命令后，将会有一个模块：

* irq_subsystem.ko

加载驱动程序和内核调试信息：

1. irq_subsystem.ko

```bash
# insmod irq_subsystem.ko

    [   70.570580] led platform driver init
    [   70.571410] led platform driver probe
    [   70.571607] major=236, minor=0

# cat /proc/interrupts | grep button_irq

    105:        349          0          0          0     gpio1  10 Edge      button_irq

# 按下和松开按键

    [   83.795412] 按键已按下，LED状态翻转
    [   84.868826] 按键已按下，LED状态翻转
    [   86.078850] 按键已按下，LED状态翻转
    [   87.078942] 按键已按下，LED状态翻转
```