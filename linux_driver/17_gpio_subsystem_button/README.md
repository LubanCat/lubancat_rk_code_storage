# gpio_subsystem drivers

运行`make`命令后，将会有1个模块：

* gpio_subsystem_button.ko

加载驱动程序和内核调试信息：

1. gpio_subsystem_button.ko

```bash
# insmod gpio_subsystem_button.ko

    [  909.260201] led platform driver init
    [  909.260810] led platform driver probe
    [  909.261041] major=241, minor=0
    [  909.261431] last_button_state = 1


# ls /dev/gpio_subsystem_button

    /dev/gpio_subsystem_button

# echo 0 > /dev/gpio_subsystem_button

    [  108.117275] pdrv_button open
    [  108.117377] pdrv_button write
    [  108.117400] val = 0
    [  108.117429] pdrv_button release

# echo 2 200 > /dev/gpio_subsystem_button

    [  195.861583] pdrv_led open
    [  195.861950] pdrv_led write
    [  195.862001] val = 2
    [  195.862021] Timer interval set to 200 ms
    [  195.862053] pdrv_led release

# echo 1 > /dev/gpio_subsystem_button

    [  115.503787] pdrv_button open
    [  115.503941] pdrv_button write
    [  115.503979] val = 1
    [  115.504034] pdrv_button release

# 按下和松开按键

    [  209.795942] 按键已按下
    [  209.999300] 按键已松开
    [  210.199296] 按键已按下
    [  210.609310] 按键已松开
    [  212.042821] 按键已按下
    [  212.446097] 按键已松开
```