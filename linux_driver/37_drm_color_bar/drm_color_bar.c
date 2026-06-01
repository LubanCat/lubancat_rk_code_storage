#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <drm/drm_device.h>
#include <drm/drm_crtc.h>
#include "rockchip_drm_drv.h"
#include "rockchip_drm_direct_show.h"

/* 定义显示缓冲区宽度 */
#define BUFFER_WIDTH        640
/* 定义显示缓冲区高度 */
#define BUFFER_HEIGHT       480
/* 定义像素格式 */
#define BUFFER_FORMAT       DRM_FORMAT_ARGB8888
/* 定义缓冲区数量 */
#define BUFFER_COUNT        2
/* 定义彩条总段数 */
#define BAR_TOTAL           8
/* 定义彩条偏移更新间隔 */
#define UPDATE_MS           1000

/* 默认PLANE、CRTC配置值 */
#define DEFAULT_PLANE_NAME     "Esmart0-win0"
#define DEFAULT_CRTC_INDEX     0

#define COLOR_WHITE        0xFFFFFFFF   /* 白色 */
#define COLOR_YELLOW       0xFFFFFF00   /* 黄色 */
#define COLOR_CYAN         0xFF00FFFF   /* 青色 */
#define COLOR_GREEN        0xFF00FF00   /* 绿色 */
#define COLOR_MAGENTA      0xFFFF00FF   /* 粉色 */
#define COLOR_RED          0xFFFF0000   /* 红色 */
#define COLOR_BLUE         0xFF0000FF   /* 蓝色 */
#define COLOR_BLACK        0xFF000000   /* 黑色 */

/* 驱动私有数据结构体 */
struct colorbar_drv_data {
    struct drm_device *ddev;            /* DRM设备句柄 */
    struct drm_crtc *crtc;              /* DRM显示控制器 */
    struct drm_plane *plane;            /* DRM图层 */
    struct rockchip_drm_direct_show_buffer *buf[BUFFER_COUNT];  /* Direct Show缓冲 */
    struct task_struct *thread;         /* 内核线程句柄 */
    int bar_offset;                     /* 彩条偏移量 */
    int front_buf;                      /* 显示缓冲索引 */
    int enable;                         /* 彩条显示使能开关 */
    char plane_name[32];                /* Plane名称 */
    int crtc_index;                     /* CRTC索引 */
    struct kobject *kobj;               /* sysfs kobject节点 */
    int hw_ready;                       /* 硬件绑定就绪标记 */
};

/* 定义全局驱动私有数据对象 */
static struct colorbar_drv_data drv_data;

/* sysfs读取彩条使能状态 */
static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", drv_data.enable);
}

/* sysfs设置彩条使能状态 */
static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr,
                            const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val) < 0)
        return -EINVAL;

    /* 仅允许0/1配置 */
    drv_data.enable = (val ? 1 : 0);
    return count;
}

/* sysfs读取当前绑定的Plane名称 */
static ssize_t plane_name_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%s\n", drv_data.plane_name);
}

/* sysfs修改绑定的Plane名称 */
static ssize_t plane_name_store(struct kobject *kobj, struct kobj_attribute *attr,
                                const char *buf, size_t count)
{
    /* 限制名称长度，防止溢出 */
    strncpy(drv_data.plane_name, buf, sizeof(drv_data.plane_name) - 1);
    /* 去除换行符 */
    drv_data.plane_name[strcspn(drv_data.plane_name, "\n")] = '\0';
    /* 修改配置后清空硬件就绪标记，下次使能重新绑定 */
    drv_data.hw_ready = 0;
    return count;
}

/* sysfs读取当前CRTC索引 */
static ssize_t crtc_index_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "%d\n", drv_data.crtc_index);
}

/* sysfs设置CRTC索引 */
static ssize_t crtc_index_store(struct kobject *kobj, struct kobj_attribute *attr,
                               const char *buf, size_t count)
{
    int val;
    if (kstrtoint(buf, 10, &val) < 0)
        return -EINVAL;

    /* 限制支持所有大于0的数字索引 */
    if (val >= 0)
        drv_data.crtc_index = val;
    /* 修改配置后清空硬件就绪标记，下次使能重新绑定 */
    drv_data.hw_ready = 0;
    return count;
}

