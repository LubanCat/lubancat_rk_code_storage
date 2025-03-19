/*
*
*   file: main.c
*   update: 2025-05-05
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
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

#include "mqttclient.h"
#include "ringbuff.h"
#include "cJSON.h"
#include "config.h"
#include "mpu6050.h"
#include "atgm332d.h"
#include "http.h"
#include "gaodemap.h"

// 设备信息
#define TUYA_DEVICE_ID			        "2669d2b95856963bb4alms"
#define TUYA_DEVICE_SECRET              "FNjwnHf7sGQCJzhc"

// MQTT服务器信息
#define TUYA_HOST                       "m1.tuyacn.com"
#define TUYA_PORT                       "8883"
#define TUYA_CLIENT_ID                  "tuyalink_"TUYA_DEVICE_ID

// 用于发布数据的 MQTT 主题
#define TUYA_TOPIC_PUBLISH              "tylink/"TUYA_DEVICE_ID"/thing/property/report"

// 高德地图API KEY
#define GAODE_API_KEY                   "cc834f3ef25cb1f3a87bc8f14f8a5873"

mqtt_client_t *client = NULL;                     // MQTT 客户端对象
ring_buffer_t *mqtt_rb = NULL;                    // 环形缓冲区，用于存储 MQTT 消息
pthread_t mqtt_publish_thread_obj;                // MQTT 发布线程对象

pthread_t mpu6050_thread_obj;
int mpu6050_thread_stop = 0;
pthread_t atgm332d_thread_obj;
int atgm332d_thread_stop = 0;              

// 传感器消息结构体，包含传感器名称和其测量值
typedef struct sensor_mes {
    char name[20];
    char value[500];
} sensor_mes_t;

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

/*****************************
 * @brief : 信号处理函数，用于清理资源并安全退出程序
 * @param : sig_num - 信号编号
 * @return: 无返回值
 *****************************/
void sigint_handler(int sig_num) 
{    
    /* 销毁ring buffer */
    ring_buffer_destroy(mqtt_rb);

    /* 释放配置文件 */
    config_free();

    exit(0);  
}

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
 * @brief : 创建符合阿里云 ALINK 协议的 JSON 消息
 * @param : sensor - 指向 sensor_mes_t 结构体的指针，该结构体包含传感器名称和测量值
 * @return: 指向 cJSON 对象的指针，该对象表示创建的 JSON 消息；创建失败返回 NULL
 *****************************/
static cJSON *create_tuya_json(sensor_mes_t *sensor) 
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

        sleep(1);
    }
}

/*****************************
 * @brief : MPU6050 传感器的数据采集线程函数
 * @param : arg - 线程参数，此处未使用
 * @return: none
 *****************************/
void *mpu6050_thread(void *arg)
{
    short int gyro_buf[3], accel_buf[3];
    float lsb_sensitivity_gyro = 16.4; 
    float lsb_sensitivity_accel = 8192;   

    char gyro_xyz_str[30], accel_xyz_str[30];
    sensor_mes_t gyro_mes_t, accel_mes_t;

    while(mpu6050_thread_stop != 1) 
    {  
        /* 读取mpu6050数据 */
        mpu6050_read_gyro(gyro_buf);
        mpu6050_read_accel(accel_buf);
        sprintf(gyro_xyz_str, "%.2f,%.2f,%.2f", gyro_buf[0]/lsb_sensitivity_gyro, gyro_buf[1]/lsb_sensitivity_gyro, gyro_buf[2]/lsb_sensitivity_gyro);
        sprintf(accel_xyz_str, "%.2f,%.2f,%.2f", accel_buf[0]/lsb_sensitivity_accel, accel_buf[1]/lsb_sensitivity_accel, accel_buf[2]/lsb_sensitivity_accel);
        printf("gyro = %s, accel = %s\n", gyro_xyz_str, accel_xyz_str);

        // 初始化gyro消息结构体
        sensor_mes_init(&gyro_mes_t, "mpu6050_gyro", gyro_xyz_str);
        // 初始化accel消息结构体
        sensor_mes_init(&accel_mes_t, "mpu6050_accel", accel_xyz_str);

        // 将gyro消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &gyro_mes_t);
        // 将accel消息写入环形缓冲区
        ring_buffer_write(mqtt_rb, &accel_mes_t);

        sleep(1);
    }   
    
    printf("%s has been exit!\n", __FUNCTION__);
    pthread_exit(NULL);
}

