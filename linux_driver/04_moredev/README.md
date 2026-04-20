# moredev drivers

运行`make`命令后，将会有两个模块和一个应用程序：

* moredev1.ko
* moredev2.ko

* chardev_app

加载驱动程序和内核调试信息：

1. moredev1.ko

```bash
# insmod moredev1.ko

    [   16.325308] chrdev init
    [   16.325360] major=241,minor=0
    [   16.325629] device0 created
    [   16.325809] device1 created

# ls /dev/moredev*

    /dev/moredev0  /dev/moredev1

# ./chardev_app /dev/moredev0

    --------------写入数据--------------
    [   63.462793] chardev open
    [   63.462862] write data: Hello World
    [   63.462862]
    [   63.462890] chardev release
    --------------读取数据--------------
    [   64.463172] chardev open
    data:chardev driver
    [   64.463372] chardev release

# ./chardev_app /dev/moredev1

    --------------写入数据--------------
    [   67.540695] chardev open
    [   67.540771] write data: Hello World
    [   67.540771]
    [   67.540812] chardev release
    --------------读取数据--------------
    [   68.541272] chardev open
    read data:chardev driver
    [   68.541510] chardev release
```

2. moredev2.ko

```bash
# insmod moredev2.ko

    [   36.486664] chrdev init
    [   36.486718] major=241,minor=0
    [   36.487049] device0 created
    [   36.487185] device1 created

# ls /dev/moredev*

    /dev/moredev0  /dev/moredev1

# ./chardev_app /dev/moredev0

    --------------写入数据--------------
    [   76.923322] chardev open
    [   76.923370] write data: Hello World
    [   76.923370]
    [   76.923392] chardev release
    --------------读取数据--------------
    [   77.923903] chardev open
    read data:chardev driver
    [   77.924128] chardev release

# ./chardev_app /dev/moredev1

    --------------写入数据--------------
    [   81.191343] chardev open
    [   81.191417] write data: Hello World
    [   81.191417]
    [   81.191449] chardev release
    --------------读取数据--------------
    [   82.191791] chardev open
    read data:chardev driver
    [   82.191992] chardev release
```
