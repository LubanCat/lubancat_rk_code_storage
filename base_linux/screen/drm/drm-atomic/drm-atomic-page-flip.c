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
	uint32_t count_plane;
	struct drm_mode_create_dumb create ;	//创建的dumb
 	struct drm_mode_map_dumb map;			//内存映射结构体
};

struct property_crtc {
	uint32_t blob_id;
	uint32_t property_crtc_id;
	uint32_t property_mode_id;
	uint32_t property_active;
};



struct property_planes {
	uint32_t plane_id;
	uint32_t property_fb_id;
	uint32_t property_crtc_x;
	uint32_t property_crtc_y;
	uint32_t property_crtc_w;
	uint32_t property_crtc_h;
	uint32_t property_src_x;
	uint32_t property_src_y;
	uint32_t property_src_w;
	uint32_t property_src_h;
};

struct planes_setting {
	uint32_t crtc_id; 
	uint32_t plane_id;
	uint32_t fb_id;
	uint32_t crtc_x;
	uint32_t crtc_y;
	uint32_t crtc_w;
	uint32_t crtc_h;
	uint32_t src_x;
	uint32_t src_y;
	uint32_t src_w;
	uint32_t src_h;  
};


drmModeConnector *conn;	//connetor相关的结构体
drmModeRes *res;		//资源
drmModePlaneRes *plane_res;

drmEventContext ev;
int count;

int fd;					//文件描述符
uint32_t conn_id;
uint32_t crtc_id;
uint32_t plane_id[3];
int printf_status = 1;

#define RED 0XFF0000
#define GREEN 0X00FF00
#define BLUE 0X0000FF


uint32_t color_table[6] = {RED,GREEN,BLUE,RED,GREEN,BLUE};

struct drm_device buf;
struct property_crtc pc;
struct property_planes pp[3];
struct planes_setting ps5;

static int drm_create_fb(struct drm_device *bo)
{
	/* create a dumb-buffer, the pixel format is XRGB888 */
	bo->create.width = bo->width;
	bo->create.height = bo->height*2;
	bo->create.bpp = 32;

	/* handle, pitch, size will be returned */
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &bo->create);

	/* bind the dumb-buffer to an FB object */
	bo->pitch = bo->create.pitch;
	bo->size = bo->create.size;
	bo->handle = bo->create.handle;
	drmModeAddFB(fd, bo->width, bo->height*2, 24, 32, bo->pitch,
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

static uint32_t get_property(int fd, drmModeObjectProperties *props)
{
	drmModePropertyPtr property;
	uint32_t i, id = 0;

	for (i = 0; i < props->count_props; i++) {
		property = drmModeGetProperty(fd, props->props[i]);
		printf("\"%s\"\t\t---",property->name);
		printf("id = %d , value=%ld\n",props->props[i],props->prop_values[i]);
	}
    return 0;
}

static uint32_t get_property_id(int fd, drmModeObjectProperties *props,
				const char *name)
{
	drmModePropertyPtr property;
	uint32_t i, id = 0;


	/* find property according to the name */
	for (i = 0; i < props->count_props; i++) {
		property = drmModeGetProperty(fd, props->props[i]);
		if (!strcmp(property->name, name))
			id = property->prop_id;
		drmModeFreeProperty(property);

		if (id)
			break;
	}

	return id;
}

static uint32_t drm_get_plane_property_id(int fd,uint32_t plane_id)
{
	drmModeObjectProperties *props;
	int i,num;
	for(i=0;i<buf.count_plane+1;i++){
		if(pp[i].plane_id == plane_id){
			num = i;
			break;
		}
		else{
			num = buf.count_plane;
			buf.count_plane++;
			break;
		}
	}
	pp[num].plane_id = plane_id;
	/* get plane properties */
	props = drmModeObjectGetProperties(fd, plane_id, DRM_MODE_OBJECT_PLANE);
	if(printf_status == 1){
		get_property(fd, props);
		printf_status = 0;
	}
	pp[num].property_fb_id  = get_property_id(fd, props, "FB_ID");
	pp[num].property_crtc_x = get_property_id(fd, props, "CRTC_X");
	pp[num].property_crtc_y = get_property_id(fd, props, "CRTC_Y");
	pp[num].property_crtc_w = get_property_id(fd, props, "CRTC_W");
	pp[num].property_crtc_h = get_property_id(fd, props, "CRTC_H");
	pp[num].property_src_x  = get_property_id(fd, props, "SRC_X");
	pp[num].property_src_y  = get_property_id(fd, props, "SRC_Y");
	pp[num].property_src_w  = get_property_id(fd, props, "SRC_W");
	pp[num].property_src_h  = get_property_id(fd, props, "SRC_H");
	drmModeFreeObjectProperties(props);
	buf.count_plane ++;
}

static uint32_t drm_set_plane(int fd,struct planes_setting *ps)
{
	drmModeAtomicReq *req;
	int i;
	int num;
	uint32_t plane = ps->plane_id;
	for(i=0;i<buf.count_plane+1;i++){
		if(pp[i].plane_id == ps->plane_id){
			num = i;
			break;
		}
	}

	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, ps->plane_id, pc.property_crtc_id, crtc_id);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_fb_id,   ps->fb_id);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_crtc_x,  ps->crtc_x);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_crtc_y,  ps->crtc_y);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_crtc_w,  ps->crtc_w);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_crtc_h,  ps->crtc_h);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_src_x,   ps->src_x << 16);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_src_y,   ps->src_y << 16);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_src_w,   ps->src_w << 16);
	drmModeAtomicAddProperty(req, ps->plane_id, pp[num].property_src_h,   ps->src_h << 16);
	drmModeAtomicCommit(fd, req, 0, NULL);
	drmModeAtomicFree(req);
}


