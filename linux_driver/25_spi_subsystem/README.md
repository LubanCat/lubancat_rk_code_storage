# spi_subsystem drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* spi_oled.ko

* spi_oled_app

加载驱动程序和内核调试信息：

1. spi_oled.ko

```bash
# insmod spi_oled.ko

   [   44.026150] spi_oled driver init
   [   44.026544] spi_oled driver probe
   [   44.026652] major=236, minor=0
   [   44.027456] SPI max_speed: 2000000Hz
   [   44.027571] SPI mode: 0x00
   [   44.027633] SPI chip_select = 0
   [   44.027700] SPI bits_per_word = 8

# ./spi_oled_app /dev/spi_oled

   #信息打印如下
   [  344.342242] spi_oled device open
   设备打开成功，开始显示...
   hello world 显示！
   hello world 显示！
```