/* 定义sysfs属性文件 */
static struct kobj_attribute enable_attr = __ATTR(enable, 0660, enable_show, enable_store);
static struct kobj_attribute plane_name_attr = __ATTR(plane_name, 0660, plane_name_show, plane_name_store);
static struct kobj_attribute crtc_index_attr = __ATTR(crtc_index, 0660, crtc_index_show, crtc_index_store);

/*
 * 函数功能：绘制带偏移量的彩条图案
 * @buf: 指向要绘制的显示缓冲区
 * @offset: 彩条水平偏移量
 */
static void draw_color_bar(struct rockchip_drm_direct_show_buffer *buf, int offset)
{
    u32 *pix = (u32 *)buf->vir_addr[0];      /* 映射显存32位像素指针 */
    u32 w = buf->width;                      /* 获取缓冲区宽度 */
    u32 h = buf->height;                     /* 获取缓冲区高度 */
    u32 pitch = buf->pitch[0] / 4;           /* 计算行跨度（4字节/像素） */
    u32 bar_width = w / BAR_TOTAL;           /* 计算单段彩条宽度 */
    int x, y;                                /* 像素坐标遍历变量 */
    u32 color;                               /* 当前像素颜色值 */

    /* 判断像素格式是否匹配，不匹配则直接返回 */
    if (buf->pixel_format != BUFFER_FORMAT)
        return;

    /* 遍历所有行 */
    for (y = 0; y < h; y++) {
        /* 遍历所有列 */
        for (x = 0; x < w; x++) {
            /* 计算当前像素对应彩条索引 */
            int idx = (x / bar_width + offset) % BAR_TOTAL;
            /* 根据索引选择对应颜色 */
            switch (idx) {
                case 0: color = COLOR_WHITE;   break;      /* 索引0：白色 */
                case 1: color = COLOR_YELLOW;  break;      /* 索引1：黄色 */
                case 2: color = COLOR_CYAN;    break;      /* 索引2：青色 */
                case 3: color = COLOR_GREEN;   break;      /* 索引3：绿色 */
                case 4: color = COLOR_MAGENTA; break;      /* 索引4：粉色 */
                case 5: color = COLOR_RED;     break;      /* 索引5：红色 */
                case 6: color = COLOR_BLUE;    break;      /* 索引6：蓝色 */
                default: color = COLOR_BLACK;  break;      /* 默认：黑色 */
            }
            /* 将颜色写入显存对应位置 */
            pix[y * pitch + x] = color;
        }
    }
}

/*
 * 函数功能：分配双显示缓冲区
 * @data: 驱动私有数据指针
 */
static int alloc_bufs(struct colorbar_drv_data *data)
{
    int i, ret;

    /* 循环分配2个缓冲区 */
    for (i = 0; i < BUFFER_COUNT; i++) {
        /* 分配Direct Show缓冲区结构体内存 */
        data->buf[i] = kzalloc(sizeof(*data->buf[i]), GFP_KERNEL);
        if (!data->buf[i]) {
            printk(KERN_ERR "rk-colorbar: alloc buf[%d] failed\n", i);
            return -ENOMEM;
        }

        /* 设置缓冲区宽度 */
        data->buf[i]->width = BUFFER_WIDTH;
        /* 设置缓冲区高度 */
        data->buf[i]->height = BUFFER_HEIGHT;
        /* 设置缓冲区像素格式 */
        data->buf[i]->pixel_format = BUFFER_FORMAT;
        /* 设置缓冲区为连续物理内存 */
        data->buf[i]->flag = ROCKCHIP_BO_CONTIG;

        /* 调用Direct Show接口分配硬件缓冲区 */
        ret = rockchip_drm_direct_show_alloc_buffer(data->ddev, data->buf[i]);
        if (ret) {
            printk(KERN_ERR "rk-colorbar: alloc buf[%d] failed %d\n", i, ret);
            kfree(data->buf[i]);
            data->buf[i] = NULL;
            return ret;
        }
    }

    return 0;
}

