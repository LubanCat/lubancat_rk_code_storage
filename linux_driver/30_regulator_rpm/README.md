# regulator_rpm drivers

运行`make`命令后，将会有一个模块：

* regulator_rpm.ko

加载驱动程序和内核调试信息：

1. regulator_rpm.ko

```bash
# insmod regulator_rpm.ko

   [  275.097869] regulator_rpm driver probe
   [  275.098152] regulator_rpm regulator-rpm: Linked as a consumer to regulator.19
   [  275.101985] PM suspend: regulator OFF


# cd /sys/class/regulator-rpm/regulator-test/

# ls -l

   total 0
   lrwxrwxrwx 1 root root    0 Apr 20 16:08 device -> ../../../regulator-rpm
   drwxr-xr-x 2 root root    0 Apr 20 16:08 power
   -rw-r--r-- 1 root root 4096 Apr 20 16:08 power_state
   lrwxrwxrwx 1 root root    0 Apr 20 16:08 subsystem -> ../../../../../class/regulator-rpm
   -rw-r--r-- 1 root root 4096 Apr 20 15:43 uevent

# echo 1 > power_state

   [ 2019.654317] PM resume: regulator ON
   [ 2020.656894] PM suspend: regulator OFF
```