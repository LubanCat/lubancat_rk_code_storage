#include "drm-core.h"

uint32_t color_table[6] = {RED,GREEN,BLUE,BLACK,WHITE,BLACK_BLUE};

struct bmpfile {
	FILE* fp;
	int fd;
	int file_size;				//文件中图像的大小-4Bytes
	int bmp_offset;				//图像偏移量-4Bytes
	int biSize;					//位图信息头大小-4Bytes
	int biWidth;				//图像宽度-4Bytes
	int biHeight;				//图像高度-4Bytes
	short biPlanes;				//颜色平面书-2Bytes
	short biBitCount;			//每像素占用位数-2Bytes
	int biCompression;			//数据压缩类型-4Bytes
	int biSizeImage;			//图像数据大小-4Bytes
	int biXPelsPerMeter;		//像素/米-4Bytes
	int biYPelsPerMeter;		//像素/米-4Bytes
	int biClrUsed;				//调色板索引数-4Bytes
	int biClrImportant;			//索引数目-4Bytes
	unsigned char *mem_buf;		//内存映射，整个文件
	unsigned char *bmp_buf;		//将bmp文件的格式改成图像rgb格式
	unsigned char Bpp;			//每个像素占用的字节数
	int row_size;
};


void show_bmp_info(struct bmpfile *bf)
{
	//打印文件的内容
	printf("file_size = %d\n",bf->file_size);
	printf("bmp_offset = %d\n",bf->bmp_offset);
	printf("biSize = %d\n",bf->biSize);
	printf("biWidth = %d\n",bf->biWidth);
	printf("biHeight = %d\n",bf->biHeight);
	printf("biPlanes = %d\n",bf->biPlanes);
	printf("biBitCount = %d\n",bf->biBitCount);
	printf("biCompression = %d\n",bf->biCompression);
	printf("biSizeImage = %d\n",bf->biSizeImage);
	printf("biXPelsPerMeter = %d\n",bf->biXPelsPerMeter);
	printf("biYPelsPerMeter = %d\n",bf->biYPelsPerMeter);
	printf("biClrUsed = %d\n",bf->biClrUsed);
	printf("biClrImportant = %d\n",bf->biClrImportant);
}


int judge_bmp(char *filename,struct bmpfile *pfd)
{
	int offset = 0;
	int count;
	char file_head[14];
	//打开文件
	pfd->fp = fopen(filename,"rb");
	if(pfd->fp == NULL){
		printf("open file fail\n");
		return -1;
	}
	//获取前14个字节的内容
	count = fread(file_head,1,14,pfd->fp);
	if(count != 14){
		printf("read file fail\n");
		return -1;
	}
	//判断是否为BMP文件
	if(file_head[0] == 0x42 &&  file_head[1] == 0x4d){
		//解析参数
		offset = 2;
		memcpy(&pfd->file_size,file_head+offset,4);
		offset += 8;		//中间有四个字节的空白区域
		memcpy(&pfd->bmp_offset,file_head+offset,4);
		fclose(pfd->fp);
		return 1;
	}
	else{
		fclose(pfd->fp);
		return -1;
	}
		
}

