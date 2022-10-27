#include <stdio.h>
#include <sys/types.h>		//open需要的头文件
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>	        //write
#include <sys/types.h>
#include <sys/mman.h>		//mmap  内存映射相关函数库
#include <stdlib.h>	        //malloc free 动态内存申请和释放函数头文件
#include <string.h> 	
#include <linux/fb.h>
#include <sys/ioctl.h>

//32位的颜色
#define Black 	0x00000000
#define White 	0xffFFFFFF
#define Red 	0xffFF0000
#define Green 	0xff00ff00
#define Blue 	0xff0000ff

int fd;
unsigned int *fb_mem  = NULL;	//设置显存的位数为32位
struct fb_var_screeninfo var;
struct fb_fix_screeninfo fix;

int main(void)
{
	unsigned int i;
	int ret;

	/*--------------第一步--------------*/
	fd = open("/dev/fb0",O_RDWR);			//打开framebuffer设备
	if(fd == -1){
		perror("Open LCD");
		return -1;
	}
	/*--------------第二步--------------*/
    fb_mem = (unsigned int *)mmap(NULL, 720*1280*4, 		//获取显存，映射内存
			PROT_READ |  PROT_WRITE, MAP_SHARED, fd, 0);   
								  
	if(fb_mem == MAP_FAILED){
		perror("Mmap LCD");
		return -1;	
	}
	/*--------------第三步--------------*/
	//获取屏幕的可变参数
	ioctl(fd, FBIOGET_VSCREENINFO, &var);
	//获取屏幕的固定参数
	ioctl(fd, FBIOGET_FSCREENINFO, &fix);
	//设置屏幕，需要开启才能使用屏幕
	ioctl(fd, FBIOPAN_DISPLAY, &var);

	//打印分辨率
	printf("xres= %d,yres= %d \n",var.xres,var.yres);
    //打印总字节数和每行的长度
	printf("line_length=%d,smem_len= %d \n",fix.line_length,fix.smem_len);
	printf("xpanstep=%d,ypanstep= %d \n",fix.xpanstep,fix.ypanstep);
			
	memset(fb_mem,0xff,720*1280*4);		//清屏
	sleep(1);
	/*--------------第四步--------------*/
	//将屏幕全部设置成红色
	for(i=0;i< var.xres*var.yres ;i++)
		fb_mem[i] = Blue;

	munmap(fb_mem,720*1280*4); //映射后的地址，通过mmap返回的值	
	close(fd); 			//关闭fb0设备文件
	return 0;			
}