/*
 * 函数功能：释放双显示缓冲区
 * @data: 驱动私有数据指针
 */
static void free_bufs(struct colorbar_drv_data *data)
{
    int i;

    /* 循环释放2个缓冲区 */
    for (i = 0; i < BUFFER_COUNT; i++) {
        /* 判断缓冲区是否存在 */
        if (data->buf[i]) {
            /* 调用Direct Show接口释放硬件缓冲区 */
            rockchip_drm_direct_show_free_buffer(data->ddev, data->buf[i]);
            /* 释放结构体内存 */
            kfree(data->buf[i]);
            /* 清空指针 */
            data->buf[i] = NULL;
        }
    }
}

/*
 * 函数功能：遍历获取指定索引的CRTC，并等待其激活
 * @data: 驱动私有数据指针
 */
static int get_crtc(struct colorbar_drv_data *data)
{
    /* CRTC临时指针 */
    struct drm_crtc *crtc;
    /* CRTC索引计数器 */
    int index = 0;

    /* 遍历DRM设备下所有CRTC */
    drm_for_each_crtc(crtc, data->ddev) {
        /* 判断是否为目标CRTC索引 */
        if (index == data->crtc_index) {
            /* 等待CRTC硬件激活 */
            while (!crtc->state->active) {
                /* 休眠100ms，释放CPU */
                schedule_timeout_interruptible(msecs_to_jiffies(100));
            }
            /* 保存激活的CRTC到私有数据 */
            data->crtc = crtc;

            /* 打印CRTC获取成功日志 */
            printk(KERN_INFO "rk-colorbar: get CRTC%d", data->crtc_index);

            return 0;
        }
        /* CRTC索引自增 */
        index++;
    }

    /* 未找到目标CRTC，返回无设备错误 */
    return -ENODEV;
}

/*
 * 函数功能：等待所有显示硬件(DRM/Plane/CRTC)就绪
 * @data: 驱动私有数据指针
 */
static int wait_hw_ready(struct colorbar_drv_data *data)
{
    /* 循环等待硬件就绪 */
    while (1) {
        /* 获取Rockchip DRM设备句柄 */
        data->ddev = rockchip_drm_get_dev();
        /* 判断DRM设备是否获取成功 */
        if (!data->ddev) {
            /* 休眠100ms，等待DRM设备初始化 */
            schedule_timeout_interruptible(msecs_to_jiffies(100));
            
            /* 继续循环 */
            continue;
        }

        /* 获取指定名称的Plane图层 */
        data->plane = rockchip_drm_direct_show_get_plane(data->ddev, data->plane_name);
        /* 获取指定索引的CRTC，并判断Plane/CRTC是否全部就绪 */
        if (!get_crtc(data) && data->plane) {
            /* 打印硬件就绪日志 */
            printk(KERN_INFO "rk-colorbar: VP all hardware ready!");

            /* 返回成功 */
            return 0;
        }

        /* 硬件未就绪，休眠200ms后重试 */
        schedule_timeout_interruptible(msecs_to_jiffies(200));
    }
}

/*
 * 函数功能：重新绑定Plane和CRTC
 * @data: 驱动私有数据指针
 */
static void rebind_hardware(struct colorbar_drv_data *data)
{
    /* 关闭旧图层 */
    if (data->ddev && data->plane) {
        rockchip_drm_direct_show_disable_plane(data->ddev, data->plane);
        printk(KERN_INFO "rk-colorbar: disable old plane before rebind\n");
    }

    /* 清空旧的硬件绑定 */
    data->plane = NULL;
    data->crtc = NULL;

    /* 重新获取最新配置的Plane + CRTC */
    wait_hw_ready(data);

    /* 标记硬件就绪 */
    data->hw_ready = 1;

    printk(KERN_INFO "rk-colorbar: rebind hardware success (plane:%s, crtc:%d)",
           data->plane_name, data->crtc_index);
}

