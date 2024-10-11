/*
*
*   file: ec11.c
*   update: 2024-09-12
*
*/

#include "ec11.h"

/* fd */
static int ec11_SW_fd = -1, ec11_A_fd = -1, ec11_B_fd = -1;

static int ec11_SW_value = 0;  
static int ec11_A_value = 1;  
static int ec11_B_value = 1;  
volatile static int ec11_direction = EC11_NORMAL;  
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/* thread id */
static pthread_t ec11_SW_thread, ec11_A_thread, ec11_B_thread;

/*****************************
 * @brief : ec11线程函数
 * @param : arg 线程函数参数
 * @return: none
*****************************/
static void *ec11_scan_device(void *arg) 
{  
    int fd = *(int*)arg;  
    struct input_event ie;  
  
    while(1) 
    {  
        if (read(fd, &ie, sizeof(struct input_event)) == sizeof(struct input_event)) 
        {  
            if (ie.type == EV_KEY) 
            {  
                pthread_mutex_lock(&lock); 

                if (fd == ec11_SW_fd && ie.code == 4) 
                {  
                    ec11_SW_value = ie.value;  
                    if (ec11_SW_value == 1 && ec11_A_value == 1 && ec11_B_value == 1)   
                        ec11_direction = EC11_PRESS;                    // 按键按下
                } 
                else if (fd == ec11_A_fd && ie.code == 250) 
                {  
                    if (ie.value == 0 && ec11_B_value == 1)         
                    {  
                        ec11_A_value = 0;  
                        if (ec11_SW_value == 1)                     
                            ec11_direction = EC11_TURN_RIGHT_PRESS;     // 按键顺时针旋转且按下
                        else  
                            ec11_direction = EC11_TURN_RIGHT;           // 按键顺时针旋转
                    } 
                    else if (ie.value == 1) 
                        ec11_A_value = 1;  
                } 
                else if (fd == ec11_B_fd && ie.code == 251) 
                {  
                    if (ie.value == 0 && ec11_A_value == 1)             
                    {  
                        ec11_B_value = 0;  
                        if (ec11_SW_value == 1)                     
                            ec11_direction = EC11_TURN_LEFT_PRESS;      // 按键逆时针旋转且按下
                        else  
                            ec11_direction = EC11_TURN_LEFT;            // 按键逆时针旋转
                    } 
                    else if (ie.value == 1) 
                        ec11_B_value = 1;  
                }  

                pthread_mutex_unlock(&lock);  
            }  
        }  
    }   

    printf("%d has been stopped.\n", fd);  
    pthread_exit(NULL);
}

/*****************************
 * @brief : ec11初始化
 * @param : arg 线程函数参数
 * @return: 0成功 -1失败
*****************************/
int ec11_init(const unsigned char *ec11_sw_event, const unsigned char *ec11_A_event, const unsigned char *ec11_B_event)
{
    int ret = 0;
    
    ec11_SW_fd = open(ec11_sw_event, O_RDONLY);  
    ec11_A_fd = open(ec11_A_event, O_RDONLY);  
    ec11_B_fd = open(ec11_B_event, O_RDONLY);  
  
    if (ec11_SW_fd < 0 || ec11_A_fd < 0 || ec11_B_fd < 0) 
    {  
        fprintf(stderr, "Failed to open input device\n");  
        return -1;  
    }  
  
    return 0;
}

/*****************************
 * @brief : ec11线程启动
 * @param : none
 * @return: 0成功 -1失败
*****************************/
int ec11_thread_start()
{
    int ret = 0;

    if (ec11_SW_fd < 0 || ec11_A_fd < 0 || ec11_B_fd < 0) 
    {  
        fprintf(stderr, "ec11 can not init!\n");  
        return -1;  
    }

    ret |= pthread_create(&ec11_SW_thread, NULL, ec11_scan_device, &ec11_SW_fd);  
    ret |= pthread_create(&ec11_A_thread, NULL, ec11_scan_device, &ec11_A_fd);  
    ret |= pthread_create(&ec11_B_thread, NULL, ec11_scan_device, &ec11_B_fd);

    return ret;
}

/*****************************
 * @brief : ec11获取按键状态
 * @param : none
 * @return: 0按键无动作 1按键顺时针旋转 2按键逆时针旋转 3按键顺时针旋转且按下 4按键逆时针旋转且按下 5按键按下
*****************************/
int ec11_get_value()
{
    int temp = ec11_direction;
    if(temp == EC11_NORMAL)
        return temp;

    pthread_mutex_lock(&lock);
    ec11_direction = EC11_NORMAL;
    pthread_mutex_unlock(&lock);

    return temp;
}

/*****************************
 * @brief : ec11线程停止
 * @param : none
 * @return: none
*****************************/
void ec11_thread_stop()
{
    /* 暂不作处理 */
}

/*****************************
 * @brief : ec11反初始化
 * @param : none
 * @return: none
*****************************/
void ec11_exit()
{
    if(ec11_SW_fd > 0)
        close(ec11_SW_fd);

    if(ec11_A_fd > 0)
        close(ec11_A_fd);

    if(ec11_B_fd > 0)
        close(ec11_B_fd);
}