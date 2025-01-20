/*
*
*   file: main.c
*   update: 2024-12-31
*   usage: 
*       make
*       ./make
*
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <gpiod.h>
#include <math.h>

#include "mqttclient.h"
#include "ringbuff.h"
#include "cJSON.h"
#include "config.h"
#include "bme280.h"
#include "ads1115.h"

// 阿里云 MQTT 服务器的主机地址
#define ALIYUN_HOST                     ".iot-as-mqtt.cn-shanghai.aliyuncs.com"
// 阿里云的产品密钥
#define ALIYUN_PRODUCT_KEY			    "a1pwoLHW8Tl"
// 设备名称
#define ALIYUN_DEVICE_NAME			    "lubancat"
// 设备密钥
#define ALIYUN_DEVICE_SECRET            "160cacffcba7b83f2eb896403688dac5"

// 用于发布数据的 MQTT 主题
#define ALIYUN_TOPIC_PUBLISH            "/sys/"ALIYUN_PRODUCT_KEY"/"ALIYUN_DEVICE_NAME"/thing/event/property/post"
// 用于订阅数据的 MQTT 主题
#define ALIYUN_TOPIC_SUBSCRIB           "/sys/"ALIYUN_PRODUCT_KEY"/"ALIYUN_DEVICE_NAME"/thing/event/property/post_reply"


// MQTT 客户端对象
mqtt_client_t *client = NULL;
// 存储 MQTT 消息的环形缓冲区
ring_buffer_t *mqtt_rb;
// 用于 MQTT 发布的线程对象
pthread_t mqtt_publish_thread_obj;

struct gpiod_chip *ldr_gpiochip;   
struct gpiod_chip *ntc_gpiochip;
struct gpiod_line *ldr_line;          
struct gpiod_line *ntc_line;

// 用于 DHT11 传感器的线程对象
pthread_t dht11_thread_obj;
// 用于控制 DHT11 线程的停止标志
int dht11_thread_stop = 0;              
// DHT11 设备的文件描述符
int dht11_fd;                                        

// 用于光敏和热敏传感器的线程对象
pthread_t ldrntc_thread_obj;
// 用于控制光敏和热敏传感器线程的停止标志
int ldrntc_thread_stop = 0;              

// 用于 BMP280 传感器的线程对象
pthread_t bmp280_thread_obj;
// 用于控制 BMP280 线程的停止标志
int bmp280_thread_stop = 0;

// 用于 MQ135 传感器的线程对象
pthread_t mq135_thread_obj;
// 用于控制 MQ135 线程的停止标志
int mq135_thread_stop = 0;

// 传感器消息结构体，包含传感器名称和其测量值
typedef struct sensor_mes {
    char name[20];
    char value[10];
} sensor_mes_t;

// 声明外部函数 aiotMqttSign，用于生成 MQTT 签名
extern int aiotMqttSign(const char *productKey, const char *deviceName, const char *deviceSecret, char clientId[150], char username[65], char password[65]);

/*****************************
 * @brief : 信号处理函数，用于清理资源并安全退出程序
 * @param : sig_num - 信号编号
 * @return: 无返回值
 *****************************/
void sigint_handler(int sig_num) 
{    
    /* 退出dht11线程 */
    dht11_thread_stop = 1;
    pthread_join(dht11_thread_obj, NULL);

    /* 退出光敏和热敏线程 */
    ldrntc_thread_stop = 1;
    pthread_join(ldrntc_thread_obj, NULL);

    /* 退出BMP280线程 */
    bmp280_thread_stop = 1;
    pthread_join(bmp280_thread_obj, NULL);

    /* 退出MQ135线程 */
    mq135_thread_stop = 1;
    pthread_join(mq135_thread_obj, NULL);

    /* 关闭dht11 */
    close(dht11_fd);

    /* 反初始化光敏和热敏传感器 */
    if(ldr_line)
    {
        gpiod_line_release(ldr_line);
        ldr_line = NULL;
    }
    if(ntc_line)
    {
        gpiod_line_release(ntc_line);
        ntc_line = NULL;
    }

    /* bme280反初始化 */
    bme280_exit();

    /* ads1115反初始化 */
    ads1115_exit();

    /* 销毁ring buffer */
    ring_buffer_destroy(mqtt_rb);

    /* 释放配置文件 */
    config_free();

    exit(0);  
}

