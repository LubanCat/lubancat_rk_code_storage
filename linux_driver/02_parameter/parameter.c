#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int itype=0;                                   /* 定义静态整型模块参数，默认初始化值为0 */
module_param(itype,int,S_IWUSR|S_IRUSR);              /* 注册itype为整型模块参数，权限S_IWUSR(用户可写)+S_IRUSR(用户可读) */
MODULE_PARM_DESC(itype,"this is int variable");       /* 为itype参数添加描述 */

static bool btype=0;                                  /* 定义静态布尔型模块参数，默认初始化值为0（false） */
module_param(btype,bool,S_IWUSR|S_IRUSR);             /* 注册btype为布尔型模块参数，权限用户可读可写 */
MODULE_PARM_DESC(btype,"this is bool variable");      /* 为btype参数添加描述 */

static char ctype=0;                                  /* 定义静态字节型模块参数，默认初始化值为0 */
module_param(ctype,byte,S_IWUSR|S_IRUSR);             /* 注册ctype为字节型模块参数，权限用户可读可写 */
MODULE_PARM_DESC(ctype,"this is byte variable");      /* 为ctype参数添加描述 */

static char *stype=0;                                 /* 定义静态字符串指针型模块参数，默认初始化值为空指针 */
module_param(stype,charp,S_IWUSR|S_IRUSR);            /* 注册stype为字符串指针型模块参数，权限用户可读可写 */
MODULE_PARM_DESC(stype,"this is charp variable");     /* 为stype参数添加描述 */

static int iarr[3] = {0, 1, 2};                       /* 定义静态整型数组模块参数，长度3，默认初始化值为{0,1,2} */
module_param_array(iarr, int,NULL, S_IWUSR|S_IRUSR);  /* 注册iarr为整型数组模块参数，NULL表示使用默认数组长度，权限用户可读可写 */
MODULE_PARM_DESC(iarr,"this is array of int");        /* 为iarr参数添加描述 */

static int __init parameter_init(void)                /* 定义模块初始化函数，__init宏标记该函数仅模块加载时执行 */
{
pr_info(KERN_INFO "parameter init!\n");               /* 打印模块初始化提示信息，KERN_INFO指定日志级别为信息级 */
pr_info(KERN_INFO "itype=%d\n",itype);                /* 打印整型模块参数itype的当前值 */
pr_info(KERN_INFO "btype=%d\n",btype);                /* 打印布尔型模块参数btype的当前值 */
pr_info(KERN_INFO "ctype=%d\n",ctype);                /* 打印字节型模块参数ctype的当前值 */
pr_info(KERN_INFO "stype=%s\n",stype);                /* 打印字符串指针模块参数stype的当前值 */
pr_info("*iarr* parameter: %d, %d, %d\n", iarr[0], iarr[1], iarr[2]);  /* 打印整型数组模块参数iarr的三个元素值 */
return 0;  
}

static void __exit parameter_exit(void)               /* 定义模块退出函数，__exit宏标记该函数仅模块卸载时执行 */
{
printk(KERN_INFO "parameter exit!\n");                /* 打印模块退出提示信息 */
}

EXPORT_SYMBOL(itype);                                 /* 将itype变量导出到内核符号表，供其他内核模块访问使用 */

int my_add(int a, int b)                              /* 定义全局加法函数，接收两个整型参数a（加数1）、b（加数2） */
{
   return a+b;                                        /* 返回两个参数的和 */
}

EXPORT_SYMBOL(my_add);                                /* 将my_add函数导出到内核符号表，供其他内核模块调用 */

int my_sub(int a, int b)                              /* 定义全局减法函数，接收两个整型参数a（被减数）、b（减数） */
{
   return a-b;                                        /* 返回两个参数的差 */
}

EXPORT_SYMBOL(my_sub);                                /* 将my_sub函数导出到内核符号表，供其他内核模块调用 */

module_init(parameter_init);                          /* 注册parameter_init为模块加载时的初始化函数，是模块入口 */
module_exit(parameter_exit);                          /* 注册parameter_exit为模块卸载时的退出函数，是模块出口 */

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>"); /* 声明模块作者信息 */
MODULE_DESCRIPTION("parameter module");               /* 声明模块功能描述 */
MODULE_LICENSE("GPL");                                /* 声明模块许可证为GPL */