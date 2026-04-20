# device_tree_overlays drivers

运行`make`命令后，将会有一个模块：

* dts_led.ko

加载驱动程序和内核调试信息：

1. dts_led.ko

```bash
# insmod dts_led.ko

    [   60.942158] led platform driver init
    [   60.942468] led platform driver probe
    [   60.942528] GPIO_BASE address: 0xFDD60000
    [   60.942572] major=241, minor=0

# ls /dev/dts_led

    /dev/dts_led

# echo 1 > /dev/dts_led

    [   64.945481] pdrv_led open
    [   64.945608] pdrv_led write

# echo 0 > /dev/dts_led

    [   62.848713] pdrv_led open
    [   62.848846] pdrv_led write
    [   62.848897] pdrv_led release
```