#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

extern int itype;                                           /* 声明外部整型变量itype，该变量由parameter模块通过EXPORT_SYMBOL导出到内核符号表 */

int my_add(int a, int b);                                   /* 声明外部加法函数my_add，该函数由parameter模块导出，用于两数相加 */
int my_sub(int a, int b);                                   /* 声明外部减法函数my_sub，该函数由parameter模块导出，用于两数相减 */

static int __init calculation_init(void)                                                    /* 定义模块初始化函数，__init宏标记该函数仅加载时执行 */
{
    pr_info(KERN_INFO "calculation init!\n");                                               /* 打印模块初始化提示日志，KERN_INFO为信息级日志 */
    pr_info(KERN_INFO "itype+1 = %d, itype-1 = %d\n", my_add(itype,1), my_sub(itype,1));    /* 调用parameter模块导出的my_add/my_sub函数，计算itype+1和itype-1并打印结果 */
    return 0;
}

static void __exit calculation_exit(void)                                                   /* 定义模块退出函数，__exit宏标记该函数仅卸载时执行 */
{
    pr_info(KERN_INFO "calculation exit!\n");                                               /* 打印模块退出提示日志 */
}

module_init(calculation_init);                                                              /* 注册calculation_init为模块加载时的入口函数 */
module_exit(calculation_exit);                                                              /* 注册calculation_exit为模块卸载时的出口函数 */

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");                                       /* 声明模块作者 */
MODULE_DESCRIPTION("calculation module");                                                   /* 声明模块功能描述 */
MODULE_LICENSE("GPL");                                                                      /* 声明模块许可证为GPL */
