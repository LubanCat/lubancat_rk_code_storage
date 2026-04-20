# device_model drivers

运行`make`命令后，将会有三个模块：

* xdev.ko
* xbus.ko
* xdrv.ko

加载驱动程序和内核调试信息：

1. xbus.ko

```bash
# insmod xbus.ko

    [ 2974.407715] xbus init

# tree /sys/bus/xbus/

# cat /sys/bus/xbus/xbus_test

    xbus
```


2. xdev.ko

```bash
# insmod xdev.ko

    [ 3144.036761] xdev init

# tree /sys/bus/xbus/
```

3. xdrv.ko

```bash
# insmod xdrv.ko

    [ 3318.564948] xdrv init

# tree /sys/bus/xbus/ 
```