# timer_led drivers

运行`make`命令后，将会有一个模块：

* timer_led.ko

加载驱动程序和内核调试信息：

1. timer_led.ko

```bash
# insmod timer_led.ko

    [ 4271.958588] led platform driver init
    [ 4271.959173] led platform driver probe
    [ 4271.959280] GPIO_BASE address: 0xFDD60000
    [ 4271.959352] major=241, minor=0

# ls /dev/timer_led

    /dev/timer_led

# echo 0 > /dev/timer_led

    [ 4301.266279] pdrv_led open
    [ 4301.266429] pdrv_led write
    [ 4301.266474] val = 0
    [ 4301.266530] pdrv_led release

# echo 1 > /dev/timer_led

    [ 4303.684530] pdrv_led open
    [ 4303.684680] pdrv_led write
    [ 4303.684714] val = 1

# echo 2 200 > /dev/timer_led

    [ 4289.711000] pdrv_led open
    [ 4289.711119] pdrv_led write
    [ 4289.711143] val = 2
    [ 4289.711160] Timer interval set to 200 ms
    [ 4289.711185] pdrv_led release

# echo 2 500 > /dev/timer_led

    [ 4312.904165] pdrv_led open
    [ 4312.904281] pdrv_led write
    [ 4312.904305] val = 2
    [ 4312.904323] Timer interval set to 500 ms
    [ 4312.904348] pdrv_led release
```