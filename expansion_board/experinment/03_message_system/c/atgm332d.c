/*
*
*   file: atgm332d.c
*   update: 2024-09-13
*
*/

#include "atgm332d.h"

int fd_atgm332d;
static pthread_t atgm332d_thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

volatile static int thread_flag = 1;

// $GPRMC,111025.00,A,2517.033747,N,11019.176025,E,0.0,144.8,270920,2.3,W,A*2D\r\n
// $GPRMC,,V,,,,,,,,,,N*53\r\n
// $GPRMC,024443.0,A,2517.038296,N,11019.174048,E,0.0,,120201,0.0,E,A*2F\r\n
static char buf[500];
static char Utctime[20];
static char Lat[20]; 
static char ns[5]; 
static char Lng[20]; 
static char ew[5];
static char Date[20];

/*****************************
 * @brief : 串口配置
 * @param : fd 文件句柄
 * @param : nSpeed 波特率
 * @param : nBits 数据位
 * @param : nEvent 特殊事件配置（如果支持）
 * @param : nStop 停止位
 * @return: 0成功 -1失败
*****************************/
static int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	
	if ( tcgetattr( fd,&oldtio) != 0) { 
		perror("SetupSerial 1");
		return -1;
	}
	
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag |= CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 

	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);     /* Input */
	newtio.c_oflag  &= ~OPOST;                              /* Output */

	switch( nBits )
	{
	case 7:
		newtio.c_cflag |= CS7;
	break;
	case 8:
		newtio.c_cflag |= CS8;
	break;
	}

	switch( nEvent )
	{
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
	break;
	case 'E': 
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
	break;
	case 'N': 
		newtio.c_cflag &= ~PARENB;
	break;
	}

	switch( nSpeed )
	{
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
	break;
	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
	break;
	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
	break;
	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
	break;
	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
	break;
	}
	
	if( nStop == 1 )
		newtio.c_cflag &= ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |= CSTOPB;
	
	newtio.c_cc[VMIN]  = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
	newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间: 
	                         * 比如VMIN设为10表示至少读到10个数据才返回,
	                         * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
	                         * 假设VTIME=1，表示: 
	                         *    10秒内一个数据都没有的话就返回
	                         *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
	                         */

	tcflush(fd,TCIFLUSH);
	
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	//printf("set done!\n");
	return 0;
}

/*****************************
 * @brief : 串口配置
 * @param : com 设备dev
 * @return: 0成功 <0失败
*****************************/
static int open_port(unsigned char *com)
{
	int fd;

	fd = open(com, O_RDWR | O_NOCTTY);
    if(fd == -1)
        return -1;
	
    /* 设置串口为阻塞状态 */
    if(fcntl(fd, F_SETFL, 0) < 0)   
    {
        fprintf(stderr, "fcntl failed!\n");
        return -1;
    }

    return fd;
}

/*****************************
 * @brief : atgm332d读原始数据
 * @param : none
 * @return: 0成功 -1失败
*****************************/
static int atgm332d_read_raw_data()
{
	int i = 0;
	int ret;
	char c;
	int start = 0;
	
	while (1)
	{
		ret = read(fd_atgm332d, &c, 1);
		if (ret == 1)
		{
			if(c == '$')
				start = 1;
			if(start)
			    buf[i++] = c;
			if(c == '\n' || c == '\r')
            {
                buf[i] = '\0';
                return 0;
            }		
		}
		else
		    return -1;
	}
}

/*****************************
 * @brief : atgm332d解析原始数据
 * @param : none
 * @return: 0成功 -1失败
*****************************/
static int atgm332d_parse_raw_data()
{
	char head[10];
    char signal[10];
    char speed[10];
    char azimuth[10];

	if(buf[0] != '$')
		return -1;
	else if(strncmp(buf+3, "RMC", 3) != 0)
		return -1;
	else 
    {
        //$GPRMC,111025.00,A,2517.033747,N,11019.176025,E,0.0,144.8,270920,2.3,W,A*2D\r\n
        
        sscanf(buf, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]", head, Utctime, signal, Lat, ns, Lng, ew, speed, azimuth, Date);
		return 0;
	}
}

