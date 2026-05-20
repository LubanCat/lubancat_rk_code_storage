#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

// 定义32位ARGB8888格式颜色常量（A=透明度 R=红 G=绿 B=蓝）
#define RED      0xFFFF0000  // 红色：透明度255，红色255，绿色0，蓝色0
#define GREEN    0xFF00FF00  // 绿色：透明度255，红色0，绿色255，蓝色0
#define BLUE     0xFF0000FF  // 蓝色：透明度255，红色0，绿色0，蓝色255
#define YELLOW   0xFFFFFF00  // 黄色：透明度255，红色255，绿色255，蓝色0
#define WHITE    0xFFFFFFFF  // 白色：透明度255，红/绿/蓝全255
#define BLACK    0xFF000000  // 黑色：透明度255，红/绿/蓝全0

/**
 * @brief 全屏纯色填充函数（32位色深）
 * @param fb_addr    帧缓冲显存指针
 * @param color      要填充的颜色值
 * @param screen_size 屏幕总字节数
 */
void fill_color(uint32_t *fb_addr, uint32_t color, int screen_size)
{
    // 计算屏幕总像素数：32位像素 = 4字节，总字节数/4=像素个数
    int pix_count = screen_size / 4;
    // 循环遍历所有像素，填充指定颜色
    for (int i = 0; i < pix_count; i++) {
        fb_addr[i] = color;  // 给每个像素点赋值
    }
}

/**
 * @brief  获取并打印帧缓冲硬件信息
 * @param  fp      帧缓冲设备文件描述符
 * @param  finfo   存储帧缓冲固定信息的结构体
 * @param  vinfo   存储帧缓冲可变信息的结构体
 */
void fb_get_info(int fp, struct fb_fix_screeninfo *finfo, struct fb_var_screeninfo *vinfo)
{
    long screensize;

    // IOCTL获取帧缓冲固定信息（显存地址、行宽等不可修改参数）
    if (ioctl(fp, FBIOGET_FSCREENINFO, finfo)) {
        perror("读取固定信息失败");
        exit(2);
    }

    // IOCTL获取帧缓冲可变信息（分辨率、偏移等可修改参数）
    if (ioctl(fp, FBIOGET_VSCREENINFO, vinfo)) {
        perror("读取可变信息失败");
        exit(3);
    }

    // 计算单屏字节大小 = 行宽 × 垂直分辨率
    screensize = finfo->line_length * vinfo->yres;

    // 打印帧缓冲硬件信息
    printf("========== 帧缓冲信息 ==========\n");
    printf("设备ID: %s\n", finfo->id);                          // 帧缓冲设备名称
    printf("物理地址: 0x%lx, 总大小: %u 字节\n", (unsigned long)finfo->smem_start, finfo->smem_len);  // 显存物理地址+总大小
    printf("行宽: %u 字节\n", finfo->line_length);              // 每行像素的字节数
    printf("分辨率: %dx%d, 位深: %u 位\n", vinfo->xres, vinfo->yres, vinfo->bits_per_pixel);  // 屏幕分辨率+色深
    printf("虚拟分辨率: %dx%d\n", vinfo->xres_virtual, vinfo->yres_virtual);  // 虚拟分辨率
    printf("=================================\n");
}

int main()
{
    int fb_fd;                          // 帧缓冲设备文件描述符
    struct fb_var_screeninfo vinfo;     // 帧缓冲可变参数结构体
    struct fb_fix_screeninfo finfo;     // 帧缓冲固定参数结构体
    void *fb_base;                      // 显存映射到用户空间的基地址
    uint32_t *fb_addr;                  // 前台显示缓冲指针
    uint32_t *fb_back_buf;              // 后台显示缓冲指针
    int screen_size;                    // 单屏总字节大小
    int i;                              // 循环计数变量

    // 打开帧缓冲设备 /dev/fb0，读写模式
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        perror("打开 /dev/fb0 失败");
        exit(1);
    }

    // 获取并打印帧缓冲硬件参数
    fb_get_info(fb_fd, &finfo, &vinfo);

    // 将内核态显存映射到用户空间，方便直接操作
    // 参数：NULL(系统自动分配地址)、显存总大小、读写权限、共享映射、设备fd、偏移0
    fb_base = mmap(NULL, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_base == MAP_FAILED) {
        perror("mmap 映射失败");
        close(fb_fd);
        exit(4);
    }

    // 强制转换为32位指针，匹配ARGB8888颜色格式
    fb_addr = (uint32_t *)fb_base;
    // 打印用户空间显存地址
    printf("显存虚拟地址: %p\n", fb_addr);

    // 计算单屏总字节大小 = 行宽 × 垂直分辨率
    screen_size = finfo.line_length * vinfo.yres;

    // 后台缓冲地址 = 前台地址 + 单屏像素数
    fb_back_buf = fb_addr + (screen_size / 4);

    // 预填充两个缓冲：前台蓝色，后台红色
    fill_color(fb_addr, BLUE, screen_size);       // 物理屏填充蓝色
    fill_color(fb_back_buf, RED, screen_size);    // 虚拟屏填充红色

    // 初始化显示：X/Y轴偏移量0，显示前台缓冲颜色
    vinfo.xoffset = 0;
    vinfo.yoffset = 0;
    ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);  // 刷新屏幕
    sleep(1);

    // ===================== 双缓冲红蓝颜色切换循环 =====================
    printf("\n双缓冲红蓝颜色切换5次测试开始！\n");
    while (1) {
        // 循环5次：红蓝交替切换
        for(i=0;i<5;i++) {

            // 切换Y轴偏移=屏幕高度，显示后台缓冲
            vinfo.yoffset = vinfo.yres;
            ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);
            printf("显示：红色\n");
            sleep(1);

            // 切换Y轴偏移=0，显示前台缓冲
            vinfo.yoffset = 0;
            ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);
            printf("显示：蓝色\n");
            sleep(1);
        }

        break;
    }

    // 测试结束，填充黑色清屏
    fill_color(fb_addr, BLACK, screen_size);
    ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);

    // 释放用户空间映射的显存
    munmap(fb_base, finfo.smem_len);
    // 关闭帧缓冲设备文件
    close(fb_fd);

    printf("测试完成！\n");
    return 0;
}