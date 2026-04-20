# atomic_led drivers

运行`make`命令后，将会有一个模块：

* dts_led.ko

加载驱动程序和内核调试信息：

1. atomic_led.ko

```bash
# insmod atomic_led.ko

    [   60.942158] led platform driver init
    [   60.942468] led platform driver probe
    [   60.942528] GPIO_BASE address: 0xFDD60000
    [   60.942572] major=241, minor=0

# ls /dev/atomic_led

    /dev/atomic_led

# echo 1 > /dev/atomic_led

    [  223.803544] pdrv_led open
    [  223.803700] pdrv_led write
    [  223.803770] pdrv_led release

# echo 0 > /dev/atomic_led

    [  226.399790] pdrv_led open
    [  226.399987] pdrv_led write
    [  226.400028] pdrv_led release

# echo 0 > /dev/atomic_led & echo 1 > /dev/atomic_led

    [  547.342205] pdrv_led open
    [  547.342278] pdrv_led open
    [  547.342353] pdrv_led write
    [  547.342354] pdrv_led write
    [  547.342368] pdrv_led release
    [  547.342379] pdrv_led release
```