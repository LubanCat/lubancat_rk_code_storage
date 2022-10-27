#include "drm-core.h"
#include <jpeglib.h>
#include <jerror.h>

#define PNG_FILE 	0X03
#define JPEG_FILE 	0X02
#define BMP_FILE 	0X01 

uint32_t color_table[6] = {RED,GREEN,BLUE,BLACK,WHITE,BLACK_BLUE};


struct jpeg_file{
	char filename[50];
	FILE* fp;			//文件符
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	int width;			//图像宽度
	int height;			//图像高度
	int size;			//图像大小
	int row_size;		//每行占用的字节大小
	int Bpp;			//Bytes per pixel
	uint8_t *buffer;	//解压后的图像buffer
};

void free_jpeg(struct jpeg_file *jf)
{
	free(jf->buffer);
}

//判断文件是否是jpeg文件
int judge_jpeg(char *filename,struct jpeg_file *pfd)
{
	int ret;
	uint32_t word;

    pfd->cinfo.err = jpeg_std_error(&pfd->jerr);
	//创建一个jpeg_compress_struct结构体
    jpeg_create_decompress(&pfd->cinfo);
	
	//指定jpeg解压的源文件
    jpeg_stdio_src(&pfd->cinfo, pfd->fp);
	
	//解析jpeg文件，解析完成后可获得图像的格式
    ret = jpeg_read_header(&pfd->cinfo, TRUE);
	if(ret < 0){
		printf("file is not jpg ...\n");
		return -1;
	}

	return JPEG_FILE;
}

int decode_jpeg(char *filename,struct jpeg_file *jf)
{
	int ret;
	uint32_t word;
	unsigned char *buf_cpy;
	//临时变量行buffer
	uint8_t *pucbuffer;
	//放大倍率
	jf->cinfo.scale_num = 2;
	//缩小倍率
	jf->cinfo.scale_denom = 1;

	//对cinfo所指定的源文件进行解压，并将解压后的数据存到cinfo结构体的成员变量中。
    jpeg_start_decompress(&jf->cinfo);
	//获取图片基本信息
    jf->row_size = jf->cinfo.output_width * jf->cinfo.output_components;
    jf->width = jf->cinfo.output_width;
    jf->height = jf->cinfo.output_height;
	jf->Bpp = jf->cinfo.output_components;
    jf->size = jf->row_size * jf->cinfo.output_height;
	//分配内存空间 
	pucbuffer = malloc(jf->row_size);
    jf->buffer = malloc(jf->size);

    printf("size: %d w: %d h: %d row_size: %d ,Bpp: %d\n",
			jf->size,jf->width,jf->height,jf->row_size,jf->Bpp);
	//缓冲指针指向buffer		
	buf_cpy = jf->buffer;

    while (jf->cinfo.output_scanline < jf->cinfo.output_height){
        //可以读取RGB数据到buffer中，参数3能指定读取多少行
		jpeg_read_scanlines(&jf->cinfo, &pucbuffer, 1);
        //复制到内存
		memcpy(buf_cpy , pucbuffer, jf->row_size);
		buf_cpy = buf_cpy + jf->row_size;
    }

	// 完成解码
	jpeg_finish_decompress(&jf->cinfo);
	free(pucbuffer);
	//释放结构体
    jpeg_destroy_decompress(&jf->cinfo);

	return 0;
}


int show_jpeg(struct jpeg_file *jf,int x ,int y)
{

	int i,j;
	uint32_t word;

	for(j=0; j<jf->height; j++){
		for(i = 0 ; i<jf->width;i++){
			if((j+y) < buf.height && (i+x)<buf.width){
				word = 0;
				word = (word | jf->buffer[(j*jf->width+i)*jf->Bpp+2]) | 
					   (word | jf->buffer[(j*jf->width+i)*jf->Bpp+1])<<8 | 
					   (word | jf->buffer[(j*jf->width+i)*jf->Bpp])<<16;
				buf.vaddr[(j+y)*buf.width+(x+i)] = word;
			}
			else
				continue;
		}
	}
}

int main(int argc, char **argv)
{
	int i,j;
	int x =0, y=0 ;
	uint32_t word;
	int ret;
	struct jpeg_file jf;

	if(argc <2 ){
		printf("Wrong use !!!!\n");
		printf("Usage: %s [xxx.jpg / xxx.jpeg]\n",argv[0]);
		goto fail1;
	}

	ret = drm_init();	
	if(ret < 0 ){
		printf("drm_init fail\n");
		return -1;
	}

	jf.fp= fopen(argv[1], "rb");
	if (jf.fp== NULL) {
		printf("can not open file");
		return -1;
	}

	memcpy(jf.filename,argv[1],sizeof(argv[1]));

	ret =judge_jpeg(argv[1],&jf);
	if(ret < 0 ){
		goto fail2;
	}

	ret = decode_jpeg(argv[1],&jf);
	if(ret < 0 ){
		printf("decode_jpeg wrong\n");
		goto fail2;
	}

	show_jpeg(&jf , buf.width/2 - jf.width/2, buf.height/2 - jf.height/2);

	getchar();
	
    fclose(jf.fp);
	free_jpeg(&jf);
	drm_exit();	
	return 0;

fail2:
	fclose(jf.fp);
	free_jpeg(&jf);
	drm_exit();
fail1:	
	printf("Proglem run fail,please check !\n");
	return -1;
}