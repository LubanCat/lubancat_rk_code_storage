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

#include "ringbuff.h"
#include "mqttclient.h"
#include "cJSON.h"

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

// 上报的温度值
#define TEMP_VALUE                      "35"
// 上报的LED开关值
#define LED_VALUE                       "1"

// MQTT 客户端对象
mqtt_client_t *client = NULL;
// 存储 MQTT 消息的环形缓冲区
ring_buffer_t *mqtt_rb;
// 用于 MQTT 发布的线程对象
pthread_t mqtt_publish_thread_obj;

// 传感器消息结构体，包含传感器名称和其测量值
typedef struct sensor_mes {
    char name[20];
    char value[10];
} sensor_mes_t;

// 声明外部函数 aiotMqttSign，用于生成 MQTT 签名
extern int aiotMqttSign(const char *productKey, const char *deviceName, const char *deviceSecret, char clientId[150], char username[65], char password[65]);

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

        sleep(3);
    }
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
    sensor_mes_t temp_mes_t, led_mes_t;

    /* MQTT初始化 */
    mqtt_log_init();
    ret = mqtt_init();
    if(ret == -1)
    {
        MQTT_LOG_E("mqtt init fail\n");
        goto _err_exit;
    }

    /* 环形缓冲区初始化 */
    mqtt_rb = ring_buffer_init(20, sizeof(sensor_mes_t));
    if(mqtt_rb == NULL)
    {
        MQTT_LOG_E("ringbuff init fail\n");
        goto _err_exit;
    }

    while (1) 
    {
        // 初始化温度传感器消息结构体
        sensor_mes_init(&temp_mes_t, "temperature", TEMP_VALUE);
        // 初始化湿度传感器消息结构体
        sensor_mes_init(&led_mes_t, "led", LED_VALUE);

        // 将温度传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &temp_mes_t);
        // 将湿度传感器消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &led_mes_t);

        sleep(1);
    }

    return 0;

_err_exit:
    return -1;
}