/*****************************
 * @brief : gps经纬度数据采集
 * @param : arg - 线程参数，此处未使用
 * @return: none
 *****************************/
void *atgm332d_thread(void *arg)
{
    int ret;
    double lat, lon;
    sensor_mes_t lon_lat_mes_t;
    char http_url[200], http_response[1024*1024];
    ResponseData response;

    while(atgm332d_thread_stop != 1) 
    {  
        atgm332d_get_latlon_dd(&lat, &lon);
        // printf("%lf,%lf\n", lon, lat);

        /* 将gps坐标转换成高德地图坐标 */
        sprintf(http_url, "https://restapi.amap.com/v3/assistant/coordinate/convert?locations=%lf,%lf&coordsys=gps&output=xml&key=%s", lon, lat, GAODE_API_KEY);
        // printf("url = %s\n", http_url);

        /* 发起http请求 */
        ret = http_get(http_url, http_response);
        if(ret == 0)
        {
            ret = gaode_parse_xml(http_response, &response);
            if(ret == -1)
                continue;
            if(response.status == 0)
            {
                fprintf(stdout, "http request to restapi.amap.com failed, errcode : %s\n", response.infocode);
                sleep(5);
                continue;
            }
        }
        fprintf(stdout, "locations = %s\n", response.locations);

        // 初始化经纬度数据结构体
        sensor_mes_init(&lon_lat_mes_t, "gps_lon_lat", response.locations);

        // 将经纬度数据写入环形缓冲区
        ring_buffer_write(mqtt_rb, &lon_lat_mes_t);

        sleep(5);
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
    
    /* 初始化配置文件 */
    ret = config_init(CONFIG_FILE_NAME);
    if(ret < 0)
    {
        printf("config init error!\n");
        goto _err_exit;
    }

    /* 环形缓冲区初始化 */
    mqtt_rb = ring_buffer_init(20, sizeof(sensor_mes_t));

    /* MQTT初始化 */
    ret = mqtt_init();
    if(ret == -1)
    {
        MQTT_LOG_E("mqtt init fail");
        goto _err_exit;
    }

    /* http初始化 */
    ret = http_init();
    if(ret == -1)
    {
        fprintf(stderr, "http init err\n");
        goto _err_exit;
    }

    /* mpu6050初始化 */
    cJSON *mpu6050_bus = config_get_value("mpu6050", "bus");
    if(mpu6050_bus == NULL)
        goto _err_exit;
    ret = mpu6050_init(mpu6050_bus->valueint, 0x68);
    if(ret == -1)
    {
        fprintf(stderr, "mpu6050 init err\n");
        goto _err_exit;
    }

    /* gps初始化 */
    cJSON *atgm332d_uart = config_get_value("atgm332d", "tty-dev");
    if(atgm332d_uart == NULL)
        goto _err_exit;
    ret |= atgm332d_init(atgm332d_uart->valuestring);
    ret |= atgm332d_thread_start();
    if(ret != 0)
    {
        fprintf(stderr, "atgm332d init err\n");
        return -1;
    }

    /* 创建mpu6050线程 */
    ret = pthread_create(&mpu6050_thread_obj, NULL, mpu6050_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create mpu6050 thread\n");
        goto _err_exit;
    }

    /* 创建gps线程 */
    ret = pthread_create(&atgm332d_thread_obj, NULL, atgm332d_thread, NULL);
    if (ret != 0) 
    {
        printf("Error: Failed to create atgm332d thread\n");
        goto _err_exit;
    }

    while (1) 
    {
        sleep(1);
    }

    return 0;

_err_exit:
    return -1;
}
