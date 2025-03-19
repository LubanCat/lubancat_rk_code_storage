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
#include <time.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include "ringbuff.h"
#include "mqttclient.h"
#include "cJSON.h"

// 设备信息
#define TUYA_DEVICE_ID			        "2632d4478100fda674lxlu"
#define TUYA_DEVICE_SECRET              "qxZS1LhriSGzzqiE"

// MQTT服务器信息
#define TUYA_HOST                       "m1.tuyacn.com"
#define TUYA_PORT                       "8883"
#define TUYA_CLIENT_ID                  "tuyalink_"TUYA_DEVICE_ID

// 用于发布数据的 MQTT 主题
#define TUYA_TOPIC_PUBLISH              "tylink/"TUYA_DEVICE_ID"/thing/property/report"

// 上报的温度值
#define TEMP_VALUE                      "35"
// 上报的LED开关值
#define LED_VALUE                       "1"

mqtt_client_t *client = NULL;                     // MQTT 客户端对象
ring_buffer_t *mqtt_rb = NULL;                    // 环形缓冲区，用于存储 MQTT 消息
pthread_t mqtt_publish_thread_obj;                // MQTT 发布线程对象

// CA 证书，用于 TLS 连接（嵌入代码中）
static const char *ca_crt = {
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n"
  "EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n"
  "EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n"
  "ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n"
  "NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n"
  "EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n"
  "AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n"
  "DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n"
  "E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH\n"
  "/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy\n"
  "DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh\n"
  "GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR\n"
  "tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA\n"
  "AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n"
  "FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX\n"
  "WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu\n"
  "9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr\n"
  "gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo\n"
  "2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO\n"
  "LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI\n"
  "4uJEvlz36hz1\n"
  "-----END CERTIFICATE-----\n"
};

// 传感器消息结构体，包含传感器名称和其测量值
typedef struct sensor_mes {
    char name[20];
    char value[10];
} sensor_mes_t;

/***************************
 * @brief : 生成 HMAC-SHA256 哈希值
 * @param : key - HMAC 算法的密钥（字符串形式）
 * @param : data - 需要进行哈希计算的数据（字符串形式）
 * @param : output - 用于存储哈希结果的输出缓冲区（必须有足够的空间，长度至少为 64 字节）
 * @return: 无返回值，结果存储在 output 参数中
 ***************************/
void generate_hmac_sha256(const char *key, const char *data, char *output) 
{
    unsigned char *digest;
    digest = HMAC(EVP_sha256(), key, strlen(key), (unsigned char *)data, strlen(data), NULL, NULL);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) 
    {
        sprintf(output + (i * 2), "%02x", digest[i]);
    }
}

/*****************************
 * @brief : 创建符合涂鸦智能平台的 JSON 消息
 * @param : sensor - 指向 sensor_mes_t 结构体的指针，该结构体包含传感器名称和测量值
 * @return: 指向 cJSON 对象的指针，该对象表示创建的 JSON 消息；创建失败返回 NULL
 *****************************/
static cJSON *create_tuya_json(sensor_mes_t *sensor) 
{   
    if(sensor == NULL)
        return NULL;

    if(sensor->name == NULL || sensor->value == NULL)
        return NULL;

    /* json example:
        dht11_data = {
            "msgId":str(uuid.uuid4()),
            "time":current_time,
            "data":{
                "temperature":{
                    "value": temp_value,
                    "time": current_time  
                }
            }
        }
    */

    time_t t = time(NULL);
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%ld", t);

    // 创建根节点，对应整个 JSON 对象
    cJSON *root = cJSON_CreateObject();

    // 添加 "msgID" 字段
    cJSON_AddStringToObject(root, "msgID", timestamp);
    // 添加 "time" 字段
    cJSON_AddStringToObject(root, "time", timestamp);

    // 创建 "data" 子对象
    cJSON *data = cJSON_CreateObject();

    // 创建以传感器名称命名的子对象，并添加 "value" 字段存储传感器的测量值
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "value", sensor->value);
    cJSON_AddStringToObject(item, "time", timestamp);

    cJSON_AddItemToObject(data, sensor->name, item);
    cJSON_AddItemToObject(root, "data", data);

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

    cJSON *tuya_json;
    char *tuya_json_str;

    memset(&msg, 0, sizeof(msg));
    msg.qos = 0;

    sleep(2);

    while(1) 
    {
        // 从环形缓冲区读取传感器消息
        ret = ring_buffer_read(mqtt_rb, &payload);
        if(ret == -1)
            continue;
        
        // 根据传感器消息创建 JSON 消息
        tuya_json = create_tuya_json(&payload);
        tuya_json_str = cJSON_Print(tuya_json);

        msg.payload = (void *)tuya_json_str;
        // printf("\ntuya json : \n");
        // printf("%s\n", tuya_json_str);

        // 发布消息到指定的 MQTT 主题
        mqtt_publish(client, TUYA_TOPIC_PUBLISH, &msg);
        cJSON_Delete(tuya_json);
        tuya_json = NULL;

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

    // 获取当前时间戳
    time_t t = time(NULL);
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%ld", t);

    // 构建 UserName
    char username[256];
    snprintf(username, sizeof(username), "%s|signMethod=hmacSha256,timestamp=%s,secureMode=1,accessType=1", TUYA_DEVICE_ID, timestamp);
    // printf("UserName: %s\n", username);

    // 构建待签名的数据
    char data_for_signature[256];
    snprintf(data_for_signature, sizeof(data_for_signature), "deviceId=%s,timestamp=%s,secureMode=1,accessType=1", TUYA_DEVICE_ID, timestamp);
 
    // 生成 HMAC-SHA256 签名
    char password[65]; 
    generate_hmac_sha256(TUYA_DEVICE_SECRET, data_for_signature, password);
    // printf("Password: %s\n", password);

    // 初始化 MQTT 客户端
    client = mqtt_lease();
    if(client == NULL)
        return -1;

    mqtt_set_port(client, TUYA_PORT);               // 设置 MQTT 消息的端口
    mqtt_set_host(client, TUYA_HOST);               // 设置 MQTT 消息的主机
    mqtt_set_ca(client, (char*)ca_crt);             // 设置 CA 证书
    mqtt_set_user_name(client, username);           // 设置 MQTT 用户名
    mqtt_set_password(client, password);            // 设置 MQTT 密码
    mqtt_set_client_id(client, TUYA_CLIENT_ID);     // 设置 MQTT 客户端 ID
    mqtt_set_clean_session(client, 1);              // 设置 MQTT 会话为清除模式  

    // 连接到 MQTT 服务器
    mqtt_connect(client);

    // 创建 MQTT 发布线程
    rc = pthread_create(&mqtt_publish_thread_obj, NULL, mqtt_publish_thread, client);
    if(rc!= 0) 
    {
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