/*
 * 函数功能：彩条显示核心线程
 * @arg: 传递驱动私有数据指针
 */
static int colorbar_thread_func(void *arg)
{
    /* 转换参数为驱动私有数据指针 */
    struct colorbar_drv_data *data = (struct colorbar_drv_data *)arg;
    /* 定义Direct Show提交信息结构体 */
    struct rockchip_drm_direct_show_commit_info info = {0};
    /* 函数返回值变量 */
    int ret;

    /* 等待显示硬件全部初始化完成 */
    wait_hw_ready(data);
    /* 初始化硬件就绪标志 */
    data->hw_ready = 1;

    /* 分配双显示缓冲区 */
    ret = alloc_bufs(data);
    if (ret)
        /* 分配失败，退出线程 */
        return ret;

    /* 清空提交信息结构体 */
    memset(&info, 0, sizeof(info));
    /* 绑定目标CRTC */
    info.crtc    = data->crtc;
    /* 绑定目标Plane */
    info.plane   = data->plane;
    /* 设置源区域X坐标 */
    info.src_x   = 0;
    /* 设置源区域Y坐标 */
    info.src_y   = 0;
    /* 设置源区域宽度 */
    info.src_w   = BUFFER_WIDTH;
    /* 设置源区域高度 */
    info.src_h   = BUFFER_HEIGHT;
    /* 设置目标显示X坐标 */
    info.dst_x   = 0;
    /* 设置目标显示Y坐标 */
    info.dst_y   = 0;
    /* 设置目标显示宽度 */
    info.dst_w   = BUFFER_WIDTH;
    /* 设置目标显示高度 */
    info.dst_h   = BUFFER_HEIGHT;

    /* 初始化彩条偏移量为0 */
    data->bar_offset = 0;
    /* 初始化缓冲索引为0 */
    data->front_buf = 0;
    /* 绘制初始偏移的彩条到第一块缓冲区 */
    draw_color_bar(data->buf[data->front_buf], data->bar_offset);

    /* 打印彩条启动日志 */
    printk(KERN_INFO "rk-colorbar: VP start, 1s scroll\n");

    /* 线程主循环：监听停止信号，持续显示 */
    while (!kthread_should_stop()) {
        if (data->enable) {
            /* 配置变更/首次启动：重新绑定Plane和CRTC */
            if (!data->hw_ready) {
                rebind_hardware(data);
                /* 更新提交信息的硬件绑定 */
                info.crtc  = data->crtc;
                info.plane = data->plane;
            }

            /* 绑定当前缓冲区 */
            info.buffer = data->buf[data->front_buf];
            /* 提交画面到硬件显示 */
            rockchip_drm_direct_show_commit(data->ddev, &info);

            /* 更新彩条偏移量，循环8段 */
            data->bar_offset = (data->bar_offset + 1) % BAR_TOTAL;
            /* 切换缓冲区 */
            data->front_buf = !data->front_buf;
            /* 绘制新偏移的彩条到切换后的缓冲区 */
            draw_color_bar(data->buf[data->front_buf], data->bar_offset);
        } else {
            /* 调用官方接口关闭Plane */
            if (data->ddev && data->plane) {
                rockchip_drm_direct_show_disable_plane(data->ddev, data->plane);
                printk(KERN_INFO "rk-colorbar: plane disabled by enable=0\n");
                /* 关闭后清空指针，防止重复操作 */
                data->plane = NULL;
                data->crtc = NULL;
            }
            /* 关闭使能时，清空硬件就绪标记，允许下次重新绑定 */
            data->hw_ready = 0;
        }

        /* 休眠1秒，实现每秒更新一次 */
        schedule_timeout_interruptible(msecs_to_jiffies(UPDATE_MS));
    }
    return 0;
}

