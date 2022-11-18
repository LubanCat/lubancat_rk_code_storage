#psutil包
import psutil
import datetime

#获取当前用户信息，输出的name用户名、terminal终端、host主机地址、started登录时间、pid 进程id
print("当前用户:", psutil.users())

#获取当前系统时间，转换成自然时间格式
print("当前系统时间:", datetime.datetime.fromtimestamp(psutil.boot_time()).strftime("%Y-%m-%d %H: %M: %S"))  

print("\n-----------内存信息-------------------")
mem = psutil.virtual_memory()
print("系统内存全部信息:", mem)
mem_total = float(mem.total)
mem_used = float(mem.used)
mem_free = float(mem.free)
mem_percent = float(mem.percent)
print(f"系统总计内存：{mem_total}")
print(f"系统已经使用内存：{mem_used}")
print(f"系统空闲内存：{mem_free}")
print(f"系统内存使用率：{mem_percent}")

print("\n-----------CPU信息-------------------")
print("CPU汇总信息:", psutil.cpu_times())
print("cpu逻辑个数:", psutil.cpu_count())
print("cpu频率:", psutil.cpu_freq())

