/*
*
*   file: dht11.c
*   date: 2024-08-06
*   usage: 
*       sudo gcc -o dht11 dht11.c
*       sudo ./dht11
*
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdint.h>

#define DEV_NAME "/dev/dht11"

int main(int argc, char **argv)
{
	int fd;
	int ret;
    uint8_t data[6];
	
	/* open dev */
	fd = open(DEV_NAME, O_RDWR);
	if (fd < 0)
	{
		printf("can not open file %s, %d\n", DEV_NAME, fd);
		return -1;
	}

	while(1)
	{
		/* read date from dht11 */
		ret = read(fd, &data, sizeof(data));	
		if(ret)
		    printf("Temperature=%d.%d Humidity=%d.%d\n", data[2], data[3], data[0], data[1]);
        else
            printf("error reading!\n");
		
		sleep(1);
	}
	
	close(fd);
	
	return 0;
}


