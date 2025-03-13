#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

int main(int argc, char *argv[])
{
    struct ifreq ifr = {0};
    struct sockaddr_can can_addr = {0};
    struct can_frame frame = {0};
    unsigned char buffer[4] = {0};
    int sockfd = -1;
    unsigned int cnt = 0;
    int ret;

    // 设置CAN波特率为500000
    system("ip link set can0 down");
    system("ip link set can0 type can bitrate 500000 fd off");
    system("ip link set can0 up");

    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(0 > sockfd) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    /* 指定 can0 设备 */
    strcpy(ifr.ifr_name, "can0");
    ioctl(sockfd, SIOCGIFINDEX, &ifr);
    can_addr.can_family = AF_CAN;
    can_addr.can_ifindex = ifr.ifr_ifindex;

    /* 将 can0 与套接字进行绑定 */
    ret = bind(sockfd, (struct sockaddr *)&can_addr, sizeof(can_addr));
    if (0 > ret) {
        perror("bind error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    setsockopt(sockfd, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

    memcpy(frame.data, buffer, sizeof(buffer));
    frame.can_dlc = sizeof(buffer);
    frame.can_id = 0x123;

    for (;;) {
        frame.data[0] = (cnt >> 24) & 0xff;
        frame.data[1] = (cnt >> 16) & 0xff;
        frame.data[2] = (cnt >> 8) & 0xff;
        frame.data[3] = cnt & 0xff;
        
        cnt++;
        ret = write(sockfd, &frame, sizeof(frame));
        if (sizeof(frame) != ret) {
            perror("write error");
            break;
        }
        usleep(10 * 1000);
    }    

    close(sockfd);
    exit(EXIT_SUCCESS);
}