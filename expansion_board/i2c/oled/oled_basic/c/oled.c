/*
 *
 *   file: oled.c
 *   update: 2024-12-04
 *  
 *   note:
 *      该oled驱动文件中的所有绘图函数出处：https://github.com/BaudDance/LEDFont
 */

#include "oled.h"

static int file;
static char filename[20];
static uint8_t OLED_GRAM[OLED_PAGE][OLED_COLUMN];

/*****************************
 * @brief : oled写命令
 * @param : cmd 要写入的命令
 * @return: none
 *****************************/
static void oled_write_command(uint8_t cmd)
{
    if (file > 0)
        i2c_smbus_write_byte_data(file, 0x00, cmd);
    else
        return;
}

/*****************************
 * @brief : oled写数据
 * @param : data 要写入的数据
 * @return: none
 *****************************/
static void oled_write_data(uint8_t data)
{
    if (file > 0)
        i2c_smbus_write_byte_data(file, 0x40, data);
    else
        return;
}

/*****************************
 * @brief : oled写多个数据
 * @param : data 要写入的数据
 * @param : len 数据长度
 * @return: none
 *****************************/
static void oled_write_ndata(uint8_t *data, uint8_t len)
{
    if (len <= 0)
        return;

    for (int i = 0; i < len; i++)
        oled_write_data(data[i]);
}

/*****************************
 * @brief : 设置数据写入的起始行列
 * @param : x 列起始位置
 * @param : y 行起始位置
 * @return: none
 *****************************/
static void oled_set_pos(uint8_t x, uint8_t y)
{
    oled_write_command(0xb0 + y);
    oled_write_command((x & 0x0f));
    oled_write_command(((x & 0xf0) >> 4) | 0x10);
}

/*****************************
 * @brief : 显示单个字符
 * @param : x 列起始位置
 * @param : y 行起始位置
 * @param : achar 要显示的单个字符（size: 8*16）
 * @return: none
 *****************************/
void oled_show_char(uint8_t x, uint8_t y, uint8_t achar)
{
    uint8_t c = 0, i = 0;
    c = achar - ' ';

    if (x > 127)
    {
        x = 0;
        y = y + 2;
    }

    if (SIZE == 16)
    {
        oled_set_pos(x, y);
        for (i = 0; i < 8; i++)
            oled_write_data(F8X16[c * 16 + i]);

        oled_set_pos(x, y + 1);
        for (i = 0; i < 8; i++)
            oled_write_data(F8X16[c * 16 + i + 8]);
    }
}

/*****************************
 * @brief : 显示一行字符串
 * @param : x 列起始位置
 * @param : y 行起始位置
 * @param : string 要显示的字符串
 * @return: none
 *****************************/
void oled_show_string(uint8_t x, uint8_t y, const uint8_t *string)
{
    uint8_t n_char = 0;

    while (string[n_char] != '\0')
    {
        oled_show_char(x, y, string[n_char]);

        x += 8;
        if (x >= 128)
        {
            x = 0;
            y += 2;
        }

        n_char++;
    }
}

/*****************************
 * @brief : 显示单个中文字符
 * @param : x 列起始位置
 * @param : y 行起始位置
 * @param : no 汉字编号（size: 16*16）
 * @return: none
 *****************************/
void oled_show_chinese(uint8_t x, uint8_t y, uint8_t no)
{
    uint8_t step = no * 2;
    uint8_t i = 0;

    if (x > 127)
    {
        x = 0;
        y = y + 2;
    }

    if (SIZE == 16)
    {
        oled_set_pos(x, y);
        for (i = 0; i < 16; i++)
            oled_write_data(F16X16[step][i]);

        oled_set_pos(x, y + 1);
        for (i = 0; i < 16; i++)
            oled_write_data(F16X16[step + 1][i]);
    }
}

/*****************************
 * @brief : oled清屏
 * @param : none
 * @return: none
 *****************************/
void oled_clear(void)
{
    uint8_t page, row;

    for (page = 0; page < 8; page++)
    {
        oled_write_command(0xb0 + page);
        oled_write_command(0x00);
        oled_write_command(0x10);

        for (row = 0; row < 128; row++)
            oled_write_data(0x00);
    }
}

/*****************************
 * @brief : oled清除某一页
 * @param : page, 要清除的页码，0-7
 * @return: none
 *****************************/
void oled_clear_page(int page)
{
    uint8_t row;

    if (page < 0 || page > 7)
        return;

    oled_write_command(0xb0 + page);
    oled_write_command(0x00);
    oled_write_command(0x10);

    for (row = 0; row < 128; row++)
        oled_write_data(0x00);
}

