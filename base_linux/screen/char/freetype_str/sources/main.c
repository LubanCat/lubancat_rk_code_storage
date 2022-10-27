#include "drm-core.h"
#include <ft2build.h>
#include <wchar.h>
#include <math.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


struct file_string {
	int fd;
	uint8_t *mem;
	uint8_t utf_8[100];
	uint16_t unicode[75];
	uint32_t len;
	uint64_t file_size;
	struct stat stat;
};

struct file_string f_name;
struct file_string f_str[2];

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
	uint8_t unicode[2];

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

void draw_bitmap( FT_Bitmap* bitmap,FT_Int x_pen,FT_Int y_pen)
{
	FT_Int x, y;
	FT_Int x_count, y_count;
	unsigned char show;
	uint32_t buffer_size = bitmap->width * bitmap->rows;
	uint8_t buffer[buffer_size];
	uint32_t color;
	FT_Int  x_max = x_pen + bitmap->width;
	FT_Int  y_max = y_pen + bitmap->rows;

	memcpy(buffer,bitmap->buffer,buffer_size);

	for ( y = y_pen, y_count = 0; y < y_max; y++, y_count++ ){
		for ( x = x_pen, x_count = 0; x < x_max; x++, x_count++ ){
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

int compute_string_bbox(FT_Face face,uint16_t *str,int len, FT_BBox  *abbox)
{
    int i;
    int error;
    FT_BBox bbox;
    FT_BBox glyph_bbox;
    FT_Vector pen;
    FT_Glyph  glyph;
    FT_GlyphSlot slot = face->glyph;

    /* 初始化 */
    bbox.xMin = bbox.yMin = 32000;
    bbox.xMax = bbox.yMax = -32000;

    //指定原点为(0, 0)
    pen.x = 0;
    pen.y = 0;

    /* 计算每个字符的bounding box */
    /* 先translate, 再load char, 就可以得到它的外框了 */
    for (i = 0; i < len; i++){

        /* 转换：transformation */
        FT_Set_Transform(face, 0, &pen);
        /* 加载位图: load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(face, str[i], FT_LOAD_RENDER);
        if (error){
            printf("FT_Load_Char error\n");
            return -1;
        }

        /* 取出glyph */
        error = FT_Get_Glyph(face->glyph, &glyph);
        if (error){
            printf("FT_Get_Glyph error!\n");
            return -1;
        }
        
        /* 从glyph得到外框: bbox */
        FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_TRUNCATE, &glyph_bbox);

        /* 更新外框 */
        if ( glyph_bbox.xMin < bbox.xMin )
            bbox.xMin = glyph_bbox.xMin;

        if ( glyph_bbox.yMin < bbox.yMin )
            bbox.yMin = glyph_bbox.yMin;

        if ( glyph_bbox.xMax > bbox.xMax )
            bbox.xMax = glyph_bbox.xMax;

        if ( glyph_bbox.yMax > bbox.yMax )
            bbox.yMax = glyph_bbox.yMax;
    }
	
    /* return string bbox */
    *abbox = bbox;
}




int display_string(FT_Face face, uint16_t *str,int len, int lcd_x, int lcd_y,FT_BBox *bbox)
{
    int i;
    int error;
    FT_Vector pen;
    FT_Glyph  glyph;
    FT_GlyphSlot slot = face->glyph;
	FT_Matrix	  matrix;
	
	int angle_degree = 0;
	double angle;

    /* 把LCD坐标转换为笛卡尔坐标 */
    int x = lcd_x;
    int y = buf.height - lcd_y;
	//设置原点
    pen.x = (x - bbox->xMin) * 64; /* 单位: 1/64像素 */
    pen.y = (y - bbox->yMax) * 64; /* 单位: 1/64像素 */

    /* 处理每个字符 */
    for (i = 0; i < len; i++){
        //设置旋转
		angle = (1.0 * angle_degree/360) * 3.14159 * 2;
		/* set up matrix */
		matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
		matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
		matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
		matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L );
		
		/* 转换：transformation */
        FT_Set_Transform(face, &matrix, &pen);

        /* 加载位图: load glyph image into the slot (erase previous one) */
        error = FT_Load_Char(face, str[i], FT_LOAD_DEFAULT|FT_LOAD_RENDER);

        /* 显示内容 */
        draw_bitmap(&slot->bitmap,slot->bitmap_left,
                        buf.height - slot->bitmap_top);
		
        /* 计算下一个字符的原点: increment pen position */
        pen.x += slot->advance.x;
		printf("%ld\n",pen.x/64);
		// 竖向字体使用
		// pen.y += slot->advance.y;
    }

    return 0;
}


int main(int argc, char **argv)
{
	FT_Library    library;
    FT_Face       face;
	FT_BBox bbox;
    int i,j,error;
	//字体大小
    int font_size = 180;

	uint8_t str[] = "野火科技";
	uint8_t str1[] = "www.embedfire.com";
	uint16_t unicode[10];
	uint16_t unicode1[10];
	uint32_t u_len;
	uint32_t u1_len;
	int ret;
	//清空内容
	memset(f_name.unicode,0,sizeof(f_name.unicode));
	memset(f_str[0].unicode,0,sizeof(f_str[0].unicode));
	memset(f_str[1].unicode,0,sizeof(f_str[1].unicode));


	//初始化
	ret = drm_init();
	if(ret < 0){
		printf("drm init fail\n");
		return -1;
	}
	//打开文件，在linux中文件以utf-8编码的形式存在
	f_name.fd = open("file/name.txt", O_RDONLY);
	//获取文件大小
	fstat(f_name.fd, &f_name.stat);
	//将文件中的内容全部映射出来
	f_name.mem = (unsigned char *)mmap(NULL , f_name.stat.st_size, PROT_READ, MAP_SHARED, f_name.fd, 0);
	//utf_8转unicode
	f_name.len=utf_8_to_unicode_string(f_name.mem,f_name.unicode);
	u_len = utf_8_to_unicode_string(str,unicode);
	u1_len = utf_8_to_unicode_string(str1,unicode1);

	//初始化freetype
	error = FT_Init_FreeType( &library );
	//读取文字文件，创建face
	error = FT_New_Face( library, "file/simsun.ttc", 0, &face ); 

	//显示野火科技
	FT_Set_Pixel_Sizes(face, 80, 0);
	compute_string_bbox(face, unicode,u_len,&bbox );
	display_string(face, unicode,u_len, (buf.width - u_len * 80)/2, buf.height - 280,&bbox);

	//显示官网
	FT_Set_Pixel_Sizes(face, font_size/3, 0);
	compute_string_bbox(face,unicode1,u1_len,&bbox);
	display_string(face, unicode1,u1_len ,(buf.width - u1_len * font_size/6)/2, buf.height - 180,&bbox);
	
	//显示欢迎来到野火科技		
	FT_Set_Pixel_Sizes(face, 60, 0);
	compute_string_bbox(face, f_name.unicode,f_name.len,&bbox);
	display_string(face, f_name.unicode, f_name.len,(buf.width - f_name.len * 60)/2, buf.height/2-200,&bbox);

	getchar();
	drm_exit();	

	return 0;
}