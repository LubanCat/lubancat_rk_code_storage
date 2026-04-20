# device_tree drivers

运行`make`命令后，将会有一个模块：

* get_dts_info.ko

加载驱动程序和内核调试信息：

1. get_dts_info.ko

```bash
# insmod get_dts_info.ko

    [  180.227460] get_dts_info_probe
    [  180.227548] name: get_dts_info_test
    [  180.227551] child name: led
    [  180.227561] name: led
    [  180.227569] parent name: get_dts_info_test
    [  180.227578] size = : 14
    [  180.227587] name: compatible
    [  180.227595] length: 14
    [  180.227611] value : fire,led_test
    [  180.227620] 0xFDD60000
```
