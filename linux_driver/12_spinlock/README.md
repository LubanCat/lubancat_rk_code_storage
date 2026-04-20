# spinlock_led drivers

运行`make`命令后，将会有一个模块：

* spinlock_led.ko

加载驱动程序和内核调试信息：

1. spinlock_led.ko

```bash
# insmod spinlock_led.ko

    [   60.942158] led platform driver init
    [   60.942468] led platform driver probe
    [   60.942528] GPIO_BASE address: 0xFDD60000
    [   60.942572] major=241, minor=0

# ls /dev/spinlock_led

    /dev/spinlock_led

# echo 1 > /dev/spinlock_led

    [   64.945481] pdrv_led open
    [   64.945608] pdrv_led write

# echo 0 > /dev/spinlock_led

    [   62.848713] pdrv_led open
    [   62.848846] pdrv_led write
    [   62.848897] pdrv_led release

# echo 0 > /dev/spinlock_led & echo 1 > /dev/spinlock_led
    [ 4969.517237] pdrv_led open
    [ 4969.517239] pdrv_led open
    [ 4969.517310] pdrv_led write
    [ 4969.517381] pdrv_led write
    [ 4969.517387] pdrv_led release
    [ 4969.517398] pdrv_led release
```