/*****************************
 * @brief : 创建符合阿里云 ALINK 协议的 JSON 消息
 * @param : sensor - 指向 sensor_mes_t 结构体的指针，该结构体包含传感器名称和测量值
 * @return: 指向 cJSON 对象的指针，该对象表示创建的 JSON 消息；创建失败返回 NULL
 *****************************/
static cJSON *create_alink_json(sensor_mes_t *sensor) 
{   
    if(sensor == NULL)
        return NULL;

    if(sensor->name == NULL || sensor->value == NULL)
        return NULL;

    // 创建根节点，对应整个 JSON 对象
    cJSON *root = cJSON_CreateObject();

    // 添加 "id" 字段，可能用于消息的唯一标识
    cJSON_AddStringToObject(root, "id", "123");
    // 添加 "version" 字段，可能表示消息的版本
    cJSON_AddStringToObject(root, "version", "1.0");

    // 创建 "sys" 子对象，可能包含系统相关信息
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddNumberToObject(sys, "ack", 0);
    cJSON_AddItemToObject(root, "sys", sys);

    // 创建 "params" 子对象，存储传感器参数信息
    cJSON *params = cJSON_CreateObject();

    // 创建以传感器名称命名的子对象，并添加 "value" 字段存储传感器的测量值
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "value", sensor->value);
    cJSON_AddItemToObject(params, sensor->name, item);

    cJSON_AddItemToObject(root, "params", params);

    // 添加 "method"
    cJSON_AddStringToObject(root, "method", "thing.event.property.post");

    return root;
}

/*****************************
 * @brief : 初始化传感器消息结构体
 * @param : sensor - 指向 sensor_mes_t 结构体的指针，用于存储传感器消息信息
 * @param : name - 传感器名称的字符串指针
 * @param : value - 传感器测量值的字符串指针
 * @return: none
 *****************************/
static void sensor_mes_init(sensor_mes_t *sensor, const char *name, const char *value)
{
    if(sensor == NULL)
        return;
    
    if(name == NULL || value == NULL)
        return;

    strcpy(sensor->name, name);
    strcpy(sensor->value, value);
}

/*****************************
 * @brief : 处理接收到的 MQTT 主题消息
 * @param : client - 指向 MQTT 客户端的指针，此处被强制转换为 void* 类型
 * @param : msg - 指向 message_data_t 结构体的指针，包含 MQTT 消息信息
 * @return: none
 *****************************/
static void topic_handle(void* client, message_data_t* msg)
{
    (void) client;
    MQTT_LOG_I("-----------------------------------------------------------------------------------");
    MQTT_LOG_I("topic: %s", msg->topic_name);
    MQTT_LOG_I("message:%s", (char*)msg->message->payload);
    MQTT_LOG_I("-----------------------------------------------------------------------------------\n");
}

/*****************************
 * @brief : MQTT 发布线程函数
 * @param : arg - 指向 MQTT 客户端的指针，作为线程函数的参数传递
 * @return: none
 *****************************/
void *mqtt_publish_thread(void *arg)
{
    mqtt_client_t *client = (mqtt_client_t *)arg;

    int ret;
    sensor_mes_t payload;
    mqtt_message_t msg;

    cJSON *alink_json;
    char *alink_json_str;

    memset(&msg, 0, sizeof(msg));
    msg.qos = 0;

    sleep(2);

    // 订阅 MQTT 主题
    mqtt_list_subscribe_topic(client);

    while(1) 
    {
        // 从环形缓冲区读取传感器消息
        ret = ring_buffer_read(mqtt_rb, &payload);
        if(ret == -1)
            continue;
        
        // 根据传感器消息创建 JSON 消息
        alink_json = create_alink_json(&payload);
        alink_json_str = cJSON_Print(alink_json);

        msg.payload = (void *)alink_json_str;
        // printf("\nalink json : \n");
        printf("%s\n", alink_json_str);

        // 发布消息到指定的 MQTT 主题
        mqtt_publish(client, ALIYUN_TOPIC_PUBLISH, &msg);
        cJSON_Delete(alink_json);
        alink_json = NULL;

        sleep(1);
    }
}

