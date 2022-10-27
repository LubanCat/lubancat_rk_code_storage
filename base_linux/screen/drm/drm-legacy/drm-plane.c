#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct drm_device {
	uint32_t width;			//显示器的宽的像素点数量
	uint32_t height;		//显示器的高的像素点数量
	uint32_t pitch;			//每行占据的字节数
	uint32_t handle;		//drm_mode_create_dumb的返回句柄
	uint32_t size;			//显示器占据的总字节数
	uint32_t *vaddr;		//mmap的首地址
	uint32_t fb_id;			//创建的framebuffer的id号
	struct drm_mode_create_dumb create ;	//创建的dumb
 	struct drm_mode_map_dumb map;			//内存映射结构体
};

drmModeConnector *conn;	//connetor相关的结构体
drmModeRes *res;		//资源
drmModePlaneRes *plane_res;//图层资源

int fd;					//文件描述符
uint32_t conn_id;
uint32_t crtc_id;
uint32_t plane_id[3];	//图层id数组


#define RED 0XFF0000
#define GREEN 0X00FF00
#define BLUE 0X0000FF

struct drm_device buf;

uint32_t color_table[6] = {RED,GREEN,BLUE,RED,GREEN,BLUE};


static int drm_create_fb(struct drm_device *bo)
{
	/* create a dumb-buffer, the pixel format is XRGB888 */
	bo->create.width = bo->width;
	bo->create.height = bo->height;
	bo->create.bpp = 32;

	/* handle, pitch, size will be returned */
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &bo->create);

	/* bind the dumb-buffer to an FB object */
	bo->pitch = bo->create.pitch;
	bo->size = bo->create.size;
	bo->handle = bo->create.handle;
	drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
			   bo->handle, &bo->fb_id);
	
	//每行占用字节数，共占用字节数，MAP_DUMB的句柄
	printf("pitch = %d ,size = %d, handle = %d \n",bo->pitch,bo->size,bo->handle);

	/* map the dumb-buffer to userspace */
	bo->map.handle = bo->create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &bo->map);

	bo->vaddr = mmap(0, bo->create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, bo->map.offset);

	/* initialize the dumb-buffer with white-color */
	memset(bo->vaddr, 0xff,bo->size);

	return 0;
}

static void drm_destroy_fb(struct drm_device *bo)
{
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(fd, bo->fb_id);
	
	munmap(bo->vaddr, bo->size);
	
	destroy.handle = bo->handle;

	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

int drm_init()
{
	int i;
	//打开drm设备，设备会随设备树的更改而改变,多个设备时，请留一下每个屏幕设备对应的drm设备
	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if(fd < 0){
        printf("wrong\n");
        return 0;
    }

	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	plane_res = drmModeGetPlaneResources(fd);
	printf("count_planes = %d\n",plane_res-> count_planes);
	for(i=0;i<3;i++){
		plane_id[i] = plane_res->planes[i];
		printf("planes[%d]= %d\n",i,plane_id[i]);
	}
		
	//获取drm的信息
	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[0];
	conn_id = res->connectors[0];
	//打印CRTCS,以及conneter的id
	printf("crtc = %d , conneter = %d\n",crtc_id,conn_id);

	conn = drmModeGetConnector(fd, conn_id);
	buf.width = conn->modes[0].hdisplay;
	buf.height = conn->modes[0].vdisplay;

	//打印屏幕分辨率
	printf("width = %d , height = %d\n",buf.width,buf.height);

	//创建framebuffer层
	drm_create_fb(&buf);

	//设置CRTCS
	drmModeSetCrtc(fd, crtc_id, buf.fb_id,
			0, 0, &conn_id, 1, &conn->modes[0]);

	return 0;
}

int drm_exit()
{
	drm_destroy_fb(&buf);
	drmModeFreeConnector(conn);
	drmModeFreeResources(res);
	close(fd);
}

int main(int argc, char **argv)
{
	int i;
	int j = 0;
	drm_init();

	//显示三色
	for(j=0;j<3;j++){
		for(i =j*buf.width*buf.height/3;i< (j+1)*buf.width*buf.height/3;i++)
			buf.vaddr[i] = color_table[j];
	}

	getchar();
	//将framebuffer上2/3的区域放到图层一上，
	//此时屏幕改变，将的framebuffer区域拉伸到整个屏幕中
	drmModeSetPlane(fd, plane_id[0], crtc_id, buf.fb_id, 0,
			0, 0, buf.width, buf.height,
			0 << 16, 0 << 16, buf.width << 16, buf.height/3*2 << 16);

	getchar();
	//将framebuffer区域缩放一倍放到图层二上，把图层二的位置放到屏幕的下方
	//叠加在图层一上，可以看到图层二覆盖了图层一的部分区域
	drmModeSetPlane(fd, plane_id[1], crtc_id, buf.fb_id, 0,
			buf.width/4, buf.height/2, buf.width/2, buf.height/2,
			0 << 16, 0 << 16, buf.width << 16, buf.height << 16);

	getchar();

	drm_exit();
	
	exit(0);
}
	