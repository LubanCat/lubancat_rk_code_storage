# platform drivers

运行`make`命令后，将会有两个模块：

* pdev_led.ko
* pdrv_led.ko

加载驱动程序和内核调试信息：

1. pdev_led.ko

```bash
# insmod pdev_led.ko

    [   28.196752] pdev init
```

2. pdrv_led.ko

```bash
# insmod pdrv_led.ko

    [  163.814911] led platform driver init
    [  163.815255] led platform driver probe
    [  163.815335] major=241, minor=0

# ls /dev/pdrv_led

    /dev/pdrv_led

# echo 0 > /dev/pdrv_led

    [  248.524097] pdrv_led open
    [  248.524262] pdrv_led write
    [  248.524313] pdrv_led release

# echo 1 > /dev/pdrv_led

    [  252.850652] pdrv_led open
    [  252.850849] pdrv_led write
    [  252.850911] pdrv_led release
```