/*****************************
 * @brief : DHT11 传感器的数据采集线程函数
 * @param : arg - 线程参数，此处未使用
 * @return: none
 *****************************/
void *dht11_thread(void *arg)
{
    int ret;
    uint8_t data[6];                    // 存储DHT11数据
    char temp_str[10], humi_str[10];
    float ftemp = 0, fhumi = 0;
    sensor_mes_t temp_mes_t, humi_mes_t;

    if(dht11_fd < 0)                    // 检查DHT11设备是否打开成功
        return NULL;

    while(dht11_thread_stop != 1) 
    {  
        /* 读取dht11温湿度数据 */
		ret = read(dht11_fd, &data, sizeof(data));	
		if(ret)
        {
            ftemp = data[2] + data[3] * 0.01;    // 提取温度
            fhumi = data[0] + data[1] * 0.01;    // 提取湿度
        }
        else
            printf("read data from dth11 err!\n");

        // printf("temperature=%.2f humidity=%.2f\n", ftemp, fhumi);

        // 将温度数据转换为字符串
        sprintf(temp_str, "%.2f", ftemp);
        // 将湿度数据转换为字符串
        sprintf(humi_str, "%.2f", fhumi);

        // 初始化温度传感器消息结构体
        sensor_mes_init(&temp_mes_t, "temperature", temp_str);
        // 初始化湿度传感器消息结构体
        sensor_mes_init(&humi_mes_t, "Humidity", humi_str);

        // 将温度传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &temp_mes_t);
        // 将湿度传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &humi_mes_t);

        // 温湿度数据不需要频繁上报，10s 更新一次 
        sleep(10);
    }   
    
    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

/*****************************
 * @brief : 光敏和热敏传感器的数据采集线程函数
 * @param : arg - 线程参数，此处未使用
 * @return: none
 *****************************/
void *ldrntc_thread(void *arg)
{
    int ldr, ntc;
    char ldr_str[5], ntc_str[5];
    sensor_mes_t ldr_mes_t, ntc_mes_t;

    while(ldrntc_thread_stop != 1) 
    {  
        // 获取光敏传感器数据
        ldr = gpiod_line_get_value(ldr_line);         
        // 获取热敏传感器数据
        ntc = gpiod_line_get_value(ntc_line);         

        //printf("ldr=%d ntc=%d\n", ldr, ntc);

        // 将光敏传感器数据转换为字符串
        sprintf(ldr_str, "%d", ldr);
        // 将热敏传感器数据转换为字符串
        sprintf(ntc_str, "%d", ntc);

        // 初始化光敏传感器消息结构体
        sensor_mes_init(&ldr_mes_t, "ldr", ldr_str);
        // 初始化热敏传感器消息结构体
        sensor_mes_init(&ntc_mes_t, "ntc", ntc_str);

        // 将光敏传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &ldr_mes_t);
        // 将热敏传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &ntc_mes_t);

        sleep(5);
    }   
    
    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

/*****************************
 * @brief : BMP280 传感器的数据采集线程函数
 * @param : arg - 线程参数，此处未使用
 * @return: none
 *****************************/
void *bmp280_thread(void *arg)
{
    float bmp280_pres = 0;
    char bmp280_pres_str[10];
    sensor_mes_t bmp280_mes_t;

    while(bmp280_thread_stop != 1) 
    {  
        // 获取大气压数据
        bmp280_pres = bme280_get_pres();        
        bmp280_pres = bmp280_pres * 0.001;
        // printf("bmp280 pressure = %.2f\n", bmp280_pres);

        // 将气压数据转换为字符串
        sprintf(bmp280_pres_str, "%.2f", bmp280_pres);

        // 初始化气压传感器消息结构体
        sensor_mes_init(&bmp280_mes_t, "Atmosphere", bmp280_pres_str);

        // 将气压传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &bmp280_mes_t);

        sleep(5);
    }   
    
    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

/*****************************
 * @brief : MQ135 传感器的数据采集线程函数
 * @param : arg - 线程参数，此处未使用
 * @return: none
 *****************************/
void *mq135_thread(void *arg)
{
    double ads1115_vol = 0;

    float a = 5.06;                         // a b为甲苯检测中的校准常数                     
    float b = 2.46;
    double vrl_clean = 0.576028;            // 洁净空气下的平均Vrl电压值（就是adc读到的平均电压值）
    double ratio, ppm;

    char ppm_str[10];
    sensor_mes_t ppm_mes_t;

    while(mq135_thread_stop != 1) 
    {  
        // 获取 adc 电压值
        ads1115_vol = ads1115_read_vol();
        // 利用实际测量时的 Vrl 与洁净空气下的 Vrl 来计算比例
        ratio = ads1115_vol / vrl_clean;    
        // 计算 ppm 值
        ppm = a * pow(ratio, b);

        //printf("ppm = %.2f\n", ppm);

        // 将 ppm 值转换为字符串
        sprintf(ppm_str, "%.2f", ppm);

        // 初始化空气质量传感器消息结构体
        sensor_mes_init(&ppm_mes_t, "AQI", ppm_str);

        // 将空气质量传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &ppm_mes_t);

        sleep(10);
    }   
    
    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

/*****************************
 * @brief : 初始化 MQTT 相关配置
 * @param : none
 * @return: 0 表示初始化成功，-1 表示初始化失败
 *****************************/
int mqtt_init()
{
    int rc = 0;
    char clientId[150] = {0};
	char username[65] = {0};
	char password[65] = {0};

    // 初始化 MQTT 客户端
    client = mqtt_lease();
    if(client == NULL)
        return -1;

    // 生成 MQTT 密码
    if ((rc = aiotMqttSign(ALIYUN_PRODUCT_KEY, ALIYUN_DEVICE_NAME, ALIYUN_DEVICE_SECRET, clientId, username, password) < 0)) 
    {
        printf("aiotMqttSign -%0x4x\n", -rc);
        return -1;
    }
    // printf("clientid: %s\n", clientId);
    // printf("username: %s\n", username);
    // printf("password: %s\n", password);

    // 设置 MQTT 消息的端口
    mqtt_set_port(client, "1883");
    // 设置 MQTT 消息的主机
    mqtt_set_host(client, ALIYUN_PRODUCT_KEY".iot-as-mqtt.cn-shanghai.aliyuncs.com");
    // 设置 MQTT 客户端 ID
    mqtt_set_client_id(client, clientId);
    // 设置 MQTT 用户名
    mqtt_set_user_name(client, username);
    // 设置 MQTT 密码
    mqtt_set_password(client, password);
    // 设置 MQTT 会话为清除模式
    mqtt_set_clean_session(client, 1);

    // 连接到 MQTT 服务器
    mqtt_connect(client);

    // 订阅主题
    mqtt_subscribe(client, ALIYUN_TOPIC_SUBSCRIB, QOS0, topic_handle);

    // 创建 MQTT 发布线程
    rc = pthread_create(&mqtt_publish_thread_obj, NULL, mqtt_publish_thread, client);
    if(rc!= 0) {
        MQTT_LOG_E("create mqtt publish thread fail");
        return -1;
    }

    return 0;
}

int main()
{
    int ret;

    /* 初始化配置文件 */
    ret = config_init(CONFIG_FILE_NAME);
    if(ret < 0)
    {
        printf("config init error!\n");
        return -1;
    }

    /* 环形缓冲区初始化 */
    mqtt_rb = ring_buffer_init(20, sizeof(sensor_mes_t));

    /* MQTT初始化 */
    mqtt_log_init();
    ret = mqtt_init();
    if(ret == -1)
    {
        MQTT_LOG_E("mqtt init fail");
        goto _err_exit;
    }

    /* dht11初始化 */
    cJSON *dht11_dev = config_get_value("dht11", "devname");
    if(dht11_dev == NULL)
        return -1;

    dht11_fd = open(dht11_dev->valuestring, O_RDWR);
    if(dht11_fd < 0)
    {
        fprintf(stderr, "dht11 init error!\n");
        return -1;
    }

    /* 热敏模块、光敏模块初始化 */
    char ldr_pin_chip[20], ntc_pin_chip[20];
    cJSON *ldrchip = config_get_value("ldr", "pin_chip");
    cJSON *ldrnum = config_get_value("ldr", "pin_num");
    cJSON *ntcchip = config_get_value("ntc", "pin_chip");
    cJSON *ntcnum = config_get_value("ntc", "pin_num");
    if(ldrchip == NULL || ntcchip == NULL || ldrnum == NULL || ntcnum == NULL)
        return -1;

    sprintf(ldr_pin_chip, "/dev/gpiochip%s", ldrchip->valuestring);
    sprintf(ntc_pin_chip, "/dev/gpiochip%s", ntcchip->valuestring);
    ldr_gpiochip = gpiod_chip_open(ldr_pin_chip);  
    ntc_gpiochip = gpiod_chip_open(ntc_pin_chip);
    if(ldr_gpiochip == NULL || ntc_gpiochip == NULL)
    {
        fprintf(stderr, "gpiod_chip_open error!\n");
        return -1;
    }

    ldr_line = gpiod_chip_get_line(ldr_gpiochip, ldrnum->valueint);
    ntc_line = gpiod_chip_get_line(ntc_gpiochip, ntcnum->valueint);
    if(ldr_line == NULL || ntc_line == NULL)
    {
        fprintf(stderr, "gpiod_chip_get_line error!\n");
        return -1;
    }

    ret = gpiod_line_request_input(ldr_line, "ldr"); 
    ret = gpiod_line_request_input(ntc_line, "ntc"); 
    if(ret < 0)
    {
        printf("set sensor_line to input mode error!\n");
        return -1;
    }

    /* bmp280初始化 */
    char bmp280spi[20], bmp280cschip[20];
    cJSON *bmp280_spi = config_get_value("bmp280", "bus");
    cJSON *cs_chip = config_get_value("bmp280", "cs_chip");
    cJSON *cs_pin = config_get_value("bmp280", "cs_pin");
    if(bmp280_spi == NULL || cs_chip == NULL || cs_pin == NULL)
        return -1;

    sprintf(bmp280spi, "/dev/spidev%d.0", bmp280_spi->valueint);
    sprintf(bmp280cschip, "/dev/gpiochip%s", cs_chip->valuestring);
    ret = bme280_init(bmp280spi, bmp280cschip, cs_pin->valueint);
    if(ret == -1)
    {
        printf("bmp280 init err!\n");
        return -1;
    }

    /* ADS1115初始化 */
    cJSON *ads1115_bus = config_get_value("ads1115", "bus");
    if(ads1115_bus == NULL)
        return -1;
    ret = ads1115_init(ads1115_bus->valueint);
    if(ret == -1)
    {
        printf("ads1115 init err!\n");
        return -1;
    }

    /* 创建DHT11线程 */
    ret = pthread_create(&dht11_thread_obj, NULL, dht11_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create dht11 thread\n");
        goto _err_exit;
    }

    /* 创建光敏、热敏线程 */
    ret = pthread_create(&ldrntc_thread_obj, NULL, ldrntc_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create ldrntc thread\n");
        goto _err_exit;
    }

    /* 创建bmp280线程 */
    ret = pthread_create(&bmp280_thread_obj, NULL, bmp280_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create bmp280 thread\n");
        goto _err_exit;
    }

    /* 创建mq135线程 */
    ret = pthread_create(&mq135_thread_obj, NULL, mq135_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create mq135 thread\n");
        goto _err_exit;
    }

    while (1) 
    {
        sleep(100);
    }

    return 0;

_err_exit:
    return -1;
}
