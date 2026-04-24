# adc_driver

运行`make`命令后，将会有一个模块和一个应用程序：

* adc_ads1115.ko

* adc_ads1115_app

加载驱动程序和内核调试信息：

1. adc_ads1115.ko

```bash
# insmod adc_ads1115.ko

   [ 8820.282036] adc-ads1115 3-0048: ADS1115: PGA=6144mV, DataRate=128SPS
   [ 8820.282693] adc-ads1115 3-0048: ADS1115 IIO driver probe successful


# ls -l /sys/bus/iio/devices/iio:device1

   total 0
   -r--r--r-- 1 root root 4096 Apr 24 11:55 dev
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage0_raw
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage0_scale
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage1_raw
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage1_scale
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage2_raw
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage2_scale
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage3_raw
   -rw-r--r-- 1 root root 4096 Apr 24 11:55 in_voltage3_scale
   -r--r--r-- 1 root root 4096 Apr 24 11:54 name
   drwxr-xr-x 2 root root    0 Apr 24 11:55 power
   lrwxrwxrwx 1 root root    0 Apr 24 11:55 subsystem -> ../../bus/iio
   -rw-r--r-- 1 root root 4096 Apr 24 11:54 uevent

# cat /sys/bus/iio/devices/iio:device1/in_voltage0_raw

   17602

# cat /sys/bus/iio/devices/iio:device1/in_voltage0_scale

   0.187500000

# ./adc_ads1115_app /sys/bus/iio/devices/iio:device1

   ===== ADS1115 4通道独立读取程序 =====
   设备路径: /sys/bus/iio/devices/iio:device1
   按 Ctrl+C 退出
   -----------------------------------------
   CH0(V)  CH1(V)  CH2(V)  CH3(V)
   -----------------------------------------
   3.30    5.10    -0.00   0.54
   3.30    5.09    -0.00   0.54
   3.30    5.10    -0.00   0.54
   3.30    5.09    -0.00   0.54
   3.30    5.09    -0.00   0.54
   3.30    5.10    -0.00   0.54
```