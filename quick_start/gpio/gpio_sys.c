#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define GPIO_INDEX "42"

int gpio_init(void)
{
    int fd;
    //index config
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(fd < 0)
        return 1 ;
 
    write(fd, GPIO_INDEX, strlen(GPIO_INDEX));
    close(fd);
 
    //direction config
    fd = open("/sys/class/gpio/gpio" GPIO_INDEX "/direction", O_WRONLY);
    if(fd < 0)
        return 2;
 
    write(fd, "out", strlen("out"));
    close(fd);
 
    return 0;
}

int gpio_deinit(void)
{
    int fd;
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if(fd < 0)
        return 1;

    write(fd, GPIO_INDEX, strlen(GPIO_INDEX));
    close(fd);

    return 0;
}


int gpio_high(void)
{
   int fd;

    fd = open("/sys/class/gpio/gpio" GPIO_INDEX "/value", O_WRONLY);
    if(fd < 0)
        return 1;
 
    write(fd, "1", 1);
    close(fd);
 
    return 0;
}

int gpio_low(void)
{
   int fd;

    fd = open("/sys/class/gpio/gpio" GPIO_INDEX "/value", O_WRONLY);
    if(fd < 0)
       return 1;
 
    write(fd, "0", 1);
    close(fd);
 
    return 0;
}

int main(int argc, char *argv[])
{
    char buf[10];
    int res;
    printf("This is the gpio demo\n");
 
    res = gpio_init();
    if(res){
        printf("gpio init error,code = %d",res);
        return 0;
    }
 
    while(1){
        printf("Please input the value : 0--low 1--high q--exit\n");
        scanf("%10s", buf);
 
        switch (buf[0]){
            case '0':
                gpio_low();
                break;
 
            case '1':
                gpio_high();
                break;
 
            case 'q':
                gpio_deinit();
                printf("Exit\n");
                return 0;
 
            default:
                break;
       }
    }
}