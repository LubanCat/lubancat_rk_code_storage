# i2c_regmap_api drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* i2c_oled_regmap.ko

* i2c_oled_app

加载驱动程序和内核调试信息：

1. i2c_oled_regmap.ko

```bash
# insmod i2c_oled_regmap.ko

   [  580.832609] i2c_oled driver init
   [  580.836821] i2c_oled driver probe
   [  580.840580] major=236, minor=0


# ./i2c_oled_app /dev/i2c_oled_regmap

   #信息打印如下
   [  583.827056] i2c_oled device open
   设备打开成功，开始显示...
   hello world 显示！
   hello world 显示！
```