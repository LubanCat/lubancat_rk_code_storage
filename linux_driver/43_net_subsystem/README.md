# net_subsystem

运行`make`命令后，将会有一个模块：

* net_loopback.ko

加载驱动程序和内核调试信息：

1. net_loopback.ko

```bash

# 加载驱动
sudo insmod net_loopback.ko

# 信息输出如下
[  158.054235] 虚拟回环网卡加载
[  158.283334] lbnet0: 虚拟回环网卡已打开

# 查看lbnet0网卡
ifconfig lbnet0

# 信息输出如下
lbnet0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
    ether ae:4c:58:a4:f1:82  txqueuelen 1000  (Ethernet)
    RX packets 0  bytes 0 (0.0 B)
    RX errors 0  dropped 0  overruns 0  frame 0
    TX packets 0  bytes 0 (0.0 B)
    TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

# 禁用NetworkManager对虚拟网卡控制
sudo nmcli device set lbnet0 managed no

# 手动配置lbnet0虚拟网卡ip和掩码
sudo ip addr add 10.0.0.1/24 dev lbnet0

# 查看lbnet0网卡
ifconfig lbnet0

# 信息输出如下
lbnet0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
    inet 10.0.0.1  netmask 255.255.255.0  broadcast 0.0.0.0
    ether ae:4c:58:a4:f1:82  txqueuelen 1000  (Ethernet)
    RX packets 0  bytes 0 (0.0 B)
    RX errors 0  dropped 0  overruns 0  frame 0
    TX packets 0  bytes 0 (0.0 B)
    TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

# 查看网关信息
route -n

# 信息输出如下
Kernel IP routing table
Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
10.0.0.0        0.0.0.0         255.255.255.0   U     0      0        0 lbnet0

# ping命令测试，ping 10次
ping 10.0.0.2 -c 10

# 信息输出如下
PING 10.0.0.2 (10.0.0.2) 56(84) bytes of data.
64 bytes from 10.0.0.2: icmp_seq=1 ttl=64 time=0.184 ms
64 bytes from 10.0.0.2: icmp_seq=2 ttl=64 time=0.203 ms
64 bytes from 10.0.0.2: icmp_seq=3 ttl=64 time=0.149 ms
64 bytes from 10.0.0.2: icmp_seq=4 ttl=64 time=0.202 ms
64 bytes from 10.0.0.2: icmp_seq=5 ttl=64 time=0.205 ms
64 bytes from 10.0.0.2: icmp_seq=6 ttl=64 time=0.210 ms
64 bytes from 10.0.0.2: icmp_seq=7 ttl=64 time=0.212 ms
64 bytes from 10.0.0.2: icmp_seq=8 ttl=64 time=0.144 ms
64 bytes from 10.0.0.2: icmp_seq=9 ttl=64 time=0.214 ms
64 bytes from 10.0.0.2: icmp_seq=10 ttl=64 time=0.201 ms

--- 10.0.0.2 ping statistics ---
10 packets transmitted, 10 received, 0% packet loss, time 9016ms
rtt min/avg/max/mdev = 0.144/0.192/0.214/0.024 ms

# 查看lbnet0网卡
ifconfig lbnet0

# 信息输出如下
lbnet0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 10.0.0.1  netmask 255.255.255.0  broadcast 0.0.0.0
        ether ae:4c:58:a4:f1:82  txqueuelen 1000  (Ethernet)
        RX packets 11  bytes 868 (868.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 11  bytes 1022 (1022.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

# 开启debug信息打印
sudo sh -c "echo 1 > /sys/class/net/lbnet0/loopback/debug"

# ping命令测试，ping 1次
ping 10.0.0.2 -c 1

# 信息输出如下
[ 2118.226279] lbnet0: 报文详细打印: 开启
[ 2125.248503] lbnet0:
[ 2125.248503] ---[TX]-- lbnet0 -- 98 bytes --
[ 2125.248503] | ETH   dst=00:00:00:00:00:00  src=ae:4c:58:a4:f1:82  IPv4
[ 2125.248503] | IP    10.0.0.1 --> 10.0.0.2  ICMP  ttl=64  len=84
[ 2125.248503] | ICMP  Echo-Request  id=0x332c  seq=1
[ 2125.248503] --- fields ---
[ 2125.248588]   0000  dst-mac      00 00 00 00 00 00        00:00:00:00:00:00
[ 2125.248603]   0006  src-mac      ae 4c 58 a4 f1 82        ae:4c:58:a4:f1:82
[ 2125.248614]   000c  ethertype    08 00                    0x0800 (IPv4)
[ 2125.248626]   000e  ver/ihl      45                       v4, ihl=5 (20 bytes)
[ 2125.248637]   000f  tos          00                       0x00
[ 2125.248647]   0010  tot-len      00 54                    84
[ 2125.248657]   0012  id           3e f0                    0x3ef0
[ 2125.248666]   0014  flags/frag   40 00                    flags=0x2 frag=0
[ 2125.248676]   0016  ttl          40                       64
[ 2125.248686]   0017  protocol     01                       0x01 (ICMP)
[ 2125.248695]   0018  checksum     e7 b6                    0xe7b6
[ 2125.248706]   001a  src-ip       0a 00 00 01              10.0.0.1
[ 2125.248716]   001e  dst-ip       0a 00 00 02              10.0.0.2
[ 2125.248726]   0022  type         08                       0x08 (Echo-Request)
[ 2125.248736]   0023  code         00                       0x00
[ 2125.248746]   0024  checksum     52 e6                    0x52e6
[ 2125.248755]   0026  id           33 2c                    0x332c
[ 2125.248765]   0028  seq          00 01                    1
[ 2125.248778]   002a  payload      7f 5d 8d 6a 00 00 00 00 a4 51 02 00 00 00 00 00 (payload 0-16/56 bytes)
[ 2125.248793]   003a  cont         10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f (payload 16-32/56 bytes)
[ 2125.248808]   004a  cont         20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f (payload 32-48/56 bytes)
[ 2125.248820]   005a  cont         30 31 32 33 34 35 36 37  (payload 48-56/56 bytes)
[ 2125.248854] lbnet0:
[ 2125.248854] ---[RX]-- lbnet0 -- 98 bytes --
[ 2125.248854] | ETH   dst=ae:4c:58:a4:f1:82  src=00:00:00:00:00:00  IPv4
[ 2125.248854] | IP    10.0.0.2 --> 10.0.0.1  ICMP  ttl=64  len=84
[ 2125.248854] | ICMP  Echo-Reply  id=0x332c  seq=1
[ 2125.248854] --- fields ---
[ 2125.248869]   0000  dst-mac      ae 4c 58 a4 f1 82        ae:4c:58:a4:f1:82
[ 2125.248882]   0006  src-mac      00 00 00 00 00 00        00:00:00:00:00:00
[ 2125.248893]   000c  ethertype    08 00                    0x0800 (IPv4)
[ 2125.248903]   000e  ver/ihl      45                       v4, ihl=5 (20 bytes)
[ 2125.248913]   000f  tos          00                       0x00
[ 2125.248922]   0010  tot-len      00 54                    84
[ 2125.248931]   0012  id           3e f0                    0x3ef0
[ 2125.248941]   0014  flags/frag   40 00                    flags=0x2 frag=0
[ 2125.248951]   0016  ttl          40                       64
[ 2125.248960]   0017  protocol     01                       0x01 (ICMP)
[ 2125.248970]   0018  checksum     e7 b6                    0xe7b6
[ 2125.248980]   001a  src-ip       0a 00 00 02              10.0.0.2
[ 2125.248990]   001e  dst-ip       0a 00 00 01              10.0.0.1
[ 2125.249000]   0022  type         00                       0x00 (Echo-Reply)
[ 2125.249009]   0023  code         00                       0x00
[ 2125.249019]   0024  checksum     5a e6                    0x5ae6
[ 2125.249028]   0026  id           33 2c                    0x332c
[ 2125.249037]   0028  seq          00 01                    1
[ 2125.249050]   002a  payload      7f 5d 8d 6a 00 00 00 00 a4 51 02 00 00 00 00 00 (payload 0-16/56 bytes)
[ 2125.249065]   003a  cont         10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f (payload 16-32/56 bytes)
[ 2125.249079]   004a  cont         20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f (payload 32-48/56 bytes)
[ 2125.249092]   005a  cont         30 31 32 33 34 35 36 37  (payload 48-56/56 bytes)
[ 2130.283465] lbnet0:
[ 2130.283465] ---[TX]-- lbnet0 -- 42 bytes --
[ 2130.283465] | ETH   dst=00:00:00:00:00:00  src=ae:4c:58:a4:f1:82  ARP
[ 2130.283465] | ARP   Request  10.0.0.1(ae:4c:58:a4:f1:82) --> 10.0.0.2(00:00:00:00:00:00)
[ 2130.283465] --- fields ---
[ 2130.283673]   0000  dst-mac      00 00 00 00 00 00        00:00:00:00:00:00
[ 2130.283749]   0006  src-mac      ae 4c 58 a4 f1 82        ae:4c:58:a4:f1:82
[ 2130.283811]   000c  ethertype    08 06                    0x0806 (ARP)
[ 2130.283873]   000e  hrd-type     00 01                    0x0001 (Ethernet)
[ 2130.283933]   0010  pro-type     08 00                    0x0800 (IPv4)
[ 2130.283988]   0012  hrd-len      06                       6
[ 2130.284041]   0013  pro-len      04                       4
[ 2130.284099]   0014  opcode       00 01                    0x0001 (Request)
[ 2130.284160]   0016  sender-mac   ae 4c 58 a4 f1 82        ae:4c:58:a4:f1:82
[ 2130.284221]   001c  sender-ip    0a 00 00 01              10.0.0.1
[ 2130.284281]   0020  target-mac   00 00 00 00 00 00        00:00:00:00:00:00
[ 2130.284342]   0026  target-ip    0a 00 00 02              10.0.0.2
[ 2130.286467] lbnet0:
[ 2130.286467] ---[RX]-- lbnet0 -- 42 bytes --
[ 2130.286467] | ETH   dst=ae:4c:58:a4:f1:82  src=00:00:00:00:00:00  ARP
[ 2130.286467] | ARP   Reply  10.0.0.2(00:00:00:00:00:00) --> 10.0.0.1(ae:4c:58:a4:f1:82)
[ 2130.286467] --- fields ---
[ 2130.286649]   0000  dst-mac      ae 4c 58 a4 f1 82        ae:4c:58:a4:f1:82
[ 2130.286707]   0006  src-mac      00 00 00 00 00 00        00:00:00:00:00:00
[ 2130.286755]   000c  ethertype    08 06                    0x0806 (ARP)
[ 2130.286800]   000e  hrd-type     00 01                    0x0001 (Ethernet)
[ 2130.286845]   0010  pro-type     08 00                    0x0800 (IPv4)
[ 2130.286886]   0012  hrd-len      06                       6
[ 2130.286927]   0013  pro-len      04                       4
[ 2130.286969]   0014  opcode       00 02                    0x0002 (Reply)
[ 2130.287017]   0016  sender-mac   00 00 00 00 00 00        00:00:00:00:00:00
[ 2130.287062]   001c  sender-ip    0a 00 00 02              10.0.0.2
[ 2130.287110]   0020  target-mac   ae 4c 58 a4 f1 82        ae:4c:58:a4:f1:82
[ 2130.287155]   0026  target-ip    0a 00 00 01              10.0.0.1

# 查看驱动信息
ethtool -i lbnet0

# 信息输出如下
driver: net_loopback
version: 1.0
firmware-version:
expansion-rom-version:
bus-info: virtual
supports-statistics: yes
supports-test: no
supports-eeprom-access: no
supports-register-dump: no
supports-priv-flags: no

# 查看链路状态
ethtool lbnet0

# 信息输出如下
Settings for lbnet0:
        Link detected: yes

# 查看统计信息
ethtool -S lbnet0

# 信息输出如下
NIC statistics:
    tx_packets: 13
    tx_bytes: 1162
    rx_packets: 13
    rx_bytes: 980
    tx_dropped: 0
    lb_queue_len: 0
```
