/*
*
*   file: oled.c
*   update: 2024-08-09
*   function: 适用于i2c oled(ssd1306)模块
*   dependent files: i2cbusses.c、smbus.c
*
*/

#include "oled.h"

/*****************************
 * @brief : oled写命令
 * @param : cmd 要写入的命令
 * @return: none
*****************************/
static void oled_write_command(uint8_t cmd)
{
    i2c_smbus_write_byte_data(file, 0x00, cmd);
}

/*****************************
 * @brief : oled写数据
 * @param : data 要写入的数据
 * @return: none
*****************************/
static void oled_write_data(uint8_t data)
{
    i2c_smbus_write_byte_data(file, 0x40, data);
}

/*****************************
 * @brief : 设置数据写入的起始行列
 * @param : x 列起始位置
 * @param : y 行起始位置
 * @return: none
*****************************/
static void oled_set_pos(uint8_t x, uint8_t y)
{
    oled_write_command(0xb0+y);				    
	oled_write_command((x&0x0f));  				
	oled_write_command(((x&0xf0)>>4)|0x10);		
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
    uint8_t c=0, i=0;
    c = achar - ' ';

    if(x > 127)
    {
        x = 0;
        y = y + 2;
    }

    if(SIZE == 16)
    {
        oled_set_pos(x, y);
        for(i=0; i<8; i++)
            oled_write_data(F8X16[c*16+i]);

        oled_set_pos(x, y+1);
        for(i=0; i<8; i++)
            oled_write_data(F8X16[c*16+i+8]);
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
		if(x >= 128)
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

    if(x > 127)
    {
        x = 0;
        y = y + 2;
    }

    if(SIZE == 16)
    {
        oled_set_pos(x, y);
        for(i=0; i<16; i++)
            oled_write_data(F16X16[step][i]);

        oled_set_pos(x, y+1);
        for(i=0; i<16; i++)
            oled_write_data(F16X16[step+1][i]);
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

	for(page=0; page<8; page++)
	{
		oled_write_command(0xb0 + page);	    
		oled_write_command(0x00);      		
		oled_write_command(0x10);      		

		for(row=0; row<128; row++)
            oled_write_data(0x00);
	}
}

/*****************************
 * @brief : oled初始化
 * @param : none
 * @return: none
*****************************/
int oled_init(void)
{
    file = open_i2c_dev(i2c_bus, filename, sizeof(filename), 0);
	if (file < 0)
	{
		printf("can't open %s\n", filename);
		return -1;
	}

	if (set_slave_addr(file, dev_addr, 1))
	{
		printf("can't set_slave_addr\n");
		return -1;
	}

    oled_write_command(0xAE);    //显示打开

    oled_write_command(0x81);    //设置对比度
    oled_write_command(0xFF);

    oled_write_command(0xA4);    //使能全屏显示，恢复到RAM内容显示
    oled_write_command(0xA6);    //设置显示模式，正常显示：0灭1亮

    oled_write_command(0x2E);    //使能滚动
    oled_write_command(0x26);    //右水平滚动
    oled_write_command(0x00);    //虚拟字节
    oled_write_command(0x00);    //设置滚动起始页地址
    oled_write_command(0x03);    //设置滚动间隔
    oled_write_command(0x07);    //设置滚动结束地址

    oled_write_command(0x00);    //虚拟字节
    oled_write_command(0xFF);

    oled_write_command(0x20);    //寄存器寻址模式
    oled_write_command(0x10);    //页寻址模式
    oled_write_command(0xB0);    //设置页寻址的起始页地址（B0-B7:page0-page7）
    oled_write_command(0x00);    //设置页寻址的起始列地址低位
    oled_write_command(0x10);    //设置页寻址的起始列地址高位

    oled_write_command(0x40);    //设置显示开始线，0x40~0x7F对应0~63

    oled_write_command(0xA1);    //设置列重映射，addressX--->seg(127-X)

    oled_write_command(0xA8);    //设置多路复用比
    oled_write_command(0x3F);

    oled_write_command(0xC8);    //设置COM输出扫描方向，C8：COM63--->COM0(从下往上扫描)

    oled_write_command(0xD3);    //设置COM显示不偏移
    oled_write_command(0x00); 

    oled_write_command(0xDA);    //配置COM重映射
    oled_write_command(0x12);

    oled_write_command(0xD9);    //设置预充期
    oled_write_command(0x22); 

    oled_write_command(0xDB);    //设置VCOMH取消选择电平
    oled_write_command(0x20);

    oled_write_command(0x8d);    //设置电荷泵
    oled_write_command(0x14);

    oled_write_command(0xAF);

    oled_clear();                //清屏
}