/*****************************
 * @brief : oled初始化
 * @param : i2c_bus i2c总线编号
 * @return: 0成功 -1失败
 *****************************/
int oled_init(int i2c_bus)
{
    file = open_i2c_dev(i2c_bus, filename, sizeof(filename), 0);
    if (file < 0)
    {
        printf("can't open %s\n", filename);
        return -1;
    }

    if (set_slave_addr(file, OLED_I2C_DEV_ADDR, 1))
    {
        printf("can't set_slave_addr\n");
        return -1;
    }

    oled_write_command(0xAE); // 显示打开

    oled_write_command(0x81); // 设置对比度
    oled_write_command(0xFF);

    oled_write_command(0xA4); // 使能全屏显示，恢复到RAM内容显示
    oled_write_command(0xA6); // 设置显示模式，正常显示：0灭1亮

    oled_write_command(0x2E); // 使能滚动
    oled_write_command(0x26); // 右水平滚动
    oled_write_command(0x00); // 虚拟字节
    oled_write_command(0x00); // 设置滚动起始页地址
    oled_write_command(0x03); // 设置滚动间隔
    oled_write_command(0x07); // 设置滚动结束地址

    oled_write_command(0x00); // 虚拟字节
    oled_write_command(0xFF);

    oled_write_command(0x20); // 寄存器寻址模式
    oled_write_command(0x10); // 页寻址模式
    oled_write_command(0xB0); // 设置页寻址的起始页地址（B0-B7:page0-page7）
    oled_write_command(0x00); // 设置页寻址的起始列地址低位
    oled_write_command(0x10); // 设置页寻址的起始列地址高位

    oled_write_command(0x40); // 设置显示开始线，0x40~0x7F对应0~63

    oled_write_command(0xA1); // 设置列重映射，addressX--->seg(127-X)

    oled_write_command(0xA8); // 设置多路复用比
    oled_write_command(0x3F);

    oled_write_command(0xC8); // 设置COM输出扫描方向，C8：COM63--->COM0(从下往上扫描)

    oled_write_command(0xD3); // 设置COM显示不偏移
    oled_write_command(0x00);

    oled_write_command(0xDA); // 配置COM重映射
    oled_write_command(0x12);

    oled_write_command(0xD9); // 设置预充期
    oled_write_command(0x22);

    oled_write_command(0xDB); // 设置VCOMH取消选择电平
    oled_write_command(0x20);

    oled_write_command(0x8d); // 设置电荷泵
    oled_write_command(0x14);

    oled_write_command(0xAF);

    oled_clear(); // 清屏

    return 0;
}

/* =================== 显存操作函数 ======================= */
/*****************************
 * @brief : 清空显存，绘制新的一帧
 * @param : none
 * @return: none
 *****************************/
void oled_new_frame()
{
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

/*****************************
 * @brief : 将当前显存显示到屏幕上
 * @param : none
 * @return: none
 *****************************/
void oled_show_frame()
{
    uint8_t send_buffer[OLED_COLUMN];
    for (int i = 0; i < OLED_PAGE; i++)
    {
        oled_write_command(0xB0 + i); // 设置页地址
        oled_write_command(0x00);     // 设置列地址低4位
        oled_write_command(0x10);     // 设置列地址高4位
        memcpy(send_buffer, OLED_GRAM[i], OLED_COLUMN);
        oled_write_ndata(send_buffer, OLED_COLUMN);
    }
}

/*****************************
 * @brief : 设置一个像素点
 * @param : x x坐标
 * @param: y y坐标
 * @return: none
 *****************************/
void oled_set_pixel(uint8_t x, uint8_t y)
{
    OLED_GRAM[y / 8][x] |= 0x01 << (y % 8);
}

/* =================== 绘图函数 ======================= */
/*****************************
 * @brief : 绘制一条线段
 * @param : x1 起始点横坐标
 * @param : y1 起始点纵坐标
 * @param : x2 终止点横坐标
 * @param : y2 终止点纵坐标
 * @return: none
 *****************************/
void oled_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    static uint8_t temp = 0;
    uint8_t x_8, y_8;
    int16_t dx;
    int16_t dy;
    int16_t ux;
    int16_t uy;
    int16_t x, y, eps;

    if (x1 == x2)
    {
        if (y1 > y2)
        {
            temp = y1;
            y1 = y2;
            y2 = temp;
        }
        for (y_8 = y1; y_8 <= y2; y_8++)
        {
            oled_set_pixel(x1, y_8);
        }
    }
    else if (y1 == y2)
    {
        if (x1 > x2)
        {
            temp = x1;
            x1 = x2;
            x2 = temp;
        }
        for (x_8 = x1; x_8 <= x2; x_8++)
        {
            oled_set_pixel(x_8, y1);
        }
    }
    else
    {
        // Bresenham直线算法
        dx = x2 - x1;
        dy = y2 - y1;
        ux = ((dx > 0) << 1) - 1;
        uy = ((dy > 0) << 1) - 1;
        x = x1;
        y = y1;
        eps = 0;
        dx = abs(dx);
        dy = abs(dy);
        if (dx > dy)
        {
            for (x = x1; x != x2; x += ux)
            {
                oled_set_pixel(x, y);
                eps += dy;
                if ((eps << 1) >= dx)
                {
                    y += uy;
                    eps -= dx;
                }
            }
        }
        else
        {
            for (y = y1; y != y2; y += uy)
            {
                oled_set_pixel(x, y);
                eps += dx;
                if ((eps << 1) >= dy)
                {
                    x += ux;
                    eps -= dy;
                }
            }
        }
    }
}

