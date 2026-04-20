# mutex_led drivers

运行`make`命令后，将会有一个模块：

* mutex_led.ko

加载驱动程序和内核调试信息：

1. mutex_led.ko

```bash
# insmod mutex_led.ko

    [  145.169694] led platform driver init
    [  145.170026] led platform driver probe
    [  145.170070] GPIO_BASE address: 0xFDD60000
    [  145.170211] major=236, minor=0


# ls /dev/mutex_led

    /dev/mutex_led

# echo 1 > /dev/mutex_led

    [   64.945481] pdrv_led open
    [   64.945608] pdrv_led write

# echo 0 > /dev/mutex_led

    [   62.848713] pdrv_led open
    [   62.848846] pdrv_led write
    [   62.848897] pdrv_led release
```