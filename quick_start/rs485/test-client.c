
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CLIENT_GPIO_INDEX   "91"  //485-2的换流引脚
#define SERVER_ID         17

static int _client_ioctl_init(void)
{
   int fd;
   //index config
   fd = open("/sys/class/gpio/export", O_WRONLY);
   if(fd < 0)
         return 1 ;

   write(fd, CLIENT_GPIO_INDEX, strlen(CLIENT_GPIO_INDEX));
   close(fd);

   //direction config
   fd = open("/sys/class/gpio/gpio" CLIENT_GPIO_INDEX "/direction", O_WRONLY);
   if(fd < 0)
         return 2;

   write(fd, "out", strlen("out"));
   close(fd);

   return 0;
}

static int _client_ioctl_on(void)
{
   int fd;

   fd = open("/sys/class/gpio/gpio" CLIENT_GPIO_INDEX "/value", O_WRONLY);
   if(fd < 0)
         return 1;

   write(fd, "0", 1);
   close(fd);

   return 0;
}

static int _client_ioctl_off(void)
{
   int fd;

   fd = open("/sys/class/gpio/gpio" CLIENT_GPIO_INDEX "/value", O_WRONLY);
   if(fd < 0)
         return 1;

   write(fd, "1", 1);
   close(fd);

   return 0;
}

static void _modbus_rtu_client_ioctl(modbus_t *ctx, int on)
{
   if (on) {
         _client_ioctl_on();
   } else {
         _client_ioctl_off();
   }
}

int main(int argc, char *argv[])
{
   modbus_t *ctx = NULL;
   int i,rc;
   uint16_t tab_rp_registers[5] = {0}; //定义存放数据的数组

   /*创建一个RTU类型的变量*/
   /*设置要打开的串口设备  波特率 奇偶校验 数据位 停止位*/
   ctx = modbus_new_rtu("/dev/ttyS4", 9600, 'N', 8, 1);
   if (ctx == NULL) {
         fprintf(stderr, "Unable to allocate libmodbus context\n");
         return -1;
   }

   /*设置485模式*/
   _client_ioctl_init();
   modbus_rtu_set_custom_rts(ctx, _modbus_rtu_client_ioctl);
   modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_DOWN);
   modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);

   /*设置debug模式*/
   modbus_set_debug(ctx, TRUE);

   /*设置从机地址*/
   modbus_set_slave(ctx, SERVER_ID);

   /*RTU模式 打开串口*/
   if (modbus_connect(ctx) == -1) {
         fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
         modbus_free(ctx);
         return -1;
   }

   //读取多个连续寄存器
   rc = modbus_read_registers(ctx, 0, 5, tab_rp_registers);
   if (rc == -1)
   {
            fprintf(stderr,"%s\n", modbus_strerror(errno));
            return -1;
   }
   for (i=0; i<5; i++)
   {
         //打印读取的数据
         printf("reg[%d] = %d(0x%x)\n", i, tab_rp_registers[i], tab_rp_registers[i]);
   }

   modbus_close(ctx);
   modbus_free(ctx);
   return 0;
}
