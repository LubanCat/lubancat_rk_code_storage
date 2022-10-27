#include "drm-core.h"
#include <ft2build.h>
#include <math.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

uint32_t color_table[6] = {RED,GREEN,BLUE,BLACK,WHITE,BLACK_BLUE};

//显示像素点
void show_pixel(uint32_t x , uint32_t y , uint32_t color)
{
	if(x > buf.width || y > buf.height)
		printf("wrong set\n");
	buf.vaddr[ y*buf.width + x] = color;
}

int utf_8_to_unicode_string(uint8_t *utf_8,uint16_t *word)
{
	int len = 0;
	int utf_8_size = strlen(utf_8);
	int utf_8_len = 0;
	uint16_t unicode[2];

	while(utf_8_size > 0){
		//1位utf_8转换为两位的unicode
		if(utf_8[utf_8_len] < 0x80){
			unicode[0] = 0;
			unicode[1] = utf_8[utf_8_len];
			word[len] = (unicode[0]<<8) | unicode[1];
			len ++;
			utf_8_len ++;
			utf_8_size--;
			continue;
		}
		//2位utf_8转换为两位的unicode
		else if(utf_8[utf_8_len] > 0xc0 & utf_8[utf_8_len] <0xe0){
			unicode[1] = (utf_8[utf_8_len+1]&0x3f) | ((utf_8[utf_8_len]<< 6)& 0xc0 );
			unicode[0] = ((utf_8[utf_8_len]>>2) & 0x07) ;
			word[len] = (unicode[0]<<8) | unicode[1];
			len ++;
			utf_8_len +=2;
			utf_8_size -=2;
			continue;
		}
		//3位utf_8转换为两位的unicode
		else if(utf_8[utf_8_len] > 0xe0 & utf_8[utf_8_len]<0xf0){
			unicode[1] = (utf_8[utf_8_len+2]&0x3f) | ((utf_8[utf_8_len+1] << 6)& 0xc0);
			unicode[0] = ((utf_8[utf_8_len+1]>>2)&0x0f) | ((utf_8[utf_8_len] <<4)& 0xf0) ;
			word[len] = (unicode[0]<<8) | unicode[1];
			len ++;
			utf_8_len +=3;
			utf_8_size -=3;
			continue;
		}
		//四位的utf_8转换需要三到四位的unicode码，这样不方便操作，
		//中文基本都可以用三位utf_8表示，因此,四位及以后的解码就没必要
		else
			return -1;
	}
	return len;
}

void draw_bitmap( FT_Bitmap* bitmap,FT_Int x_ori,FT_Int y_ori)
{
	FT_Int x, y;
	FT_Int x_count, y_count;
	unsigned char show;
	uint32_t buffer_size = bitmap->width * bitmap->rows;
	uint8_t buffer[buffer_size];
	uint32_t color;
	FT_Int  x_max = x_ori + bitmap->width;
	FT_Int  y_max = y_ori + bitmap->rows;

	memcpy(buffer,bitmap->buffer,buffer_size);

	for ( y = y_ori, y_count = 0; y < y_max; y++, y_count++ ){
		for ( x = x_ori, x_count = 0; x < x_max; x++, x_count++ ){
			
			if ( x < 0 || y < 0 || x >= buf.width || y >= buf.height )
			continue;
			//buf里的图像是存放八位的梯度值，需要自己转换成颜色才能显示，否则会表现蓝色
			show = buffer[y_count * bitmap->width + x_count];
			//梯度大于零，转换为相同强度的白色
			if(show > 0)
				color = (show&0xff)|((show&0xff)<<8)|((show&0xff)<<16);
			//直接为黑色，可以省略
			else
				color=0;
			//像素显示函数
			show_pixel(x, y , color);
		}
	}

}


int freetype_set_char(FT_Face face , int lcd_x,int lcd_y ,int font_size,int angle_degree,uint16_t word)
{
	FT_Matrix	  matrix;
	FT_Vector     pen;
	int charIdx;
	FT_GlyphSlot  slot= face->glyph;;
	double angle;
	int error;

	FT_Set_Pixel_Sizes(face, font_size, 0);
	//设置旋转
	angle = (1.0 * angle_degree/360) * 3.14159 * 2;
		/* set up matrix */
	matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
	matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
	matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
	matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );
		
	/* 转换：transformation */
    FT_Set_Transform(face, &matrix, 0);

	/*下列的三条可替换FT_Load_Char使用*/
	// charIdx = FT_Get_Char_Index(face,word);
	// FT_Load_Glyph(face,charIdx, FT_LOAD_DEFAULT); 
	// FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

	error = FT_Load_Char( face, word, FT_LOAD_RENDER );
	if (error){
		printf("FT_Load_Char error\n");
		return -1;
	}
	draw_bitmap( &slot->bitmap,lcd_x, lcd_y );
	return 0;
}

int main(int argc, char **argv)
{
	int i,j,count;
	FT_Library library;
	FT_Face face;
	int error;
	int font_size = 180;
	int angle_degree = 0;

	unsigned char str1[] = "野火科技";
	unsigned char str2[] = "www.embedfire.com";
	unsigned char str3[] = "abcdefghijklmnopq";
	uint16_t unicode[20];
	int unicode_size = 0;

	int ret;
	//初始化
	ret = drm_init();
	if(ret < 0){
		printf("drm init fail\n");
		return -1;
	}

	/* 显示矢量字体 */
	error = FT_Init_FreeType( &library );			   /* initialize library */
	/* error handling omitted */
	error = FT_New_Face( library, "file/simsun.ttc", 0, &face ); /* create face object */
	unicode_size = utf_8_to_unicode_string(str1,unicode);

	// 显示野火科技
	for(i = 0 ; i<unicode_size;i++)
		freetype_set_char(face,0+font_size*i,(buf.height-font_size)/2,font_size,angle_degree,unicode[i] );

	// 显示官网
	unicode_size = utf_8_to_unicode_string(str2,unicode);
	for(i = 0 ; i<unicode_size;i++)
		freetype_set_char(face,0+font_size/6*i,(buf.height-font_size)/2 +font_size ,font_size/3,angle_degree,unicode[i] );

	getchar();
	drm_exit();	

	return 0;
}