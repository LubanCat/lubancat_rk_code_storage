#ifndef _DRM_CORE_H_
#define _DRM_CORE_H_

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

#define RED         0XFF0000
#define GREEN       0X00FF00
#define BLUE        0X0000FF
#define BLACK       0X000000
#define WHITE       0XFFFFFF
#define BLACK_BLUE  0X123456

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


extern drmModeConnector *conn;	//connetor相关的结构体
extern drmModeRes *res;		//资源
extern drmModePlaneRes *plane_res;

extern int fd;					//文件描述符
extern uint32_t conn_id;
extern uint32_t crtc_id;
extern uint32_t plane_id[3];

extern struct drm_device buf;
extern struct property_crtc pc;
extern struct property_planes pp[3];

static int drm_create_fb(struct drm_device *bo);
static void drm_destroy_fb(struct drm_device *bo);
static uint32_t get_property(int fd, drmModeObjectProperties *props);
static uint32_t get_property_id(int fd, drmModeObjectProperties *props,
				const char *name);
uint32_t drm_get_plane_property_id(int fd,uint32_t plane_id);
int drm_set_plane(int fd,struct planes_setting *ps);
uint32_t drm_get_crtc_property_id(int fd);
int drm_set_crtc(int fd);
int drm_init();
void drm_exit();
#endif
