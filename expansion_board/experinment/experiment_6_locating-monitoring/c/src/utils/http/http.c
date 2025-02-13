#include "http.h"

static CURL *curl;

// 回调函数，用于处理从服务器返回的数据
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) 
{
    ((char *)userp)[0] = '\0';  // 先清空缓冲区
    strncat((char *)userp, (char *)contents, size * nmemb); // 追加数据到缓冲区
    return size * nmemb;
}

int http_get(const char *youurl, char *buffer) 
{
    CURLcode res;

    if(curl) 
    {
        // 设置请求的URL，替换为你的API密钥和城市代码
        char url[256];
        snprintf(url, sizeof(url), "%s", youurl);
        
        // 设置URL选项
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        // 设置回调函数，用于处理返回的数据
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        
        // 设置用户数据，即存储返回数据的缓冲区
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)buffer);
        
        // 设置接收数据超时，5s
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
        
        // 设置连接超时，3s
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);

        // 执行请求
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) 
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } 
        else 
        {
            //printf("Response: %s\n", buffer);
            return 0;
        }
    }
    
    return -1;
}

int http_init()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) 
        return 0;        
    
    return -1;
}

void http_exit()
{
    if(curl)
        curl_easy_cleanup(curl);

    curl = NULL; 
    curl_global_cleanup();
}