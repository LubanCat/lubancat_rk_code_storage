#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "oled_app.h"
int fd = -1;

/*程序中使用到的点阵 字库*/
extern const unsigned char F16x16[];
extern const unsigned char F6x8[][6];
extern const unsigned char F8X16[];
extern const unsigned char BMP1[];

/* IIC 写函数
*   参数说明：fd，打开的设备文件句柄。 rg, 命令值。 val，要写入的数据
*   返回值：  成功，返回0. 失败，返回 -1
*/

static int i2c_write(int fd, unsigned char addr,unsigned char reg,unsigned char val)
{
   int retries;
   unsigned char data[2];

   data[0] = reg;
   data[1] = val;

   //设置地址长度：0为7位地址
   ioctl(fd,I2C_TENBIT,0);

   //设置从机地址
   if (ioctl(fd,I2C_SLAVE,addr) < 0){
      printf("fail to set i2c device slave address!\n");
      close(fd);
      return -1;
   }

   //设置收不到ACK时的重试次数
   ioctl(fd,I2C_RETRIES,5);

   if (write(fd, data, 2) == 2){
      return 0;
   }
   else{
      return -1;
   }
}


/*  初始化 OLED 
*   参数说明：fd，打开的设备文件句柄。 rg, 命令值。 val，要写入的数据
*   返回值：  成功，返回0. 失败，返回 -1
*/
void OLED_Init(int fd,unsigned char addr)
{
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xAE); //display off
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x20); //Set Memory Addressing Mode
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xb0); //Set Page Start Address for Page Addressing Mode,0-7
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xc8); //Set COM Output Scan Direction
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x00); //---set low column address
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x10); //---set high column address
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x40); //--set start line address
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x81); //--set contrast control register
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xf);  //亮度调节 0x00~0xff
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xa1); //--set segment re-map 0 to 127
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xa6); //--set normal display
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xa8); //--set multiplex ratio(1 to 64)
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x3F); //
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xa4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xd3); //-set display offset
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x00); //-not offset
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xd5); //--set display clock divide ratio/oscillator frequency
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xf0); //--set divide ratio
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xd9); //--set pre-charge period
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x22); //
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xda); //--set com pins hardware configuration
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x12);
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xdb); //--set vcomh
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x20); //0x20,0.77xVcc
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x8d); //--set DC-DC enable
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0x14); //
    i2c_write(fd,addr, OLED_COMMEND_ADDR, 0xaf); //--turn on oled panel
}

/**
  * @brief  OLED_Fill，填充整个屏幕
  * @param  fill_Data:要填充的数据
	* @retval 无
  */
void OLED_Fill(unsigned char addr,unsigned char fill_Data) //全屏填充
{
    unsigned char m, n;
    for (m = 0; m < 8; m++)
    {
        i2c_write(fd,addr ,OLED_COMMEND_ADDR, 0xb0 + m); //page0-page1
        i2c_write(fd,addr ,OLED_COMMEND_ADDR, 0x00);     //low column start address
        i2c_write(fd,addr ,OLED_COMMEND_ADDR, 0x10);     //high column start address

        for (n = 0; n < 128; n++)
        {
            // WriteDat(fill_Data);
            i2c_write(fd,addr,OLED_DATA_ADDR, fill_Data); //high column start address
        }
    }
}

 /**
  * @brief  OLED_SetPos，设置光标
  * @param  x,光标x位置  y，光标y位置
  *					
  * @retval 无
  */
void oled_set_Pos(unsigned char addr,unsigned char x, unsigned char y) //设置起始点坐标
{ 
	i2c_write(fd,addr,OLED_COMMEND_ADDR,0xb0+y);
	i2c_write(fd,addr,OLED_COMMEND_ADDR,((x&0xf0)>>4)|0x10);
	i2c_write(fd,addr,OLED_COMMEND_ADDR,(x&0x0f)|0x01);
}


 /**
  * @brief  OLED_CLS，清屏
  * @param  无
	* @retval 无
  */
void OLED_CLS(unsigned char addr)//清屏
{
	OLED_Fill(addr,0x00);
}

 /**
  * @brief  OLED_ON，将OLED从休眠中唤醒
  * @param  无
	* @retval 无
  */
