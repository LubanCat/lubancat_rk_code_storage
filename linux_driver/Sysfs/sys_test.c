#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include<linux/sysfs.h> 
#include<linux/kobject.h> 
#include <linux/err.h>
 
volatile int test_value = 0;
 
struct kobject *kobj_ref;

/*该函数被调用在sysfs文件被读时*/
static ssize_t sysfs_show(struct kobject *kobj, 
                struct kobj_attribute *attr, char *buf)
{
    pr_info("读sysfs!\n");
    return sprintf(buf, "test_value = %d\n", test_value);
}

/* 该函数被调用在sysfs文件被写时*/
static ssize_t sysfs_store(struct kobject *kobj, 
                struct kobj_attribute *attr,const char *buf, size_t count)
{
    pr_info("写sysfs!\n");
    sscanf(buf,"%d",&test_value);
    return count;
}

struct kobj_attribute sysfs_test_attr = __ATTR(test_value, 0664, sysfs_show, sysfs_store); 

/*模块初始化函数*/
static int __init sysfs_test_driver_init(void)
{
    /*创建一个目录在/sys下 */
    kobj_ref = kobject_create_and_add("sysfs_test",NULL);

    /*在sysfs_test目录下创建一个文件*/
    if(sysfs_create_file(kobj_ref,&sysfs_test_attr.attr)){
            pr_err("创建sysfs文件失败.....\n");
            goto error_sysfs;
    }

    pr_info("驱动模块初始化完成!\n");
    return 0;
 
error_sysfs:
        kobject_put(kobj_ref); 
        sysfs_remove_file(kernel_kobj, &sysfs_test_attr.attr);
        return -1;

}

/*模块退出函数*/
static void __exit sysfs_test_driver_exit(void)
{
    kobject_put(kobj_ref); 
    sysfs_remove_file(kernel_kobj, &sysfs_test_attr.attr);
    pr_info("设备驱动模块移除!\n");
}

module_init(sysfs_test_driver_init);
module_exit(sysfs_test_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedfire");
MODULE_DESCRIPTION("一个简单的使用sysfs的驱动程序");