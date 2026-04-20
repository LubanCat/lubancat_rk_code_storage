# irq_layering drivers

运行`make`命令后，将会有一个模块：

* irq_layering.ko

加载驱动程序和内核调试信息：

1. irq_layering.ko

```bash
# insmod irq_layering.ko

    [   70.570580] led platform driver init
    [   70.571410] led platform driver probe
    [   70.571607] major=236, minor=0

# cat /proc/interrupts | grep button_irq

    105:       1611          0          0          0     gpio1  10 Edge      button_irq

# 按下和松开按键

    [19731.978896] 按键已按下，LED状态翻转
    [19731.979072] 线程化中断底半部执行，按键有效按下总次数：1
    [19731.979127] tasklet回调函数执行
    [19732.182438] 延时工作队列回调函数执行，开始耗时打印
    [19732.388898] irq_thread counter = 1
    [19732.598878] irq_thread counter = 2
    [19732.808953] irq_thread counter = 3
    [19733.018919] irq_thread counter = 4
    [19733.225615] irq_thread counter = 5
    [19740.385983] 按键已按下，LED状态翻转
    [19740.386035] 线程化中断底半部执行，按键有效按下总次数：2
    [19740.386070] tasklet回调函数执行
    [19740.589465] 延时工作队列回调函数执行，开始耗时打印
    [19740.796062] irq_thread counter = 1
    [19741.002777] irq_thread counter = 2
    [19741.209358] irq_thread counter = 3
    [19741.416058] irq_thread counter = 4
    [19741.622879] irq_thread counter = 5
    [19743.469522] 按键已按下，LED状态翻转
    [19743.469648] 线程化中断底半部执行，按键有效按下总次数：3
    [19743.469693] tasklet回调函数执行
    [19743.672986] 延时工作队列回调函数执行，开始耗时打印
    [19743.879505] irq_thread counter = 1
    [19744.086271] irq_thread counter = 2
    [19744.292867] irq_thread counter = 3
    [19744.499581] irq_thread counter = 4
    [19744.706197] irq_thread counter = 5
```