# Helloworld drivers

运行`make`命令后，将会有一个模块：

* helloworld.ko

加载驱动程序和内核调试信息：

1. helloworld.ko

```bash
# insmod helloworld.ko

    [ 4239.578811] Hello World Module Init

# rmmod helloworld.ko

    [ 4244.853253] Hello World Module Exit
```
