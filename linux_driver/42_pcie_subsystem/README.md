# pcie_subsystem

运行`make`命令后，将会有三个模块：

* pcie_explorer.ko
* pcie_dma.ko
* pcie_irq.ko

加载驱动程序和内核调试信息：

1. pcie_explorer.ko

```bash

#查看网卡的VID和PID
lspci -nn

#信息输出如下
00:00.0 PCI bridge [0604]: Rockchip Electronics Co., Ltd RK3568 Remote Signal Processor [1d87:3566] (rev 01)
01:00.0 Network controller [0280]: Realtek Semiconductor Co., Ltd. RTL8821CE 802.11ac PCIe Wireless Network Adapter [10ec:c821]

#4.19.232内核系统卸载8821ce模块
sudo rmmod 8821ce

#6.1.99内核系统卸载rtw88_8821ce模块
sudo rmmod rtw88_8821ce

#加载实验驱动
sudo insmod pcie_explorer.ko

#信息输出如下
[   31.814097] pcie_explorer: 发现设备 0000:01:00.0
[   31.814166] ------------ 配置空间解析 ------------
[   31.814254]       PCIe 配置空间
[   31.814266]   Vendor: 0x10EC  Device: 0xC821                 #VID和PID和lspci读取到的一致
[   31.814277]   Revision: 0x00  Header: 0x00 Single-function   #芯片修订版本A0，普通设备，单功能
[   31.814454]   Class: 0x02800000 (IF=0x00 Sub=0x80 Base=0x02) #IF=0x00表示无特定编程接口，Sub=0x80表示无线控制器，Base=0x02表示网络控制器
[   31.814463]   Command: 0x0003 [IO MEM ]      #Command寄存器0x0003即BIT(0)=1表示IO端口访问已使能，BIT(1)=1表示MMIO内存访问已使能，BIT(2)=0表示DMA未使能
[   31.814470]   Status: 0x0010                 #Status寄存器0x0010即BIT(4)=1表示设备有Capability链表
[   31.814477]   Subsys: Vendor=0x10EC Device=0xC821
[   31.814485]   IRQ: Line=106 Pin=1            #系统分配的中断号106，中断引脚为1
[   31.814492]   Cache: 32 bytes  Latency: 0    #系统缓存行大小32字节，延迟计时器值为0
[   31.814498]   Capability PTR: 0x40           #Capability链表从配置空间偏移0x40开始
[   31.814506]
[   31.814512]       Capability 链表
[   31.814541]   [0] @0x40: ID=0x01 (Power Management)
[   31.814554]       PM: State=0, D1=No, D2=No, PME=No, PMEStatus=No    #State=0表示当前处于D0（全功率）状态，PME Status=No表示无唤醒事件
[   31.814591]   [1] @0x50: ID=0x05 (MSI)
[   31.814614]       MSI: Enable=No, Addr64=Yes, Data=0x0000            #Enable=No表示MSI中断未使能，Addr64=Yes表示支持64位MSI地址
[   31.814680]   [2] @0x70: ID=0x10 (PCI Express)
[   31.814703]       DEBUG: LNKCAP=0x00075C11, LNKSTA=0x1011
[   31.814712]       PCIe: MaxLanes=x1, MaxSpeed=2.5GT/s    #LNKCAP=0x00075C11其中Bit[3:0]=0x1表示最大速度Gen1(2.5 GT/s)，Bit[9:4]=0x01表示最大宽度x1，Bit[10:11]=0x01表示支持ASPM
[   31.814719]             NegLanes=x1, NegSpeed=2.5GT/s    #LNKSTA=0x1011其中Bit[3:0]=0x1表示当前协商速度Gen1(2.5 GT/s)，Bit[9:4]=0x01表示当前协商宽度x1，Bit[12]=1表示使用Slot Clock
[   31.814835]   共 3 个 Capability
[   31.814841]
[   31.814847]       Extended Capability
[   31.814862]   [0] @0x100: ID=0x0001 (Advanced Error Reporting)   #高级错误报告
[   31.814914]   [1] @0x148: ID=0x0003 (Device Serial Number)       #设备序列号
[   31.815057]   [2] @0x17C: ID=0x000B (Vendor Specific)            #Realtek私有扩展功能
[   31.815087]   [3] @0x160: ID=0x001E (L1 PM Substates)            #L1电源管理子状态
[   31.815221]   [4] @0x170: ID=0x001F (Precision Time Measurement) #精确时间测量
[   31.815227]   共 5 个 Extended Capability
[   31.815233]
[   31.815240] ------------ BAR空间解析 ------------
[   31.815246]       BAR 空间解析
[   31.815259]   BAR0: I/O  0xF4101000 (size=256)                   #BAR0是IO端口空间，地址0xF4101000，大小256B
[   31.815273]   BAR1: MEM  0x0000000000000000 (size=0 KB)
[   31.815286]   BAR2: MEM  0x00000000F4200000 (size=64 KB) 64bit   #BAR2是主要寄存器空间，地址0xF4200000，大小64KB
[   31.815300]   BAR4: MEM  0x0000000000000000 (size=0 KB)          #BAR2是64位BAR，占用BAR2 + BAR3两个槽位，所以跳过了BAR3
[   31.815312]   BAR5: MEM  0x0000000000000000 (size=0 KB)
[   31.815319]
[   31.815325]       BAR2 MMIO 探索
[   31.815342]   基地址: 0xF4200000, 大小: 65536 bytes
[   31.816297]   MMIO 虚拟地址: 000000002309e10a
[   31.816309]   读取前64个字节的寄存器:
[   31.816329]     0000: 51DDBEE1 00030412 00203023 00000000
[   31.816357]     0010: 484E6B43 95A3A155 00400100 00C6A100
[   31.816380]     0020: 0B0102C0 5FCE1DDF 6D0EF9DE 08A30448
[   31.816404]     0030: B161FDFF 18000000 02001000 00000000
[   31.816424]
[   31.816433]
[   31.816505] pcie_explorer: 设备 0000:01:00.0 初始化完成


#进入sysyfs目录，路径根据前面打印的设备名称0000:01:00.0确定
cd /sys/bus/pci/devices/0000:01:00.0/

#显示配置空间信息
cat config_space

#信息输出如下
PCIe 配置空间 (前 256 字节):
0x00: 0xC82110EC
0x04: 0x00100003
0x08: 0x02800000
0x0C: 0x00000008
...

#读取Vendor ID，偏移0x00，2字节
sudo sh -c "echo 0x00 2 > config_read"

#信息输出如下，0x10EC就是RTL8821CE网卡的VID
[  342.970350] CONFIG[0x00]: 0x10EC

#读取Device ID，偏移0x02，2字节
sudo sh -c "echo 0x02 2 > config_read"

#信息输出如下，0xC821就是RTL8821CE网卡的PID
[  637.267519] CONFIG[0x02]: 0xC821

#读取Command寄存器，偏移0x04，2字节
sudo sh -c "echo 0x04 2 > config_read"

#信息输出如下，目前启用IO+MEM
[  667.864607] CONFIG[0x04]: 0x0003

#写入Command寄存器，偏移0x04，2字节，数据0x0007，启动IO+MEM+BusMaster
sudo sh -c "echo 0x04 2 0x0007 > config_write"

#信息输出如下
[  796.366976] CONFIG_WRITE[0x04]: 0x00000007 (size=2)

#写入非白名单配置寄存器会被拒绝
sudo sh -c "echo 0x10 4 0x00 > config_write"

#信息输出如下
[  953.018945] 寄存器 0x10 (size=4) 不在白名单中
[  953.019012] 允许写入的配置空间寄存器（绝对偏移）:
[  953.019032]   0x04, 0x05 - PCI_COMMAND 命令寄存器
[  953.019040]   0x0C, 0x0D - PCI_LATENCY_TIMER/PCI_CACHE_LINE_SIZE
[  953.019047]   0x3C, 0x3D - PCI_INTERRUPT_LINE/PIN


#显示BAR空间信息
cat bar_info

#信息输出如下
BAR 空间信息:
BAR0: I/O 0x00001000
BAR2: MEM 0x00000000F4200000 (size=64 KB)
BAR2 已映射: 00000000dce8a77b (size=65536)

#读取BAR2起始处的寄存器
sudo sh -c "echo 0x00 4 > bar_read"

#信息输出如下
[ 1203.421626] BAR[0x0000]: 0x51DDBEE1

#读取RTK_PCI_HISR0中断状态0寄存器
sudo sh -c "echo 0xB4 4 > bar_read"

#信息输出如下
[ 1382.148634] BAR[0x00B4]: 0xC0000000

#写入RTK_PCI_HISR0中断状态0寄存器，向值为1的位写入1即可清除对应中断标志，写入读取值即可全部清除，因此写入读取到的0xC0000000
sudo sh -c "echo 0xB4 4 0xC0000000 > bar_write"

#信息输出如下
[ 1635.335286] BAR_WRITE[0x00B4]: 0xC0000000 (size=4)

#再次读取RTK_PCI_HISR0中断状态0寄存器
sudo sh -c "echo 0xB4 4 > bar_read"

#信息输出如下，全部位都为0，清除全部中断标志成功
[ 1661.326507] BAR[0x00B4]: 0x00000000

#写入非白名单BAR寄存器会被拒绝
sudo sh -c "echo 0xB8 4 0x0000001 > bar_write"

#信息输出如下
[ 1771.404406] BAR寄存器 0x00B8 不在白名单中，禁止写入
[ 1771.404477] 允许写入的BAR寄存器:
[ 1771.404486]   0x00B4 - RTK_PCI_HISR0 中断状态0
[ 1771.404494]   0x00BC - RTK_PCI_HISR1 中断状态1
[ 1771.404502]   0x0300 - RTK_PCI_CTRL DMA控制
```

