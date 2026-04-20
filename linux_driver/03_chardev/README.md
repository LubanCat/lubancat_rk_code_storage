# chardev drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* chardev.ko

* chardev_app

加载驱动程序和内核调试信息：

1. chardev.ko

```bash
# insmod chardev.ko

    [   16.325308] chrdev init
    [   16.325360] major=236,minor=0
    [   16.325629] device created

# ls -l /dev/chardev

    crw------- 1 root root 236, 0 Mar 20 14:55 /dev/chardev

# ./chardev_app /dev/chardev

    --------------写入数据--------------
    [   63.462793] chardev open
    [   63.462862] write data: Hello World
    [   63.462862]
    [   63.462890] chardev release
    --------------读取数据--------------
    [   64.463172] chardev open
    data:chardev driver
    [   64.463372] chardev release
```
