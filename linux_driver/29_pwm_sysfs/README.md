# pwm_sysfs drivers

运行`make`命令后，将会有一个模块：

* pwm_sysfs.ko

加载驱动程序和内核调试信息：

1. pwm_sysfs.ko

```bash
# insmod pwm_sysfs.ko

   [  284.955874] pwm driver probe


# cd /sys/class/pwm-sysfs/pwm-test/

# ls -l

   total 0
   lrwxrwxrwx 1 root root    0 Apr 18 09:29 device -> ../../../pwm-test
   -rw-r--r-- 1 root root 4096 Apr 18 09:29 duty_cycle
   -rw-r--r-- 1 root root 4096 Apr 18 09:29 enable
   -rw-r--r-- 1 root root 4096 Apr 18 09:29 period
   -rw-r--r-- 1 root root 4096 Apr 18 09:29 polarity
   drwxr-xr-x 2 root root    0 Apr 18 09:29 power
   lrwxrwxrwx 1 root root    0 Apr 18 09:29 subsystem -> ../../../../../class/pwm-sysfs
   -rw-r--r-- 1 root root 4096 Apr 18 09:28 uevent

# cat enable

# cat period

# cat duty_cycle

# cat polarity

# cat /sys/kernel/debug/pwm

   platform/fe6f0010.pwm, 1 PWM device
   pwm-0   (pwm-test            ): requested period: 10000 ns duty: 5000 ns polarity: normal

   platform/fe6e0010.pwm, 1 PWM device
   pwm-0   (backlight1          ): requested period: 50000 ns duty: 0 ns polarity: normal

   platform/fe6e0000.pwm, 1 PWM device
   pwm-0   (backlight           ): requested period: 50000 ns duty: 0 ns polarity: normal

   platform/fdd70000.pwm, 1 PWM device
   pwm-0   ((null)              ): period: 0 ns duty: 0 ns polarity: inverse


# sudo sh -c "echo 20000 > period"

# sudo sh -c "echo 10000 > duty_cycle"

# sudo sh -c "echo 1 > polarity"

# sudo sh -c "echo 1 > enable"

# sudo cat /sys/kernel/debug/pwm

   platform/fe6f0010.pwm, 1 PWM device
   pwm-0   (pwm-test            ): requested enabled period: 20000 ns duty: 10000 ns polarity: inverse

   platform/fe6e0010.pwm, 1 PWM device
   pwm-0   (backlight1          ): requested period: 50000 ns duty: 0 ns polarity: normal

   platform/fe6e0000.pwm, 1 PWM device
   pwm-0   (backlight           ): requested period: 50000 ns duty: 0 ns polarity: normal

   platform/fdd70000.pwm, 1 PWM device
   pwm-0   ((null)              ): period: 0 ns duty: 0 ns polarity: inverse
```