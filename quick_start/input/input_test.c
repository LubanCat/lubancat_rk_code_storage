#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <unistd.h>
#define  default_path "/dev/input/event2"

int  main(int argc,char * argv[])
{
    int fd;
    int ret;
    char * path;
    struct input_event event;
    int x,y;
    if (argc == 2)
        path = argv[1];

    else
        path = default_path;

    //打开设备
    fd = open(path,O_RDONLY);
    if(fd < 0){
        perror(path);
        exit(-1);
    }

    while (1){
        memset(&event, 0, sizeof(struct input_event));
        //读取按键值
        ret = read(fd,&event,sizeof(struct input_event));
        if (ret == sizeof(struct input_event)){
            //触发事件类似判断
            if(event.type == EV_ABS){
                //X、Y轴坐标
                if(event.code == ABS_X)
                    x = event.value;
                else if(event.code == ABS_Y)
                    y = event.value;
            }
            //打印坐标
            if(event.type == EV_SYN)    //EV_SYN
                printf("touch x = %d,y = %d\n", x, y);
        }
    }
    close(fd);
    return 0;
}