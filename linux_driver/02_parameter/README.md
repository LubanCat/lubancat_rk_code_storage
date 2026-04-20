# parameter drivers

运行`make`命令后，将会有两个模块：

* parameter.ko
* calculation.ko

加载驱动程序和内核调试信息：

1. parameter.ko

```bash
# insmod parameter.ko

    [ 4690.518085] parameter init!
    [ 4690.518129] itype=0
    [ 4690.518144] btype=0
    [ 4690.518157] ctype=0
    [ 4690.518170] stype=(null)
    [ 4690.518185] *iarr* parameter: 0, 1, 2


# insmod parameter.ko itype=123 btype=1 ctype=200 stype=abc iarr=3,2,1

    [ 4389.909966] parameter init!
    [ 4389.910007] itype=123
    [ 4389.910017] btype=1
    [ 4389.910025] ctype=200
    [ 4389.910032] stype=abc
    [ 4389.910041] *iarr* parameter: 3, 2, 1
```

2. calculation.ko

```bash
# insmod parameter.ko itype=123 btype=1 ctype=200 stype=abc iarr=3,2,1

    [ 4389.909966] parameter init!
    [ 4389.910007] itype=123
    [ 4389.910017] btype=1
    [ 4389.910025] ctype=200
    [ 4389.910032] stype=abc
    [ 4389.910041] *iarr* parameter: 3, 2, 1

# insmod calculation.ko

    [ 5285.919465] calculation init!
    [ 5285.919500] itype+1 = 124, itype-1 = 122
```