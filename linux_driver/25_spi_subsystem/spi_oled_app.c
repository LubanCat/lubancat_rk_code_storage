#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

/* 类型定义 */
typedef unsigned char u8;
typedef unsigned int u32;

/* OLED 屏幕参数 */
#define X_WIDTH  128   // 屏宽
#define Y_WIDTH  64    // 屏高

/* 显示数据结构体 */
struct oled_display_struct {
    u8 x;                      /* 显示起始X坐标 */
    u8 y;                      /* 显示起始Y坐标(页) */
    u32 length;                /* 显示数据长度 */
    u8 display_buffer[];       /* 显示数据缓冲区 */
};

/* hello world 8x16字模（16字节/字符）*/
unsigned char F8x16[] = {
    0x10,0xF0,0x00,0x80,0x80,0x80,0x00,0x00,0x20,0x3F,0x21,0x00,0x00,0x20,0x3F,0x20,/*"h",0*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x1F,0x24,0x24,0x24,0x24,0x17,0x00,/*"e",1*/
    0x00,0x10,0x10,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,/*"l",2*/
    0x00,0x10,0x10,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,/*"l",3*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x1F,0x20,0x20,0x20,0x20,0x1F,0x00,/*"o",4*/
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,/*" ",5*/
    0x80,0x80,0x00,0x80,0x80,0x00,0x80,0x80,0x01,0x0E,0x30,0x0C,0x07,0x38,0x06,0x01,/*"w",6*/
    0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x1F,0x20,0x20,0x20,0x20,0x1F,0x00,/*"o",7*/
    0x80,0x80,0x80,0x00,0x80,0x80,0x80,0x00,0x20,0x20,0x3F,0x21,0x20,0x00,0x01,0x00,/*"r",8*/
    0x00,0x10,0x10,0xF8,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x3F,0x20,0x20,0x00,0x00,/*"l",9*/
    0x00,0x00,0x80,0x80,0x80,0x90,0xF0,0x00,0x00,0x1F,0x20,0x20,0x20,0x10,0x3F,0x20,/*"d",10*/
};

/****************************************************************************************
 * 函数：oled_show_one_letter
 * 功能：显示单个8x16字符
 * 参数：fd      - OLED设备文件描述符
 *       x       - 字符起始X坐标
 *       y       - 字符起始Y页坐标
 *       width   - 字符宽度（8像素）
 *       high    - 字符高度（16像素）
 *       data    - 字符字模数据指针
 * 返回：0成功，-1失败
 ***************************************************************************************/
int oled_show_one_letter(int fd, u8 x, u8 y, u8 width, u8 high, u8 *data)
{
    struct oled_display_struct *display_struct = NULL;

    /* 校验：OLED按8行分页，高度必须是8的整数倍 */
    if ((high % 8) != 0) {
        printf("字符高度设置错误！\n");
        return -1;
    }

    /* 计算字符占用页数：字符高度16像素对应16行，除以8行每页，得到占用页数为2页 */
    high = high / 8;

    /* 动态分配内存：结构体大小 + 字符总字节数（宽度*页数） */
    display_struct = malloc(sizeof(struct oled_display_struct) + width * high);
    if (!display_struct) {
        printf("内存分配失败！\n");
        return -1;
    }

    /* 循环：逐页显示字符，8x16字符需要显示2页 */
    do {
        /* 填充显示坐标 */
        display_struct->x = x;
        display_struct->y = y;
        /* 每页显示的字节数 = 字符宽度 */
        display_struct->length = width;
        /* 复制字模数据到发送缓冲区 */
        memcpy(display_struct->display_buffer, data, display_struct->length);
        /* 写入驱动，完成显示 */
        write(fd, display_struct, sizeof(struct oled_display_struct) + display_struct->length);

        /* 指向下一页字模数据 */
        data += display_struct->length;
        high--;    // 剩余页数-1
        y++;       // Y页坐标+1
    } while (high > 0);

    /* 释放内存 */
    free(display_struct);
    return 0;
}

/****************************************************************************************
 * 函数：oled_fill
 * 功能：OLED区域填充/清屏
 * 参数：fd        - 设备文件描述符
 *       start_x   - 填充起始X坐标
 *       start_y   - 填充起始Y页坐标
 *       end_x     - 填充结束X坐标
 *       end_y     - 填充结束Y页坐标
 *       data      - 填充数据（0x00全黑，0xFF全亮）
 * 返回：0成功，-1失败
 ***************************************************************************************/
int oled_fill(int fd, u8 start_x, u8 start_y, u8 end_x, u8 end_y, u8 data)
{
    struct oled_display_struct *display_struct;

    /* 坐标合法性校验 */
    if (end_x < start_x || end_y < start_y)
        return -1;

    /* 分配内存：结构体 + 一行填充的字节数 */
    display_struct = malloc(sizeof(struct oled_display_struct) + end_x - start_x + 1);
    /* 设置一行填充的长度 */
    display_struct->length = end_x - start_x + 1;
    /* 缓冲区全部填充为指定数据 */
    memset(display_struct->display_buffer, data, display_struct->length);

    /* 逐页填充，直到结束页 */
    for (; start_y <= end_y; start_y++) {
        display_struct->x = start_x;
        display_struct->y = start_y;
        write(fd, display_struct, sizeof(struct oled_display_struct) + display_struct->length);
    }

    /* 释放内存 */
    free(display_struct);
    return 0;
}

/****************************************************************************************
 * 函数：oled_show_F8x16
 * 功能：通用8x16字模显示函数，自动计算字符个数，16字节/字符
 * 参数：fd        - 设备文件描述符
 *       x         - 起始X坐标
 *       y         - 起始Y页坐标
 *       font_buf  - 8x16字模数组指针
 *       total_len - 字模总字节数，传入sizeof(font_buf)自动计算
 * 返回：0成功，-1失败
 ***************************************************************************************/
int oled_show_F8x16(int fd, u8 x, u8 y, u8 *font_buf, u32 total_len)
{
    u32 char_count = 0;  // 字符总数
    u32 i = 0;           // 循环计数

    /* 1. 字符个数 = 总字节数 / 单个字符字节数(每个字符16字节) */
    char_count = total_len / 16;
    if (char_count == 0)
        return -1;

    /* 2. 循环显示所有字符 */
    for (i = 0; i < char_count; i++) {
        /* 显示单个字符：字模偏移 = 字符索引 * 16字节 */
        oled_show_one_letter(fd, x, y, 8, 16, &font_buf[i * 16]);

        /* 字符宽度8像素，X坐标右移8列 */
        x += 8;

        /* 自动换行：超出屏幕宽度，切换到下一行 */
        if (x > X_WIDTH - 8) {
            x = 0;            // X坐标归零
            y += 2;           // Y页+2，因为8x16字符占2页
            if (y > 6) break; // 超出屏幕高度，停止显示
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int fd;

    /* 校验命令行参数：必须传入设备路径 */
    if (argc != 2) {
        printf("Usage: ./spi_oled_app /dev/spi_oled\n");
        return -1;
    }

    /* 打开文件 */
    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        printf("打开设备文件 %s 失败 !\n", argv[1]);
        return -1;
    }
    printf("设备打开成功，开始显示...\n");

    while (1) {
        // 1. 全屏清屏
        oled_fill(fd, 0, 0, 127, 7, 0x00);
        sleep(1);

        // 2. 通用函数居中显示 hello world
        oled_show_F8x16(fd, 20, 2, F8x16, sizeof(F8x16));
        printf("hello world 显示！\n");
        sleep(3);
    }

    close(fd);
    return 0;
}