# scsi_subsystem

运行`make`命令后，将会有一个模块：

* virtual_scsi_host.ko

加载驱动程序和内核调试信息：

1. virtual_scsi_host.ko

```bash
# 加载驱动
sudo insmod virtual_scsi_host.ko

### 信息输出如下
[   24.008222] virtual_scsi_host: Virtual SCSI Host Driver v1.0 initializing...
[   24.035672] virtual_scsi_host: Allocated virtual disk: 16 MB (32768 sectors, 512 bytes/sector)
[   24.036243] scsi host1: virtual_scsi_host
[   24.036729] virtual_scsi_host: Virtual SCSI Host initialized successfully!
[   24.036751] virtual_scsi_host: Statistics: sectors_read=0, sectors_written=0
[   24.037144] scsi 1:0:0:0: Direct-Access     LubanCat VIRTUAL DISK     1.0  PQ: 0 ANSI: 6
[   24.039837] sd 1:0:0:0: [sda] 32768 512-byte logical blocks: (16.8 MB/16.0 MiB)
[   24.040010] sd 1:0:0:0: [sda] Write Protect is off
[   24.040091] sd 1:0:0:0: [sda] No Caching mode page found
[   24.040111] sd 1:0:0:0: [sda] Assuming drive cache: write through

# 查看系统SCSI总线设备需要安装lsscsi工具
sudo apt update && sudo apt install lsscsi

# 列出所有SCSI主机与LUN设备
lsscsi

# 信息输出如下
[1:0:0:0]    disk    LubanCat VIRTUAL DISK     1.0   /dev/sda

# 列出所有块设备节点
lsblk

# 信息输出如下
NAME         MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
sda            8:0    0   16M  0 disk
mmcblk0      179:0    0 29.3G  0 disk
├─mmcblk0p1  179:1    0    8M  0 part
├─mmcblk0p2  179:2    0  128M  0 part /boot
└─mmcblk0p3  179:3    0 29.2G  0 part /
mmcblk0boot0 179:32   0    4M  1 disk
mmcblk0boot1 179:64   0    4M  1 disk
zram0        254:0    0    0B  0 disk

# 安装sg系列工具
sudo apt update && sudo apt install sg3-utils

# INQUIRY 设备信息查询验证
sudo sg_inq /dev/sda

# 信息输出如下
standard INQUIRY:
PQual=0  PDT=0  RMB=0  LU_CONG=0  hot_pluggable=0  version=0x06  [SPC-4]
[AERC=0]  [TrmTsk=0]  NormACA=0  HiSUP=0  Resp_data_format=2
SCCS=0  ACC=0  TPGS=0  3PC=0  Protect=0  [BQue=0]
EncServ=0  MultiP=0  [MChngr=0]  [ACKREQQ=0]  Addr16=0
[RelAdr=0]  WBus16=0  Sync=0  [Linked=0]  [TranDis=0]  CmdQue=0
    length=36 (0x24)   Peripheral device type: disk
Vendor identification: LubanCat
Product identification: VIRTUAL DISK
Product revision level: 1.0

# 容量查询
sudo sg_readcap /dev/sda

# 信息输出如下
Read Capacity results:
Last LBA=32767 (0x7fff), Number of logical blocks=32768
Logical block length=512 bytes
Hence:
Device size: 16777216 bytes, 16.0 MiB, 0.02 GB

# 就绪检测
sudo sg_turs -v /dev/sda

# 信息输出如下
test unit ready cdb: [00 00 00 00 00 00]

# 读取缓存模式页，-p 8指定读取缓存模式页，对应0x08模式页
sudo sg_modes -p 8 /dev/sda

# 信息输出如下
LubanCat  VIRTUAL DISK      1.0    peripheral_type: disk [0x0]
Mode parameter header from MODE SENSE(10):
Mode data length=35, medium type=0x00, WP=0, DpoFua=1, longlba=0
Block descriptor length=8

> Direct access device block descriptors:
Density code=0x0
00     00 00 80 00 00 00 02 00

>> Caching, page_control: current
00     08 12 14 00 ff ff 00 00  ff ff ff ff 80 14 00 00
10     00 00 00

# 测试MODE_SELECT，参考命令如下
sudo sg_wr_mode --force -p 8 -c 08,12,14 /dev/sda

# 校验LBA 0，共1000个扇区，正常无信息输出
sudo sg_verify -l 0 -c 1000 /dev/sda

# 验证LBA越界报错，传入超出磁盘最大扇区
sudo sg_verify -l 40000 -c 10 /dev/sda

# 信息输出如下，LBA超出范围
VERIFY(10): LBA out of range
    failed near lba=40000 [0x9c40]
sg_verify failed: LBA out of range

# 验证缓存同步，正常无信息输出
sudo sg_sync /dev/sda

# 写入随机测试数据
sudo sg_dd if=/dev/urandom of=/dev/sda bs=512 count=10

# 读取数据到文件
sudo sg_dd if=/dev/sda of=test_data.bin bs=512 count=10

# 格式化为ext4文件系统
sudo mkfs.ext4 /dev/sda

# 信息输出如下
mke2fs 1.47.0 (5-Feb-2023)
Creating filesystem with 16384 1k blocks and 4096 inodes
Filesystem UUID: 5052d0bc-e26b-49f7-84b5-8ab8cc330d88
Superblock backups stored on blocks:
        8193

Allocating group tables: done
Writing inode tables: done
Creating journal (1024 blocks): done
Writing superblocks and filesystem accounting information: done

# 挂载虚拟磁盘
sudo mount /dev/sda /mnt/

# 信息输出如下
[   40.064055] EXT4-fs (sda): mounting with "discard" option, but the device does not support discard
[   40.064108] EXT4-fs (sda): mounted filesystem with ordered data mode. Quota mode: disabled.

# 查看系统挂载情况
df -h

# 信息输出如下
Filesystem      Size  Used Avail Use% Mounted on
udev            962M  8.0K  962M   1% /dev
tmpfs           196M  1.2M  195M   1% /run
/dev/mmcblk0p3   29G  2.1G   26G   8% /
tmpfs           977M     0  977M   0% /dev/shm
tmpfs           5.0M   12K  5.0M   1% /run/lock
tmpfs           977M  4.0K  977M   1% /tmp
/dev/mmcblk0p2  124M   55M   64M  47% /boot
tmpfs           196M     0  196M   0% /run/user/0
/dev/sda         14M   14K   13M   1% /mnt

#进入挂载目录
cd /mnt/

#写入测试文件
sudo sh -c "echo virtual_scsi_host > test.txt"

#读取测试文件
cat test.txt

#信息打印如下
virtual_scsi_host

#返回家目录
cd ~

#卸载挂载点
sudo umount /mnt/

#卸载内核模块
sudo rmmod virtual_scsi_host

#信息打印如下
[  164.367897] virtual_scsi_host: Unloading Virtual SCSI Host Driver...
[  164.368012] virtual_scsi_host: Final statistics:
[  164.368026] virtual_scsi_host:   Sectors read:    3804 
[  164.368039] virtual_scsi_host:   Sectors written: 4616
[  164.482600] virtual_scsi_host: Virtual SCSI Host Driver unloaded successfully!
```