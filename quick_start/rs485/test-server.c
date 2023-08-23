
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_GPIO_INDEX   "90"  //485-1的换流引脚
#define SERVER_ID         17
const uint16_t UT_REGISTERS_TAB[] = { 0x0A, 0x0E, 0x0A, 0x1B,0x0A};

static int _server_ioctl_init(void)
{
   int fd;
   //index config
   fd = open("/sys/class/gpio/export", O_WRONLY);
   if(fd < 0)
         return 1;

   write(fd, SERVER_GPIO_INDEX, strlen(SERVER_GPIO_INDEX));
   close(fd);

   //direction config
   fd = open("/sys/class/gpio/gpio" SERVER_GPIO_INDEX "/direction", O_WRONLY);
   if(fd < 0)
         return 2;

   write(fd, "out", strlen("out"));
   close(fd);

   return 0;
}

static int _server_ioctl_on(void)
{
   int fd;

   fd = open("/sys/class/gpio/gpio" SERVER_GPIO_INDEX "/value", O_WRONLY);
   if(fd < 0)
         return 1;

   write(fd, "1", 1);
   close(fd);
   return 0;
}

static int _server_ioctl_off(void)
{
   int fd;

   fd = open("/sys/class/gpio/gpio" SERVER_GPIO_INDEX "/value", O_WRONLY);
   if(fd < 0)
         return 1;

   write(fd, "0", 1);
   close(fd);
   return 0;
}

static void _modbus_rtu_server_ioctl(modbus_t *ctx, int on)
{
   if (on) {
         _server_ioctl_on();
   } else {
         _server_ioctl_off();
   }
}

int main(int argc, char*argv[])
{
   modbus_t *ctx;
   modbus_mapping_t *mb_mapping;
   int rc;
   int i;
   uint8_t *query;
   /*设置串口信息*/
   ctx = modbus_new_rtu("/dev/ttyS3", 9600, 'N', 8, 1);
   _server_ioctl_init();

   /*设置从机地址，设置485模式*/
   modbus_set_slave(ctx, SERVER_ID);
   modbus_rtu_set_custom_rts(ctx, _modbus_rtu_server_ioctl);
   modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_UP);
   modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485);


   query = malloc(MODBUS_RTU_MAX_ADU_LENGTH);
   /*开启调试*/
   modbus_set_debug(ctx, TRUE);

   mb_mapping = modbus_mapping_new_start_address(0,0,0,0,0,5,0,0);
   if (mb_mapping == NULL) {
         fprintf(stderr, "Failed to allocate the mapping: %s\n",
               modbus_strerror(errno));
         modbus_free(ctx);
         return -1;
   }

   /* 初始化值 */
   for (i=0; i < 5; i++) {
         mb_mapping->tab_registers[i] = UT_REGISTERS_TAB[i];
   }

   rc = modbus_connect(ctx);
   if (rc == -1) {
         fprintf(stderr, "Unable to connect %s\n", modbus_strerror(errno));
         modbus_free(ctx);
         return -1;
   }
   modbus_flush(ctx);

   for (;;) {
         do {
            rc = modbus_receive(ctx, query);
         } while (rc == 0);

         rc = modbus_reply(ctx, query, rc, mb_mapping);
         if (rc == -1) {
            break;
         }
   }

   modbus_mapping_free(mb_mapping);
   free(query);

   modbus_close(ctx);
   modbus_free(ctx);
   return 0;
}
