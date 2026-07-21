# tty_subsystem

运行`make`命令后，将会有一个模块：

* tty_subsystem.ko

加载驱动程序和内核调试信息：

1. tty_subsystem.ko

```bash
#加载驱动
sudo insmod tty_subsystem.ko

#信息输出如下
[   26.546032] ttytest: virtual 16550 UART driver loaded
[   26.546068] ttytest: device node: /dev/TEST0

#查看设备节点
ls -l /dev/ttyTEST*

#信息输出如下
crw-rw---- 1 root dialout 236, 0 Jul 14 17:18 /dev/ttyTEST0

#查看通信参数
stty -F /dev/ttyTEST0

#信息输出如下
speed 9600 baud; line = 0;
-brkint -imaxbel



#读取虚拟串口数据
cat /dev/ttyTEST0

#信息输出如下，每2秒打印一次
Virtual UART: Hello from 16550!
Virtual UART: Hello from 16550!
Virtual UART: Hello from 16550!

#向串口写入一行数据
sudo sh -c "echo Hello TTY Subsystem! > /dev/ttyTEST0"

#再次读取观察数据变化
cat /dev/ttyTEST0

#信息输出如下，每2秒打印一次
Hello TTY Subsystem!
Hello TTY Subsystem!
Hello TTY Subsystem!



#查看当前所有参数
stty -F /dev/ttyTEST0 -a

#信息输出如下
speed 9600 baud; rows 0; columns 0; line = 0;
intr = ^C; quit = ^\; erase = ^?; kill = ^U; eof = ^D; eol = <undef>;
eol2 = <undef>; swtch = <undef>; start = ^Q; stop = ^S; susp = ^Z; rprnt = ^R;
werase = ^W; lnext = ^V; discard = ^O; min = 1; time = 0;
-parenb -parodd -cmspar cs8 hupcl -cstopb cread clocal -crtscts
-ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr icrnl ixon -ixoff
-iuclc -ixany -imaxbel -iutf8
opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0
isig icanon iexten echo echoe echok -echonl -noflsh -xcase -tostop -echoprt
echoctl echoke -flusho -extproc

#修改波特率115200
stty -F /dev/ttyTEST0 115200

#修改数据位，cs5~cs8
stty -F /dev/ttyTEST0 cs7

#修改停止位
#设置2位停止位
stty -F /dev/ttyTEST0 cstopb

###恢复为1位停止位
###stty -F /dev/ttyTEST0 -cstopb

#修改奇偶校验
#奇校验
stty -F /dev/ttyTEST0 parenb parodd
#偶校验
stty -F /dev/ttyTEST0 parenb -parodd
#无校验
stty -F /dev/ttyTEST0 -parenb

#组合设置一条命令，115200 7E2（7位数据，偶校验，2停止位）
stty -F /dev/ttyTEST0 115200 cs7 cstopb parenb -parodd

#再查看当前所有参数
stty -F /dev/ttyTEST0 -a

#信息输出如下
speed 115200 baud; rows 0; columns 0; line = 0;
intr = ^C; quit = ^\; erase = ^?; kill = ^U; eof = ^D; eol = <undef>;
eol2 = <undef>; swtch = <undef>; start = ^Q; stop = ^S; susp = ^Z; rprnt = ^R;
werase = ^W; lnext = ^V; discard = ^O; min = 1; time = 0;
parenb -parodd -cmspar cs7 hupcl cstopb cread clocal -crtscts
-ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr icrnl ixon -ixoff
-iuclc -ixany -imaxbel -iutf8
opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0
isig icanon iexten echo echoe echok -echonl -noflsh -xcase -tostop -echoprt
echoctl echoke -flusho -extproc
```