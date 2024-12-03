#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>

#define WS2812_DATA_GPIOCHIP     3
#define WS2812_DATA_GPIONUM      19

struct ws2812_mes {
    unsigned int gpiochip;      // data引脚的gpiochip
    unsigned int gpionum;       // data引脚的gpionum
    unsigned int lednum;        // 要控制灯带的第几个LED，序号从1开始
    unsigned char color[3];     // color[0]:color[1]:color[2]   R:G:B 
};

// RGB to HSV 转换函数
void rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b, float *h, float *s, float *v) 
{
    float rf = r / 255.0, gf = g / 255.0, bf = b / 255.0;
    float max = fmaxf(fmaxf(rf, gf), bf);
    float min = fminf(fminf(rf, gf), bf);
    float delta = max - min;

    // 计算亮度
    *v = max;

    // 计算饱和度
    *s = (max == 0) ? 0 : (delta / max);

    // 计算色调
    if (delta == 0) 
        *h = 0;  // 灰色无色调
    else if (max == rf) 
        *h = 60 * (fmodf(((gf - bf) / delta), 6));
    else if (max == gf) 
        *h = 60 * (((bf - rf) / delta) + 2);
    else 
        *h = 60 * (((rf - gf) / delta) + 4);

    if (*h < 0) 
        *h += 360;
}

// HSV to RGB 转换函数
void hsv_to_rgb(float h, float s, float v, unsigned char *r, unsigned char *g, unsigned char *b) 
{
    int i;
    float f, p, q, t;

    if (s == 0) 
    {   
        // 无色
        *r = *g = *b = (unsigned char)(v * 255);
    } 
    else 
    {
        h /= 60;  // 将角度缩放到 [0,6)
        i = (int)h;
        f = h - i;
        p = v * (1 - s);
        q = v * (1 - s * f);
        t = v * (1 - s * (1 - f));

        switch (i) 
        {
            case 0: *r = (unsigned char)(v * 255); *g = (unsigned char)(t * 255); *b = (unsigned char)(p * 255); break;
            case 1: *r = (unsigned char)(q * 255); *g = (unsigned char)(v * 255); *b = (unsigned char)(p * 255); break;
            case 2: *r = (unsigned char)(p * 255); *g = (unsigned char)(v * 255); *b = (unsigned char)(t * 255); break;
            case 3: *r = (unsigned char)(p * 255); *g = (unsigned char)(q * 255); *b = (unsigned char)(v * 255); break;
            case 4: *r = (unsigned char)(t * 255); *g = (unsigned char)(p * 255); *b = (unsigned char)(v * 255); break;
            default: *r = (unsigned char)(v * 255); *g = (unsigned char)(p * 255); *b = (unsigned char)(q * 255); break;
        }
    }
}

int main(int argc, char **argv) 
{
    struct ws2812_mes ws2812;
    int fd;
    int led_num = atoi(argv[1]);
    unsigned char r, g, b;
    float h, s, v;
    int j = 0;

    if (argc != 3) 
    {
        printf("Usage: %s <LED number> <color (RRGGBB)>\n", argv[0]);
        return -1;
    }
    
    ws2812.gpiochip = WS2812_DATA_GPIOCHIP;
    ws2812.gpionum = WS2812_DATA_GPIONUM;
    ws2812.lednum = led_num;

    // 解析输入颜色
    sscanf(argv[2], "%2hhx%2hhx%2hhx", &r, &g, &b);
    rgb_to_hsv(r, g, b, &h, &s, &v);

    // 打开 ws2812 设备
    fd = open("/dev/ws2812", O_RDWR);
    if (fd == -1) 
    {
        perror("Failed to open /dev/ws2812");
        return -1;
    }

    for(j = 0; j < 1; j++)
    {
        // 从暗到亮
        for (int i = 0; i <= 100; i++) 
        {
            float brightness = i / 100.0;       // 更多亮度步长
            unsigned char r_out, g_out, b_out;
            hsv_to_rgb(h, s, brightness * v, &r_out, &g_out, &b_out);

            ws2812.color[0] = r_out;
            ws2812.color[1] = g_out;
            ws2812.color[2] = b_out;

            printf("0x%x 0x%x 0x%x\n", ws2812.color[0], ws2812.color[1], ws2812.color[2]);

            write(fd, &ws2812, sizeof(ws2812));
            usleep(20000);                      // 调整呼吸速度
        }

        // 从亮到暗
        for (int i = 100; i >= 0; i--) 
        {
            float brightness = i / 100.0;       // 更多亮度步长
            unsigned char r_out, g_out, b_out;
            hsv_to_rgb(h, s, brightness * v, &r_out, &g_out, &b_out);

            ws2812.color[0] = r_out;
            ws2812.color[1] = g_out;
            ws2812.color[2] = b_out;

            printf("0x%x 0x%x 0x%x\n", ws2812.color[0], ws2812.color[1], ws2812.color[2]);

            write(fd, &ws2812, sizeof(ws2812));
            usleep(20000);                      // 调整呼吸速度
        }
    }

    close(fd);
    return 0;

}
