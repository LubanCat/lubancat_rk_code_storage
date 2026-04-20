#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd, ret;
    int button_state;

    if(argc != 2) {
        printf("Usage: ./blockio_app /dev/blockio\n");
        return -1;
    }

    /* 只读打开 */
    fd = open(argv[1], O_RDONLY);
    if(0 > fd) {
        printf("ERROR: %s file open failed!\n", argv[1]);
        return -1;
    }

    for ( ; ; ) {
        /* 阻塞读取 */
        read(fd, &button_state, sizeof(int));
        
        printf("button_state = %d\n",button_state);
    }

    close(fd);
    return 0;
}