#include "drm-core.h"

drmModeConnector *conn;	//connetor相关的结构体
drmModeRes *res;		//资源
drmModePlaneRes *plane_res;

int fd;					//文件描述符
uint32_t conn_id;
uint32_t crtc_id;
uint32_t plane_id[3];

struct drm_device buf;
struct property_crtc pc;
struct property_planes pp[3];

static int drm_create_fb(struct drm_device *bo)
{
	int ret;
	
	/* create a dumb-buffer, the pixel format is XRGB888 */
	bo->create.width = bo->width;
	bo->create.height = bo->height;
	bo->create.bpp = 32;

	/* handle, pitch, size will be returned */
	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &bo->create);
	if(ret<0){
		printf("DRM_IOCTL_MODE_CREATE_DUMB fail\n");
		return ret;
	}
	/* bind the dumb-buffer to an FB object */
	bo->pitch = bo->create.pitch;
	bo->size = bo->create.size;
	bo->handle = bo->create.handle;
	ret = drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
			   bo->handle, &bo->fb_id);
	if(ret<0){
		printf("drmModeAddFB fail\n");
		return ret;
	}

	/* map the dumb-buffer to userspace */
	bo->map.handle = bo->create.handle;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &bo->map);
	if(ret<0){
		printf("DRM_IOCTL_MODE_MAP_DUMB fail\n");
		return ret;
	}

	bo->vaddr = mmap(0, bo->create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, bo->map.offset);
	if(bo->vaddr==NULL){
		printf("mmap FB fail\n");
		drm_destroy_fb(bo);
		return ret;
	}

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

uint32_t drm_get_crtc_property_id(int fd)
{
    drmModeObjectProperties *props;
    /* get connector properties */
	props = drmModeObjectGetProperties(fd, conn_id,	DRM_MODE_OBJECT_CONNECTOR);
	pc.property_crtc_id = get_property_id(fd, props, "CRTC_ID");
	drmModeFreeObjectProperties(props);
	/* get crtc properties */
	props = drmModeObjectGetProperties(fd, crtc_id, DRM_MODE_OBJECT_CRTC);
	pc.property_active = get_property_id(fd, props, "ACTIVE");
	pc.property_mode_id = get_property_id(fd, props, "MODE_ID");
	drmModeFreeObjectProperties(props);
    return 0;
}

int drm_set_crtc(int fd)
{
    int ret;
    drmModeAtomicReq *req;
    /* create blob to store current mode, and retun the blob id */
	drmModeCreatePropertyBlob(fd, &conn->modes[0],
				sizeof(conn->modes[0]), &pc.blob_id);

	/* start modeseting */
	req = drmModeAtomicAlloc();
	drmModeAtomicAddProperty(req, crtc_id, pc.property_active, 1);
	drmModeAtomicAddProperty(req, crtc_id, pc.property_mode_id, pc.blob_id);
	drmModeAtomicAddProperty(req, conn_id, pc.property_crtc_id, crtc_id);
	ret = drmModeAtomicCommit(fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL);
	if(ret<0){
		printf("plane AtomicCommit fail\n");
		return ret;
	}
	drmModeAtomicFree(req);
    return 0;
}

uint32_t drm_get_plane_property_id(int fd,uint32_t plane_id)
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

    return 0;
}

int drm_set_plane(int fd,struct planes_setting *ps)
{
	drmModeAtomicReq *req;
	int ret;
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
	ret = drmModeAtomicCommit(fd, req, 0, NULL);
	if(ret<0){
		printf("plane AtomicCommit fail\n");
		return ret;
	}
	drmModeAtomicFree(req);
    return 0;
}


int drm_init()
{
	int i;
	int ret;
	drmModeObjectProperties *props;
	drmModeAtomicReq *req;
    struct planes_setting ps5;

	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if(fd < 0){
		printf("can't open card0\n");
		ret = fd;
		goto fail;
	}

	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[0];
	conn_id = res->connectors[0];

	drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	plane_res = drmModeGetPlaneResources(fd);
	for(i=0;i<3;i++){
		plane_id[i] = plane_res->planes[i];
	}

	conn = drmModeGetConnector(fd, conn_id);
	buf.width = conn->modes[0].hdisplay;
	buf.height = conn->modes[0].vdisplay;
	ret = drm_create_fb(&buf);
	if(ret < 0){
		printf("drm_create_fb fail\n");
		goto fail1;
	}
	drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);

    drm_get_crtc_property_id(fd);
    ret = drm_set_crtc(fd);
	if(ret < 0){
		printf("drm_set_crtc fail\n");
		goto fail2;
	}

    drm_get_plane_property_id(fd,plane_id[0]);
	drm_get_plane_property_id(fd,plane_id[1]);

	//显示三色
	for(i = 0;i< buf.width*buf.height;i++)
		buf.vaddr[i] = BLACK;

	//1：1设置屏幕，没有该函数不会显示画面
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
	ret = drm_set_plane(fd,&ps5);
	if(ret < 0){
		printf("drm_set_plane fail\n");
		goto fail2;
	}
	return 0;

fail2:
	drm_destroy_fb(&buf);
fail1:
	drmModeFreeConnector(conn);
	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);
	close(fd);
fail:
	return ret;
}

void drm_exit()
{
	drm_destroy_fb(&buf);
	drmModeFreeConnector(conn);
	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);
	close(fd);
}

