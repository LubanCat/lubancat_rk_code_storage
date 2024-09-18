/*
*
*   file: ec11.c
*   date: 2024-08-29
*   usage: 
*       sudo gcc -o ec11 ec11.c -lpthread
*       sudo ./ec11
*
*/

#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <string.h>  
#include <linux/input.h>  
#include <pthread.h>  
#include <errno.h>  
  
#define EC11_SW_EVENT   "/dev/input/event6"  
#define EC11_A_EVENT    "/dev/input/event5"  
#define EC11_B_EVENT    "/dev/input/event3"  

int ec11_SW_fd, ec11_A_fd, ec11_B_fd;

int ec11_SW_value = 0;  
int ec11_A_value = 1;  
int ec11_B_value = 1;  
int ec11_direction = 0;  
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  
  
void *ec11_scan_device(void *arg) {  
    int fd = *(int*)arg;  
    struct input_event ie;  
  
    while (1) 
    {  
        if (read(fd, &ie, sizeof(struct input_event)) == sizeof(struct input_event)) 
        {  
            if (ie.type == EV_KEY) 
            {  
                pthread_mutex_lock(&lock); 

                if (fd == ec11_SW_fd && ie.code == 4) 
                {  
                    ec11_SW_value = ie.value;  
                    if (ec11_SW_value == 1 && ec11_A_value == 1 && ec11_B_value == 1)   // 按键按下
                        ec11_direction = 5;  
                } 
                else if (fd == ec11_A_fd && ie.code == 250) 
                {  
                    if (ie.value == 0 && ec11_B_value == 1)         // 顺时针旋转
                    {  
                        ec11_A_value = 0;  
                        if (ec11_SW_value == 1)                     // 按键按下
                            ec11_direction = 3;  
                        else  
                            ec11_direction = 1;  
                    } 
                    else if (ie.value == 1) 
                        ec11_A_value = 1;  
                } 
                else if (fd == ec11_B_fd && ie.code == 252) 
                {  
                    if (ie.value == 0 && ec11_A_value == 1)         // 逆时针旋转
                    {  
                        ec11_B_value = 0;  
                        if (ec11_SW_value == 1)                     // 按键按下
                            ec11_direction = 4;  
                        else  
                            ec11_direction = 2;  
                    } 
                    else if (ie.value == 1) 
                        ec11_B_value = 1;  
                }  

                pthread_mutex_unlock(&lock);  
            }  
        }  
    }   
}  
  
int main(int argc, char **argv) 
{  
    pthread_t ec11_SW_thread, ec11_A_thread, ec11_B_thread;  
  
    ec11_SW_fd = open(EC11_SW_EVENT, O_RDONLY);  
    ec11_A_fd = open(EC11_A_EVENT, O_RDONLY);  
    ec11_B_fd = open(EC11_B_EVENT, O_RDONLY);  
  
    if (ec11_SW_fd < 0 || ec11_A_fd < 0 || ec11_B_fd < 0) 
    {  
        perror("Failed to open input device");  
        return EXIT_FAILURE;  
    }  
  
    pthread_create(&ec11_SW_thread, NULL, ec11_scan_device, &ec11_SW_fd);  
    pthread_create(&ec11_A_thread, NULL, ec11_scan_device, &ec11_A_fd);  
    pthread_create(&ec11_B_thread, NULL, ec11_scan_device, &ec11_B_fd);  
  
    while (1) 
    {  
        pthread_mutex_lock(&lock);

        switch (ec11_direction) 
        {  
            case 1: printf("顺时针转\n"); break;  
            case 2: printf("逆时针转\n"); break;  
            case 3: printf("按键按下顺时针转\n"); break;  
            case 4: printf("按键按下逆时针转\n"); break;  
            case 5: printf("按键按下\n"); break;  
            default: break;  
        }  

        ec11_direction = 0;  
        
        pthread_mutex_unlock(&lock);  
    }  
  
    close(ec11_SW_fd);
    close(ec11_A_fd);
    close(ec11_A_fd);
      
    return 0;  
}