/*****************************
 * @brief : atgm332d线程函数
 * @param : arg 函数参数
 * @return: none
*****************************/
static void *atgm332d_thread_read(void *arg) 
{  
    int ret = 0;

    while(thread_flag) 
    {  
        ret = atgm332d_read_raw_data();
        if(ret == 0)
		{
            pthread_mutex_lock(&lock);
			ret = atgm332d_parse_raw_data();
            pthread_mutex_unlock(&lock);
		}
    }   

    printf("atgm332d_thread has been stopped.\n");  
    pthread_exit(NULL);
}

/*****************************
 * @brief : atgm332d初始化
 * @param : com 设备dev
 * @return: 0成功 -1失败
*****************************/
int atgm332d_init(unsigned char *uart_dev)
{
    int ret;

    fd_atgm332d = open_port(uart_dev);
	if(fd_atgm332d < 0)
	{
		printf("open %s err!\n", uart_dev);
		return -1;
	}

    ret = set_opt(fd_atgm332d, 9600, 8, 'N', 1);
	if (ret)
	{
		printf("set port err!\n");
		return -1;
	}

	return 0;
}

/*****************************
 * @brief : atgm332d线程启动
 * @param : none
 * @return: 0成功 -1失败
*****************************/
int atgm332d_thread_start()
{
    int ret = 0;

    if(fd_atgm332d < 0) 
    {  
        fprintf(stderr, "atgm332d can not init!\n");  
        return -1;  
    }

    ret = pthread_create(&atgm332d_thread, NULL, atgm332d_thread_read, NULL);  

    return ret;
}

/*****************************
 * @brief : atgm332d线程停止
 * @param : none
 * @return: none
*****************************/
void atgm332d_thread_stop()
{
    /* 暂不作处理 */
}

/*****************************
 * @brief : atgm332d反初始化
 * @param : none
 * @return: none
*****************************/
void atgm332d_exit()
{
    if(fd_atgm332d > 0)
        close(fd_atgm332d);
}

/*****************************
 * @brief : atgm332d获取经纬度数据
 * @param : lat 纬度
 * @param : lon 经度 
 * @return: none
*****************************/
void atgm332d_get_latlon(double *lat, double *lon)
{
    double _lat, _lon;

    if(strcmp(Lat, "") == 0 || strcmp(Lng, "") == 0)
    {
        *lat = 00.0;
        *lon = 00.0;
        return;
    }

    sscanf(Lat, "%lf", &_lat);
    sscanf(Lng, "%lf", &_lon);

    _lat /= 100;
    _lon /= 100;

    *lat = _lat;
    *lon = _lon;
}

/*****************************
 * @brief : atgm332d获取北京时间
 * @param : year 年
 * @param : month 月
 * @param : day 日
 * @param : hours 时
 * @param : min 分
 * @return: none
*****************************/
void atgm332d_get_bjtime(unsigned int *year, unsigned int *month, unsigned int *day, unsigned int *hours, unsigned int *min)
{
    if(strcmp(Date, "") == 0 || strcmp(Utctime, "") == 0)
    {
        *year = 0;
        *month = 0;
        *day = 0;
        *hours = 0;
        *min = 0;
        return;
    }

    /* 时分 */
    *hours = (Utctime[0] - '0') * 10 + (Utctime[1] - '0');
    *hours += 8;
    if (*hours >= 24)  
        *hours -= 24;  
    *min = (Utctime[2] - '0') * 10 + (Utctime[3] - '0');

    /* 年月日 */
    *day = (Date[0] - '0') * 10 + (Date[1] - '0');  
    *month = (Date[2] - '0') * 10 + (Date[3] - '0');  
    *year = 2000 + (Date[4] - '0') * 10 + (Date[5] - '0');
}