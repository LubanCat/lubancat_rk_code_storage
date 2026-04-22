# iio_subsystem drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* iio_mpu6050.ko

* iio_mpu6050_app

加载驱动程序和内核调试信息：

1. iio_mpu6050.ko

```bash
# insmod iio_mpu6050.ko

   [  257.962564] iio-mpu6050 3-0068: MPU6050 init success
   [  257.963151] iio-mpu6050 3-0068: MPU6050 IIO driver probe success


# ls -l /sys/bus/iio/devices/iio:device1

   total 0
   -r--r--r-- 1 root root 4096 Jun 18 04:52 dev
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_accel_x_raw
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_accel_x_scale
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_accel_y_raw
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_accel_y_scale
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_accel_z_raw
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_accel_z_scale
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_anglvel_x_raw
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_anglvel_x_scale
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_anglvel_y_raw
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_anglvel_y_scale
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_anglvel_z_raw
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_anglvel_z_scale
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_temp_raw
   -rw-r--r-- 1 root root 4096 Jun 18 04:52 in_temp_scale
   -r--r--r-- 1 root root 4096 Jun 18 04:52 name
   drwxr-xr-x 2 root root    0 Jun 18 04:52 power
   lrwxrwxrwx 1 root root    0 Jun 18 04:52 subsystem -> ../../bus/iio
   -rw-r--r-- 1 root root 4096 Jun 18 04:51 uevent

# cat /sys/bus/iio/devices/iio:device1/in_accel_x_raw

   -14400

# cat /sys/bus/iio/devices/iio:device1/in_accel_x_scale

   0.000061035

# ./iio_mpu6050_app /sys/bus/iio/devices/iio:device1

   ===== MPU6050 IIO 数据读取程序 =====
   设备路径: /sys/bus/iio/devices/iio:device1
   按 Ctrl+C 退出程序
   ===================================================
   AX(m/s²)   AY(m/s²)   AZ(m/s²)   TEMP(°C)   GX(°/s)    GY(°/s)    GZ(°/s)
   ---------------------------------------------------------------------------
   -8.18      1.08       5.30       27.35      -3.05      0.61       -0.67     //模块静止
   -8.18      1.08       5.31       27.35      -3.11      1.04       -0.43
   -8.17      1.08       5.32       27.34      -2.93      0.79       -0.61
   -8.18      1.07       5.33       27.34      -2.99      0.85       -0.67
   -8.07      1.48       5.28       27.32      0.98       -22.95     -45.35    //开始晃动模块
   -7.93      4.44       4.42       27.34      -8.06      -2.75      -8.06
   -7.86      3.62       4.74       27.36      -32.71     -1.22      -69.89
   -8.20      2.17       5.11       27.39      -13.85     8.73       -6.47
   -8.18      2.00       4.96       27.39      -3.60      0.98       -2.14
```