/* 驱动模块初始化函数 */
static int __init colorbar_driver_init(void)
{
    int ret;
    /* 清空全局私有数据结构体 */
    memset(&drv_data, 0, sizeof(drv_data));

    /* 初始化默认配置 */
    drv_data.enable = 0;
    strncpy(drv_data.plane_name, DEFAULT_PLANE_NAME, sizeof(drv_data.plane_name)-1);
    drv_data.crtc_index = DEFAULT_CRTC_INDEX;
    drv_data.hw_ready = 0; /* 初始化硬件未就绪 */

    /* 创建sysfs根节点 /sys/kernel/rk_colorbar/ */
    drv_data.kobj = kobject_create_and_add("rk_colorbar", kernel_kobj);
    if (!drv_data.kobj) {
        printk(KERN_ERR "rk-colorbar: create kobject failed\n");
        return -ENOMEM;
    }
    /* 创建sysfs属性文件 */
    ret = sysfs_create_file(drv_data.kobj, &enable_attr.attr);
    ret |= sysfs_create_file(drv_data.kobj, &plane_name_attr.attr);
    ret |= sysfs_create_file(drv_data.kobj, &crtc_index_attr.attr);
    if (ret) {
        printk(KERN_ERR "rk-colorbar: create sysfs file failed\n");
        kobject_put(drv_data.kobj);
        drv_data.kobj = NULL;
        return ret;
    }

    /* 创建彩条显示内核线程 */
    drv_data.thread = kthread_run(colorbar_thread_func, &drv_data, "rk-colorbar");
    /* 线程创建失败判断 */
    if (IS_ERR(drv_data.thread)) {
        /* 打印错误日志 */
        printk(KERN_ERR "rk-colorbar: create thread failed\n");
        /* 初始化失败释放sysfs资源 */
        sysfs_remove_file(drv_data.kobj, &enable_attr.attr);
        sysfs_remove_file(drv_data.kobj, &plane_name_attr.attr);
        sysfs_remove_file(drv_data.kobj, &crtc_index_attr.attr);
        kobject_put(drv_data.kobj);
        drv_data.kobj = NULL;
        /* 返回线程创建错误码 */
        return PTR_ERR(drv_data.thread);
    }

    /* 打印初始化日志 */
    printk(KERN_INFO "rk-colorbar: driver init, wait VP...\n");

    return 0;
}

/* 驱动模块退出函数 */
static void __exit colorbar_driver_exit(void)
{
    /* 退出前关闭图层 */
    if (drv_data.ddev && drv_data.plane) {
        rockchip_drm_direct_show_disable_plane(drv_data.ddev, drv_data.plane);
    }

    /* 判断内核线程是否存在 */
    if (drv_data.thread) {
        /* 停止内核线程 */
        kthread_stop(drv_data.thread);
        /* 清空线程指针 */
        drv_data.thread = NULL;
    }

    /* 释放双显示缓冲区 */
    free_bufs(&drv_data);

    /* 清空CRTC指针 */
    drv_data.crtc = NULL;
    /* 清空Plane指针 */
    drv_data.plane = NULL;
    /* 清空DRM设备指针 */
    drv_data.ddev = NULL;

    /* 销毁sysfs节点及属性 */
    if (drv_data.kobj) {
        sysfs_remove_file(drv_data.kobj, &enable_attr.attr);
        sysfs_remove_file(drv_data.kobj, &plane_name_attr.attr);
        sysfs_remove_file(drv_data.kobj, &crtc_index_attr.attr);
        kobject_put(drv_data.kobj);
        drv_data.kobj = NULL;
    }

    printk(KERN_INFO "rk-colorbar: driver exit\n");
}

/* 注册驱动为子系统同步初始化调用 */
subsys_initcall_sync(colorbar_driver_init);
/* 注册驱动模块退出函数 */
module_exit(colorbar_driver_exit);

MODULE_AUTHOR("embedfire <embedfire@embedfire.com>");
MODULE_DESCRIPTION("rockchip DRM Dual Buffer Color Bar");
MODULE_LICENSE("GPL v2");