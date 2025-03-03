/*
*
*   file: mipi_to_yuv.c
*   date: 2025-02-27
*   usage: 
*       sudo gcc -o mipi_to_yuv mipi_to_yuv.c
*       sudo ./mipi_to_yuv /dev/videoX
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/types.h>          
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/mman.h>
#include <jpeglib.h>
#include <linux/fb.h>

#define FRAME_SIZE_X    640
#define FRAME_SIZE_Y    480

int main(int argc, char **argv)
{
    int fd;
    int ret;
    void *buffers[VIDEO_MAX_FRAME][VIDEO_MAX_PLANES];
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    int buffer_cnt;
    struct pollfd fds[1];
    int i;
    int num_planes;
    char filename[32];
    int file_cnt = 0;

    if (argc != 2)
    {
        printf("Usage: %s </dev/videoX>\n", argv[0]);
        return -1;
    }

    /* 1. open /dev/video* */
    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        printf("can not open %s\n", argv[1]);
        goto _err;
    }

    /* 2. query capability */
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(struct v4l2_capability));

    if (0 == ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {        
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE) == 0) 
        {
            fprintf(stderr, "Error opening device %s: video capture mplane not supported.\n", argv[1]);
            goto _ioc_querycap_err;
        }

        if(!(cap.capabilities & V4L2_CAP_STREAMING)) 
        {
            fprintf(stderr, "%s does not support streaming i/o\n", argv[1]);
            goto _ioc_querycap_err;
        }
    }
    else
    {
        printf("can not get capability\n");
        goto _ioc_querycap_err;
    }

    /* 3. enum formt */
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum fsenum;
    int fmt_index = 0;
    int frame_index = 0;
    while (1)
    {
        fmtdesc.index = fmt_index;  
        fmtdesc.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;  
        if (0 != ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))
            break;
        // printf("format %s:\n", fmtdesc.description);

        frame_index = 0;
        while (1)
        {
            memset(&fsenum, 0, sizeof(struct v4l2_frmsizeenum));
            fsenum.pixel_format = fmtdesc.pixelformat;
            fsenum.index = frame_index;

            if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsenum) == 0)
            {
                // printf("\t%d: %d x %d\n", frame_index, fsenum.discrete.width, fsenum.discrete.height);
            }
            else
            {
                break;
            }

            frame_index++;
        }

        fmt_index++;
    }

    /* 4. set formt */
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(struct v4l2_format));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix.width = FRAME_SIZE_X;
    fmt.fmt.pix.height = FRAME_SIZE_Y;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV21;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (0 == ioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        num_planes = fmt.fmt.pix_mp.num_planes;
        printf("the final frame-size has been set : %d x %d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    }
    else
    {
        printf("can not set format\n");
        goto _ioc_sfmt_err;
    }
    
    /* 5. require buffer */
    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof(struct v4l2_requestbuffers));

    rb.count = 32;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    rb.memory = V4L2_MEMORY_MMAP;

    if (0 == ioctl(fd, VIDIOC_REQBUFS, &rb))
    {
        buffer_cnt = rb.count;

        for(i = 0; i < rb.count; i++) 
        {
            struct v4l2_buffer buf;
            struct v4l2_plane planes[num_planes];
            memset(&buf, 0, sizeof(struct v4l2_buffer));
            memset(&planes, 0, sizeof(planes));

            buf.index = i;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.m.planes = planes;
            buf.length = num_planes;

            if (0 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
            {
                for (int j = 0; j < num_planes; j++)
                {
                    buffers[i][j] = mmap(NULL, buf.m.planes[j].length, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, buf.m.planes[j].m.mem_offset);
                    
                    if (buffers[i][j] == MAP_FAILED) 
                    {
                        printf("Unable to map buffer\n");
                        goto _err;
                    }
                }
            }
            else
            {
                printf("can not query buffer\n");
                goto _err;
            }            
        }
    }
    else
    {
        printf("can not request buffers\n");
        goto _ioc_reqbufs_err;
    }

    /* 6. queue buffer */
    for(i = 0; i < buffer_cnt; ++i) 
    {
        struct v4l2_buffer buf;
        struct v4l2_plane planes[num_planes];
        memset(&buf, 0, sizeof(struct v4l2_buffer));
        memset(&planes, 0, sizeof(planes));

        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = num_planes;

        if (0 != ioctl(fd, VIDIOC_QBUF, &buf))
        {
            perror("Unable to queue buffer");
            goto _ioc_qbuf_err;
        }
    }

    /* start camera */
    if (0 != ioctl(fd, VIDIOC_STREAMON, &type))
    {
        printf("Unable to start capture\n");
        goto _err;
    }
    printf("start camera ...\n");

    while (1)
    {
        /* poll */
        memset(fds, 0, sizeof(fds));
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        if (1 == poll(fds, 1, -1))
        {
            /* dequeue buffer */
            struct v4l2_buffer buf;
            struct v4l2_plane planes[num_planes];
            memset(&buf, 0, sizeof(struct v4l2_buffer));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.m.planes = planes;
            buf.length = num_planes;

            if (0 != ioctl(fd, VIDIOC_DQBUF, &buf))
            {
                printf("Unable to dequeue buffer\n");
                goto _ioc_dqbuf_err;
            }
            
            /* save as file */
            sprintf(filename, "video_raw_data_%04d.yuv", file_cnt++);
            int fd_file = open(filename, O_RDWR | O_CREAT, 0666);
            if (fd_file < 0)
            {
                printf("can not create file : %s\n", filename);
            }
            printf("capture to %s\n", filename);
            for (int i = 0; i < num_planes; i++)
                write(fd_file, buffers[buf.index][i], buf.m.planes[i].bytesused);
            close(fd_file);

            /* queue buffer */
            if (0 != ioctl(fd, VIDIOC_QBUF, &buf))
            {
                printf("Unable to queue buffer");
                goto _ioc_qbuf_err;
            }
        }
    }
    
    /* close camera */
    if (0 != ioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        printf("Unable to stop capture\n");
        goto _ioc_streamoff_err;
    }
    close(fd);

    return 0;

_ioc_qbuf_err:
_ioc_reqbufs_err:
_ioc_sfmt_err:
_ioc_querycap_err:
_ioc_streamoff_err:
_ioc_dqbuf_err:
_err:
    return -1;
}