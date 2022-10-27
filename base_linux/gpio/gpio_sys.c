#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define GPIO_INDEX "42"
static char gpio_path[75];
int gpio_init(char *name)
{
    int fd;
    //index config

    sprintf(gpio_path, "/sys/class/gpio/gpio%s", name);

    if (access("gpio_path", F_OK)){
        fd = open("/sys/class/gpio/export", O_WRONLY);
        if(fd < 0)
            return 1 ;
    
        write(fd, name, strlen(name));
        close(fd);
    
        //direction config
        sprintf(gpio_path, "/sys/class/gpio/gpio%s/direction", name);
        fd = open(gpio_path, O_WRONLY);
        if(fd < 0)
            return 2;
    
        write(fd, "out", strlen("out"));
        close(fd);
    }

    return 0;
}

int gpio_deinit(char *name)
{
    int fd;
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if(fd < 0)
        return 1;

    write(fd, name, strlen(name));
    close(fd);

    return 0;
}


int gpio_high(char *name)
{
    int fd;
    sprintf(gpio_path, "/sys/class/gpio/gpio%s/value", name);
    fd = open(gpio_path, O_WRONLY);
    if(fd < 0){
        printf("open %s wrong\n",gpio_path);
        return -1;
    }
        
    if(2 != write(fd, "0", sizeof("0")))
        printf("wrong set \n");
    close(fd);
    return 0;
}


int gpio_low(char *name)
{
    int fd;
    sprintf(gpio_path, "/sys/class/gpio/gpio%s/value", name);
    fd = open(gpio_path, O_WRONLY);
    if(fd < 0){
        printf("open %s wrong\n",gpio_path);
        return -1;
    }
        
    if(2 != write(fd, "1", sizeof("1")))
        printf("wrong set \n");
    close(fd);
    return 0;
}

int main(int argc, char *argv[])
{
    char buf[10];
    int res;

    /* 校验传参 */
    if (2 != argc) {
        printf( "usage: %s <id> <PinNum>\n",argv[0]);
        return -1;
    }
    res = gpio_init(argv[1]);
    if(res){
        printf("gpio init error,code = %d",res);
        return 0;
    }

    while(1){
        printf("Please input the value : 0--low 1--high q--exit\n");
        scanf("%10s", buf);
 
        switch (buf[0]){
            case '0':
                gpio_low(argv[1]);
                break;
 
            case '1':
                gpio_high(argv[1]);
                break;
 
            case 'q':
                gpio_deinit(argv[1]);
                printf("Exit\n");
                return 0;
 
            default:
                break;
       }
    }
    return 0;
}
