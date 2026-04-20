# gpio_subsystem drivers

运行`make`命令后，将会有一个模块：

* gpio_subsystem.ko

加载驱动程序和内核调试信息：

1. gpio_subsystem.ko

```bash
# insmod gpio_subsystem.ko

    [   73.925730] led platform driver init
    [   73.926191] led platform driver probe
    [   73.926290] major=241, minor=0

# ls /dev/gpio_subsystem

    /dev/gpio_subsystem

# echo 0 > /dev/gpio_subsystem

    [  108.117275] pdrv_led open
    [  108.117377] pdrv_led write
    [  108.117400] val = 0
    [  108.117429] pdrv_led release

# echo 1 > /dev/gpio_subsystem

    [  115.503787] pdrv_led open
    [  115.503941] pdrv_led write
    [  115.503979] val = 1
    [  115.504034] pdrv_led release


# echo 2 200 > /dev/gpio_subsystem

    [ 4289.711000] pdrv_led open
    [ 4289.711119] pdrv_led write
    [ 4289.711143] val = 2
    [ 4289.711160] Timer interval set to 200 ms
    [ 4289.711185] pdrv_led release

# echo 2 500 > /dev/gpio_subsystem

    [ 4312.904165] pdrv_led open
    [ 4312.904281] pdrv_led write
    [ 4312.904305] val = 2
    [ 4312.904323] Timer interval set to 500 ms
    [ 4312.904348] pdrv_led release
```