void OLED_ON(unsigned char addr)
{
	i2c_write(fd, addr,OLED_COMMEND_ADDR,0X8D);  //设置电荷泵
	i2c_write(fd, addr,OLED_COMMEND_ADDR,0X14);  //开启电荷泵
	i2c_write(fd, addr,OLED_COMMEND_ADDR,0XAF);  //OLED唤醒
}

 /**
  * @brief  OLED_OFF，让OLED休眠 -- 休眠模式下,OLED功耗不到10uA
  * @param  无
	* @retval 无
  */
void OLED_OFF(unsigned char addr)
{
	i2c_write(fd,addr, OLED_COMMEND_ADDR,0X8D);  //设置电荷泵
	i2c_write(fd,addr, OLED_COMMEND_ADDR,0X10);  //关闭电荷泵
	i2c_write(fd,addr, OLED_COMMEND_ADDR,0XAE);  //OLED休眠
}

 /**
  * @brief  OLED_ShowStr，显示codetab.h中的ASCII字符,有6*8和8*16可选择
  * @param  x,y : 起始点坐标(x:0~127, y:0~7);
	*					ch[] :- 要显示的字符串; 
	*					TextSize : 字符大小(1:6*8 ; 2:8*16)
	* @retval 无
  */
void OLED_ShowStr(unsigned char addr,unsigned char x, unsigned char y, unsigned char ch[], unsigned char TextSize)
{
	unsigned char c = 0,i = 0,j = 0;
	switch(TextSize)
	{
		case 1:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 126)
				{
					x = 0;
					y++;
				}
				oled_set_Pos(addr,x,y);
				for(i=0;i<6;i++)
					i2c_write(fd, addr,OLED_DATA_ADDR,F6x8[c][i]);
				x += 6;
				j++;
			}
		}break;
		case 2:
		{
			while(ch[j] != '\0')
			{
				c = ch[j] - 32;
				if(x > 120)
				{
					x = 0;
					y++;
				}  
				oled_set_Pos(addr,x,y);
				for(i=0;i<8;i++)
					i2c_write(fd,addr, OLED_DATA_ADDR,F8X16[c*16+i]);
				oled_set_Pos(addr,x,y+1);
				for(i=0;i<8;i++)
					i2c_write(fd, addr,OLED_DATA_ADDR,F8X16[c*16+i+8]);
				x += 8;
				j++;
			}
		}break;
	}
}


 /**
  * @brief  OLED_ShowCN，显示codetab.h中的汉字,16*16点阵
  * @param  x,y: 起始点坐标(x:0~127, y:0~7); 
	*					N:汉字在codetab.h中的索引
	* @retval 无
  */
void OLED_ShowCN(unsigned char addr,unsigned char x, unsigned char y, unsigned char N)
{
	unsigned char wm=0;
	unsigned int  adder=32*N;
	oled_set_Pos(addr,x , y);
	for(wm = 0;wm < 16;wm++)
	{
		i2c_write(fd, addr,OLED_DATA_ADDR,F16x16[adder]);
		adder += 1;
	}
	oled_set_Pos(addr,x,y + 1);
	for(wm = 0;wm < 16;wm++)
	{
	  i2c_write(fd, addr,OLED_DATA_ADDR,F16x16[adder]);
		adder += 1;
	}
}

 /**
  * @brief  OLED_DrawBMP，显示BMP位图
  * @param  x0,y0 :起始点坐标(x0:0~127, y0:0~7);
	*					x1,y1 : 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
	* @retval 无
  */
void OLED_DrawBMP(unsigned char addr,unsigned char x0,unsigned char y0,unsigned char x1,unsigned char y1,unsigned char BMP[])
{
	unsigned int j=0;
	unsigned char x,y;

  if(y1%8==0)
		y = y1/8;
  else
		y = y1/8 + 1;
	for(y=y0;y<y1;y++)
	{
		oled_set_Pos(addr,x0,y);
    for(x=x0;x<x1;x++)
		{
			i2c_write(fd, addr,OLED_DATA_ADDR,BMP[j++]);
		}
	}
}

