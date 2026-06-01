# drm_color_bar

drm_color_bar.c为瑞芯微6.1.99内核版本实现DRM双缓冲动态彩条绘制的参考驱动程序。

需将drm_color_bar.c驱动放到 ``内核源码/drivers/gpu/drm/rockchip/`` 目录下进行编译，按要求修改Kconfig、Makefile和板卡对应的defconfig将驱动编译进内核。

加载驱动测试：

1. 默认图层显示

```bash
#查看驱动加载情况
dmesg | grep colorbar

#信息打印如下
[    2.021553] rk-colorbar: driver init, wait VP...
[    3.258247] rk-colorbar: get CRTC0
[    3.258254] rk-colorbar: VP all hardware ready!
[    3.278310] rk-colorbar: VP start, 1s scroll
[    3.872980] rk-colorbar: plane disabled by enable=0

#进入sysfs节点
cd /sys/kernel/rk_colorbar/

#查看节点目录属性
ls -l

#信息输出如下
-rw-rw---- 1 root root 4096  6月 1日 14:47 crtc_index
-rw-rw---- 1 root root 4096  6月 1日 14:47 enable
-rw-rw---- 1 root root 4096  6月 1日 14:47 plane_name

#查看默认参数
sudo cat *

#信息输出如下，默认CRTC索引为0，图层为Esmart0-win0
0
0
Esmart0-win0

#开启显示
sudo sh -c "echo 1 > enable"

#信息打印如下
[  986.939894] rk-colorbar: get CRTC0
[  986.939916] rk-colorbar: VP all hardware ready!

###关闭显示
sudo sh -c "echo 0 > enable"

###信息打印如下
[  986.939944] rk-colorbar: rebind hardware success (plane:Esmart0-win0, crtc:0)
[ 1009.241228] rk-colorbar: plane disabled by enable=0
```

2. 切换图层显示

```bash
#关闭显示
sudo sh -c "echo 0 > enable"

#切换crtc索引
sudo sh -c "echo 1 > crtc_index"

#切换图层
sudo sh -c "echo Esmart1-win0 > plane_name"

#开启显示
sudo sh -c "echo 1 > enable"

#信息打印如下
[ 1153.127784] rk-colorbar: get CRTC1
[ 1153.127801] rk-colorbar: VP all hardware ready!
```
