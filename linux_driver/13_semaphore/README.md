# semaphore_led drivers

运行`make`命令后，将会有一个模块：

* semaphore_led.ko

加载驱动程序和内核调试信息：

1. semaphore_led.ko

```bash
# insmod semaphore_led.ko

    [   60.942158] led platform driver init
    [   60.942468] led platform driver probe
    [   60.942528] GPIO_BASE address: 0xFDD60000
    [   60.942572] major=241, minor=0

# ls /dev/semaphore_led

    /dev/semaphore_led

# echo 1 > /dev/semaphore_led

    [   64.945481] pdrv_led open
    [   64.945608] pdrv_led write

# echo 0 > /dev/semaphore_led

    [   62.848713] pdrv_led open
    [   62.848846] pdrv_led write
    [   62.848897] pdrv_led release

# echo 0 > /dev/spinlock_led & echo 1 > /dev/spinlock_led
    [ 5852.782952] pdrv_led open
    [ 5852.782953] pdrv_led open
    [ 5852.783027] pdrv_led write
    [ 5852.783097] pdrv_led write
    [ 5852.783105] pdrv_led release
    [ 5852.783114] pdrv_led release
```