static uint32_t drm_set_crtc(void)
{
	drmModeAtomicReq *req;
	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, crtc_id, pc.property_active, 1);
	drmModeAtomicAddProperty(req, crtc_id, pc.property_mode_id, pc.blob_id);
	drmModeAtomicAddProperty(req, conn_id, pc.property_crtc_id, crtc_id);
	drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET|DRM_MODE_PAGE_FLIP_EVENT, NULL);
	drmModeAtomicFree(req);
	return 0;
}


int drm_init()
{
	int i;

	drmModeObjectProperties *props;
	drmModeAtomicReq *req;

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[0];
	conn_id = res->connectors[0];

	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	plane_res = drmModeGetPlaneResources(fd);
	for(i=0;i<3;i++){
		plane_id[i] = plane_res->planes[i];
		printf("planes[%d]= %d\n",i,plane_id[i]);
	}

	conn = drmModeGetConnector(fd, conn_id);
	buf.width = conn->modes[0].hdisplay;
	buf.height = conn->modes[0].vdisplay;
	drm_create_fb(&buf);

	drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);

	/* get connector properties */
	props = drmModeObjectGetProperties(fd, conn_id,	DRM_MODE_OBJECT_CONNECTOR);
	// printf("/-----conn_Property-----/\n");
	// get_property(fd, props);
	// printf("\n");
	pc.property_crtc_id = get_property_id(fd, props, "CRTC_ID");
	drmModeFreeObjectProperties(props);

	/* get crtc properties */
	props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
	// printf("/-----CRTC_Property-----/\n");
	// get_property(fd, props);
	// printf("\n");
	pc.property_active = get_property_id(fd, props, "ACTIVE");
	pc.property_mode_id = get_property_id(fd, props, "MODE_ID");
	drmModeFreeObjectProperties(props);

	/* create blob to store current mode, and retun the blob id */
	drmModeCreatePropertyBlob(fd, &conn->modes[0],
				sizeof(conn->modes[0]), &pc.blob_id);

	/* start modeseting */
	drm_set_crtc();
}
int drm_exit()
{
	drm_destroy_fb(&buf);
	drmModeFreeConnector(conn);
	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);
	close(fd);
}


static void drm_page_flip_handler(int fd, uint32_t frame,
				    uint32_t sec, uint32_t usec,
				    void *data)
{
	drmModeAtomicReq *req;
	//切换显示
    if(count==0)
		count=1;
	else if(count==1)
		count=0;
    //设置偏移量
	if(count==1)
		ps5.src_y = buf.height;
	else 
		ps5.src_y = 0;
	drm_set_plane(fd,&ps5);
	drm_set_crtc();
}


int main(int argc, char **argv)
{
	int i,j;
	drmModeAtomicReq *req;
	ev.version = DRM_EVENT_CONTEXT_VERSION;
	ev.page_flip_handler = drm_page_flip_handler;

	drm_init();
	drm_get_plane_property_id(fd,plane_id[0]);
	drm_get_plane_property_id(fd,plane_id[1]);

	for(i =0;i< buf.width*buf.height;i++)
		buf.vaddr[i] = RED;

	for(i =buf.width*buf.height;i< buf.width*buf.height*2;i++)
		buf.vaddr[i] = BLUE;

	ps5.plane_id = plane_id[0];
	ps5.fb_id = buf.fb_id;
	ps5.crtc_x = 0;
	ps5.crtc_y = 0;
	ps5.crtc_w = buf.width;
	ps5.crtc_h = buf.height;
	ps5.src_x = 0;
	ps5.src_y = 0;
	ps5.src_w = buf.width;
	ps5.src_h = buf.height;
	drm_set_plane(fd,&ps5);

	getchar();
	printf("switch\n");
	drmHandleEvent(fd, &ev);
	getchar();
	printf("switch\n");
	drmHandleEvent(fd, &ev);
	getchar();
	printf("exit\n");
	drm_exit();	

	return 0;
}