/*****************************
 * @brief 绘制一个矩形
 * @param : x 起始点横坐标
 * @param : y 起始点纵坐标
 * @param : w 矩形宽度
 * @param : h 矩形高度
 * @return: none
 *****************************/
void oled_draw_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    oled_draw_line(x, y, x + w, y);
    oled_draw_line(x, y + h, x + w, y + h);
    oled_draw_line(x, y, x, y + h);
    oled_draw_line(x + w, y, x + w, y + h);
}

/*****************************
 * @brief 绘制一个三角形
 * @param : x1 第一个点横坐标
 * @param : y1 第一个点纵坐标
 * @param : x2 第二个点横坐标
 * @param : y2 第二个点纵坐标
 * @param : x3 第三个点横坐标
 * @param : y3 第三个点纵坐标
 * @return: none
*****************************/
void oled_draw_triangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3)
{
    oled_draw_line(x1, y1, x2, y2);
    oled_draw_line(x2, y2, x3, y3);
    oled_draw_line(x3, y3, x1, y1);
}

/*****************************
 * @brief 绘制一个圆
 * @param x 圆心横坐标
 * @param y 圆心纵坐标
 * @param r 圆半径
 * @return: none
*****************************/
void oled_draw_circle(uint8_t x, uint8_t y, uint8_t r)
{
    int16_t a = 0, b = r, di = 3 - (r << 1);
    while (a <= b)
    {
        oled_set_pixel(x - b, y - a);
        oled_set_pixel(x + b, y - a);
        oled_set_pixel(x - a, y + b);
        oled_set_pixel(x - b, y - a);
        oled_set_pixel(x - a, y - b);
        oled_set_pixel(x + b, y + a);
        oled_set_pixel(x + a, y - b);
        oled_set_pixel(x + a, y + b);
        oled_set_pixel(x - b, y + a);
        a++;
        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
        oled_set_pixel(x + a, y + b);
    }
}

/*****************************
 * @brief 绘制一个椭圆
 * @param x 椭圆中心横坐标
 * @param y 椭圆中心纵坐标
 * @param a 椭圆长轴
 * @param b 椭圆短轴
 * @return: none
*****************************/
void oled_draw_ellipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b)
{
    int xpos = 0, ypos = b;
    int a2 = a * a, b2 = b * b;
    int d = b2 + a2 * (0.25 - b);
    while (a2 * ypos > b2 * xpos)
    {
        oled_set_pixel(x + xpos, y + ypos);
        oled_set_pixel(x - xpos, y + ypos);
        oled_set_pixel(x + xpos, y - ypos);
        oled_set_pixel(x - xpos, y - ypos);
        if (d < 0)
        {
            d = d + b2 * ((xpos << 1) + 3);
            xpos += 1;
        }
        else
        {
            d = d + b2 * ((xpos << 1) + 3) + a2 * (-(ypos << 1) + 2);
            xpos += 1, ypos -= 1;
        }
    }
    d = b2 * (xpos + 0.5) * (xpos + 0.5) + a2 * (ypos - 1) * (ypos - 1) - a2 * b2;
    while (ypos > 0)
    {
        oled_set_pixel(x + xpos, y + ypos);
        oled_set_pixel(x - xpos, y + ypos);
        oled_set_pixel(x + xpos, y - ypos);
        oled_set_pixel(x - xpos, y - ypos);
        if (d < 0)
        {
            d = d + b2 * ((xpos << 1) + 2) + a2 * (-(ypos << 1) + 3);
            xpos += 1, ypos -= 1;
        }
        else
        {
            d = d + a2 * (-(ypos << 1) + 3);
            ypos -= 1;
        }
    }
}