#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int __init helloworld_init(void)
 {  
    printk( "Hello World Module Init\n");
 return 0;
}

static void __exit helloworld_exit(void)
{
    printk("Hello World Module Exit\n");
}

module_init(helloworld_init);
module_exit(helloworld_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("hello world module");
MODULE_LICENSE("GPL");