int get_bmp_file(char *filename,struct bmpfile *bf)
{
	
	int i;
	int word = 0;
	char bmp_head[14];
	int size;
	unsigned char *buf_cpy;
	unsigned char *mem_cpy;
	int offset = 14;

	bf->fd = open(filename,O_RDWR);
	if(bf->fd < 0){
		printf("can not openfile\n");
		return -1;
	}

	bf->mem_buf = (unsigned char *)mmap(NULL , bf->file_size, 
						PROT_READ | PROT_WRITE, MAP_SHARED, 
						bf->fd, 0);
	if(bf->mem_buf == NULL){
		printf("mmap wrong !\n");
		close(bf->fd);
		return -1;
	}

	memcpy(&bf->biSize,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biWidth,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biHeight,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biPlanes,bf->mem_buf+offset,2);
	offset += 2;
	memcpy(&bf->biBitCount,bf->mem_buf+offset,2);
	offset += 2;
	memcpy(&bf->biCompression,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biSizeImage,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biXPelsPerMeter,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biYPelsPerMeter,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biClrUsed,bf->mem_buf+offset,4);
	offset += 4;
	memcpy(&bf->biClrImportant,bf->mem_buf+offset,4);

	if(bf->biSizeImage == 0)
		bf->biSizeImage = bf->file_size - bf->bmp_offset;
	
	if(bf->biBitCount == 24 || bf->biBitCount == 32 )
		bf->Bpp = bf->biBitCount/8;

	//分配空间，操作指针需要分配空间
	bf->bmp_buf = malloc(bf->biSizeImage);
	bf->row_size = bf->biWidth * bf->Bpp;
	mem_cpy = bf->mem_buf + bf->bmp_offset + (bf->biHeight-1)*bf->row_size;
	buf_cpy = bf->bmp_buf;
	//将bmp文件的格式改为RGB适用的格式
	for(i = 0;i<bf->biHeight;i++){
		memcpy(buf_cpy,mem_cpy,bf->row_size);
		buf_cpy += bf->row_size;
		mem_cpy -= bf->row_size;
	}
		
	close(bf->fd);
	return 0;
}


int free_bmp_file(struct bmpfile *bf)
{
	munmap(bf->mem_buf, bf->file_size);
	free(bf->bmp_buf);
}

//显示图片，根据X,Y坐标
int show_bmp(struct bmpfile *bf,int x ,int y)
{
	int i,j;
	uint32_t word;
	if(x<0 || y<0){
		printf("wrong set !\n");
		return -1;
	}
		
	if(bf->biBitCount == 24 || bf->biBitCount == 32){
		for(j=0; j<bf->biHeight; j++){
			for(i = 0 ; i<bf->biWidth;i++){
				if((j+y) < buf.height && (i+x)<buf.width){
					word = 0;
					word = ((word | bf->bmp_buf[(j*bf->biWidth+i)*bf->Bpp+2])<<16) | 
						   ((word | bf->bmp_buf[(j*bf->biWidth+i)*bf->Bpp+1])<<8) | 
						   ((word | bf->bmp_buf[(j*bf->biWidth+i)*bf->Bpp]));
					buf.vaddr[(j+y)*buf.width+(x+i)] = word;
				}
				else
					continue;
			}
		}
	}
	else
		printf("unsupport Bpps!Please use 24Bpps or 32Bpps bmp file\n");
	return 0;	
}


int main(int argc, char **argv)
{
	int i,j;
	int fd_bmp;
	int ret;
	struct bmpfile cbf;
	//获取BMP文件
	if(argc <2 ){
		printf("Wrong use !!!!\n");
		printf("Usage: %s xxx.bmp\n",argv[0]);
		goto fail1;
	}
	//初始化drm
	ret = drm_init();
	if(ret < 0){
		printf("drm init fail\n");
		return -1;
	}
	//判断是否为bmp文件
	ret = judge_bmp(argv[1],&cbf);
	if(ret < 0){
		printf("something wrong in bmp file %d\n",fd_bmp);
		goto fail1;
	}
	//获取BMP文件的各种信息
	ret = get_bmp_file(argv[1],&cbf);
	if(ret < 0){
		printf("something wrong in bmp file %d\n",fd_bmp);
		goto fail1;
	}
	//打印信息
	show_bmp_info(&cbf);
	//显示画面信息在正中央
	ret = show_bmp(&cbf , buf.width/2 - cbf.biWidth/2, buf.height/2 - cbf.biHeight/2);
	if(ret < 0){
		printf("show_bmp wrong!\n");
		goto fail2;
	}
	//获取按键，按键后退出程序
	getchar();
	//注销drm
	drm_exit();
	//释放内存空间
	free_bmp_file(&cbf);	
	return 0;

fail2: 
	drm_exit();
	free_bmp_file(&cbf);
fail1:	
	printf("Proglem run fail,please check !\n");
	return -1;
}