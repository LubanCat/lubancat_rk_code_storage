# chardev_led drivers

运行`make`命令后，将会有一个模块：

* chardev_led.ko

加载驱动程序和内核调试信息：

1. chardev_led.ko

```bash
# insmod chardev_led.ko

    [  436.029413] chrdev init
    [  436.029504] major=241, minor=0

# ls /dev/chardev_led*

    /dev/chardev_led

# echo 0 > /dev/chardev_led

    [  479.955744] chardev_led open
    [  479.955908] chardev_led write
    [  479.955979] chardev_led release

# echo 1 > /dev/chardev_led

    [  483.122486] chardev_led open
    [  483.122617] chardev_led write
    [  483.122662] chardev_led release
```
