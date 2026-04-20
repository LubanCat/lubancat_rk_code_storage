# pwm_subsystem drivers

运行`make`命令后，将会有一个模块和一个应用程序：

* pwm_subsystem.ko

* pwm_subsystem_app

加载驱动程序和内核调试信息：

1. pwm_subsystem.ko

```bash
# insmod pwm_subsystem.ko

   [  295.130691] pwm driver probe
   [  295.130864] major=236, minor=0


# ./pwm_subsystem_app /dev/pwm_subsystem

   [  608.230477] pwm device open
   [  608.230723] pwm set: period=10000 ns, duty=5000 ns, polarity=0, enable=1
   [  608.230864] pwm device release

# cat /sys/kernel/debug/pwm

   platform/fe6f0010.pwm, 1 PWM device
   pwm-0   (pwm-test            ): requested enabled period: 10000 ns duty: 5000 ns polarity: normal

   platform/fe6e0010.pwm, 1 PWM device
   pwm-0   (backlight1          ): requested period: 50000 ns duty: 0 ns polarity: normal

   platform/fe6e0000.pwm, 1 PWM device
   pwm-0   (backlight           ): requested period: 50000 ns duty: 0 ns polarity: normal

   platform/fdd70000.pwm, 1 PWM device
   pwm-0   ((null)              ): period: 0 ns duty: 0 ns polarity: inverse
```