2. pcie_dma.ko

```bash

#查看网卡的VID和PID
lspci -nn

#信息输出如下
00:00.0 PCI bridge [0604]: Rockchip Electronics Co., Ltd RK3568 Remote Signal Processor [1d87:3566] (rev 01)
01:00.0 Network controller [0280]: Realtek Semiconductor Co., Ltd. RTL8821CE 802.11ac PCIe Wireless Network Adapter [10ec:c821]

#查看当前系统加载的驱动模块
lsmod

#4.19.232内核系统信息输出如下
Module                  Size  Used by
rfcomm                 61440  12
bnep                   24576  2
rtk_btusb              57344  0
btusb                  40960  0
btrtl                  16384  1 btusb
btbcm                  16384  1 btusb
btintel                20480  1 btusb
8821ce               1859584  0
bluetooth             450560  39 btrtl,btintel,btbcm,bnep,btusb,rfcomm

#6.1.99内核系统信息输出如下
Module                  Size  Used by
btusb                  53248  0
btrtl                  28672  1 btusb
btbcm                  24576  1 btusb
rtw88_8821ce           16384  0
rtw88_8821c            90112  1 rtw88_8821ce
btintel                40960  1 btusb
bluetooth             716800  27 btrtl,btintel,btbcm,btusb
ecdh_generic           16384  2 bluetooth
ecc                    32768  1 ecdh_generic
rtw88_pci              28672  1 rtw88_8821ce
rtw88_core            208896  2 rtw88_pci,rtw88_8821c
ip_tables              32768  0
rtc_rk808              20480  0
rtc_hym8563            16384  1

#4.19.232内核系统卸载8821ce模块
sudo rmmod 8821ce

#6.1.99内核系统卸载rtw88_8821ce模块
sudo rmmod rtw88_8821ce

#加载实验驱动
sudo insmod pcie_dma.ko

#信息输出如下
[   34.761875] pcie_dma: 发现设备 0000:01:00.0
[   34.763218]   BAR2 映射: 物理=0xf4200000 虚拟=00000000c9cc0a15 大小=65536
[   34.763252] ----------DMA初始化----------
[   34.763261]   [RX环] 描述符数量: 512
[   34.763268]   [RX环] 描述符大小: 8 bytes
[   34.763275]   [RX环] 数据缓冲区: 11478 bytes/个
[   34.765599]   [RX环] 虚拟地址: 00000000941f7280
[   34.765638]   [RX环] DMA地址:  0x000000007ee04000
[   34.765651]   [RX环] 总大小:   4096 bytes
[   34.765689]   [RX环] 前4个描述符的详细信息:
[   34.765702]   [RX环] 描述符[0]: dma=0x573C0040 buf_size=11478
[   34.765718]   [RX环] 前4个描述符的详细信息:
[   34.765726]   [RX环] 描述符[1]: dma=0x54CAC040 buf_size=11478
[   34.765741]   [RX环] 前4个描述符的详细信息:
[   34.765749]   [RX环] 描述符[2]: dma=0x57278040 buf_size=11478
[   34.765767]   [RX环] 前4个描述符的详细信息:
[   34.765779]   [RX环] 描述符[3]: dma=0x57340040 buf_size=11478
[   34.770369]   [RX环] 共512个描述符已初始化
[   34.770410]   [HW配置] 写入DMA寄存器:
[   34.770424]     REG[0x0338] RX环基地址 = 0x7EE04000
[   34.770435]     REG[0x0382] RX环大小   = 512
[   34.770445]     REG[0x039C] 读写指针=0xFFFFFFFF (清零)
[   34.770458]     REG[0x0300] DMA控制    = 0xF713FEFF (复位+RX Tag)
[   34.770468]   [HW配置] 完成
[   34.770532] pcie_dma: 设备 0000:01:00.0 初始化完成


#进入sysyfs目录，路径根据前面打印的设备名称0000:01:00.0确定
cd /sys/bus/pci/devices/0000:01:00.0/

#显示DMA总体信息
cat dma_info

#信息输出如下
------PCI DMA信息------
DMA是否始化: Yes

RX 描述符环:
虚拟地址:   00000000941f7280
DMA地址:    0x000000007ee04000
环大小:     4096 bytes
描述符数:   512
描述符大小: 8 bytes
WP: 0  RP: 0

硬件寄存器:
RX_IDX [0x03B4] = 0x00000000
    HW WP: 0
    Host RP: 0
RX_DESA [0x0338] = 0x7EE04000
RX_NUM  [0x0382] = 0x0200

#显示RX描述符环内容
cat dma_ring

#信息输出如下
RX 描述符环 (显示前 16 / 512 个):
IDX    buf_size     total_pkt    dma_addr
----------------------------------------------
0      11478        0            0x573C0040
1      11478        0            0x54CAC040
2      11478        0            0x57278040
3      11478        0            0x57340040
4      11478        0            0x5731C040
5      11478        0            0x54CB8040
6      11478        0            0x54C98040
7      11478        0            0x7514C040
8      11478        0            0x7410C040
9      11478        0            0x54C28040
10     11478        0            0x54D14040
11     11478        0            0x737F8040
12     11478        0            0x77610040
13     11478        0            0x77608040
14     11478        0            0x733E8040
15     11478        0            0x731B8040
```

3. pcie_explorer.ko

```bash

#查看网卡的VID和PID
lspci -nn

#信息输出如下
00:00.0 PCI bridge [0604]: Rockchip Electronics Co., Ltd RK3568 Remote Signal Processor [1d87:3566] (rev 01)
01:00.0 Network controller [0280]: Realtek Semiconductor Co., Ltd. RTL8821CE 802.11ac PCIe Wireless Network Adapter [10ec:c821]

#查看当前系统加载的驱动模块
lsmod

#4.19.232内核系统信息输出如下
Module                  Size  Used by
rfcomm                 61440  12
bnep                   24576  2
rtk_btusb              57344  0
btusb                  40960  0
btrtl                  16384  1 btusb
btbcm                  16384  1 btusb
btintel                20480  1 btusb
8821ce               1859584  0
bluetooth             450560  39 btrtl,btintel,btbcm,bnep,btusb,rfcomm

#6.1.99内核系统信息输出如下
Module                  Size  Used by
btusb                  53248  0
btrtl                  28672  1 btusb
btbcm                  24576  1 btusb
rtw88_8821ce           16384  0
rtw88_8821c            90112  1 rtw88_8821ce
btintel                40960  1 btusb
bluetooth             716800  27 btrtl,btintel,btbcm,btusb
ecdh_generic           16384  2 bluetooth
ecc                    32768  1 ecdh_generic
rtw88_pci              28672  1 rtw88_8821ce
rtw88_core            208896  2 rtw88_pci,rtw88_8821c
ip_tables              32768  0
rtc_rk808              20480  0
rtc_hym8563            16384  1

#4.19.232内核系统卸载8821ce模块
sudo rmmod 8821ce

#6.1.99内核系统卸载rtw88_8821ce模块
sudo rmmod rtw88_8821ce

#加载实验驱动
sudo insmod pcie_irq.ko

#信息输出如下
[   31.420973] pcie_irq: 发现设备 0000:01:00.0
[   31.422397]   BAR2 映射: 物理=0xf4200000 虚拟=00000000ca1f508a 大小=65536
[   31.422428] --------- 中断子系统初始化 ---------
[   31.422783] 中断向量申请成功: IRQ=117
[   31.423216] 线程化中断注册成功
[   31.423231] 默认中断掩码: HIMR0=0x000044FD HIMR1=0x00000200 HIMR3=0x00010000
[   31.423242] 中断已使能，等待设备触发...
[   31.423273] pcie_irq: 设备 0000:01:00.0 初始化完成

#进入sysyfs目录，路径根据前面打印的设备名称0000:01:00.0确定
cd /sys/bus/pci/devices/0000:01:00.0/

#显示中断状态信息
cat irq_info

#信息输出如下
--------- PCI中断信息 ---------
中断向量:   117
已注册:     Yes
已使能:     Yes
中断计数:   0

最后中断状态:
HISR0 = 0x00000000
HISR1 = 0x00000000
HISR3 = 0x00000000

当前寄存器值:
HISR0 [0x00B4] = 0xC0000000
HIMR0 [0x00B0] = 0x000044FD
HISR1 [0x00BC] = 0x20000001
HIMR1 [0x00B8] = 0x00000200

#手动触发中断
sudo sh -c "echo 1 > irq_trigger"

#信息输出如下
[   86.311273] pcie_irq: --------- 手动触发中断 ---------
[   86.311386]   当前 HISR0=0xC0000000 HISR1=0x20000001 HISR3=0x00000001
[   86.311417] 已写入 HIMR0=0xC0000000 HIMR1=0x20000001 HIMR3=0x00000001
[   86.311535] 等待硬件触发MSI中断...
[   86.311624] pcie_irq: IRQ #1  HISR0=0xC0000000 HISR1=0x20000001 HISR3=0x00000001
[   86.311679]   -> HISR0 BIT(30) IMR_TIMER1        定时器1
[   86.311737]   -> HISR0 BIT(31) IMR_TIMER2        定时器2
[   86.311794]   -> HISR1 BIT(0)  保留位
[   86.311820]   -> HISR1 BIT(29) IMR_BTON_STS_UPDATE BT状态更新

#再次查看中断状态信息
cat irq_info

#信息输出如下
--------- PCI中断信息 ---------
中断向量:   117
已注册:     Yes
已使能:     Yes
中断计数:   1

最后中断状态:
HISR0 = 0xC0000000
HISR1 = 0x20000001
HISR3 = 0x00000001

当前寄存器值:
HISR0 [0x00B4] = 0x00000000
HIMR0 [0x00B0] = 0x000044FD
HISR1 [0x00BC] = 0x00000000
HIMR1 [0x00B8] = 0x00000200

#禁用中断
sudo sh -c "echo 0 > irq_enable"

#信息输出如下
[  739.529063] pcie_irq: 中断已禁用

#查看中断状态信息
cat irq_info

#信息输出如下
--------- PCI中断信息 ---------
中断向量:   117
已注册:     Yes
已使能:     No
中断计数:   1

最后中断状态:
HISR0 = 0xC0000000
HISR1 = 0x20000001
HISR3 = 0x00000001

当前寄存器值:
HISR0 [0x00B4] = 0x00000000
HIMR0 [0x00B0] = 0x00000000
HISR1 [0x00BC] = 0x00000000
HIMR1 [0x00B8] = 0x00000000

#使能中断
sudo sh -c "echo 1 > irq_enable"

#信息输出如下
[  785.265590] pcie_irq: 全部中断已使能 (HIMR0=0x000044FD)

#查看中断状态信息
cat irq_info

#信息输出如下
--------- PCI中断信息 ---------
中断向量:   117
已注册:     Yes
已使能:     Yes
中断计数:   1

最后中断状态:
HISR0 = 0xC0000000
HISR1 = 0x20000001
HISR3 = 0x00000001

当前寄存器值:
HISR0 [0x00B4] = 0x00000000
HIMR0 [0x00B0] = 0x000044FD
HISR1 [0x00BC] = 0x00000000
HIMR1 [0x00B8] = 0x00000200
```
