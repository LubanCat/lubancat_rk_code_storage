/*
*
*   file: config.h
*   update: 2024-09-20
*   function：用于读取和解析配置文件
*
*/

#include "config.h"

static cJSON *global_config = NULL;
static cJSON *current_board_config = NULL;
static char board_name[256];

/*****************************
 * @brief : 检查配置是否存在
 * @param : filename 文件路径
 * @return: 0存在 -1不存在
*****************************/
static int config_exists(const char *filename) 
{
    if(filename == NULL)
        return -1;

    FILE *file = fopen(filename, "r");
    if(file) 
    {
        fclose(file);
        return 0;
    }

    return -1;
}

/*****************************
 * @brief : 从设备树中获取板卡名
 * @param : none
 * @return: 0获取成功 -1获取失败
*****************************/
static int config_get_board_name_in_dts()
{
    FILE *file;
    char model[256];
    char *temp;

    file = fopen("/proc/device-tree/compatible", "r");
    if(file) 
    {
        if(fgets(model, sizeof(model), file)) 
        {
            model[strcspn(model, "\n")] = 0;
            temp = strstr(model, "lubancat");
            if(temp)
            {
                snprintf(board_name, sizeof(board_name), "%s", temp);
                return 0;
            }
            else
                return -1;
        }

        fclose(file);
    } 

    return -1;
}

/*****************************
 * @brief : 加载配置文件
 * @param : filename 文件路径
 * @return: none
*****************************/
static void config_load(const char *filename) 
{
    char *data = NULL;
    long len = 0;

    if(filename == NULL)
        return;

    if(config_exists(filename) == -1)
        return;
    
    FILE *file = fopen(filename, "rb");
    if(file) 
    {
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        fseek(file, 0, SEEK_SET);
        data = malloc(len + 1);
        fread(data, 1, len, file);
        fclose(file);
        
        data[len] = '\0';
        global_config = cJSON_Parse(data);
        free(data);
    }
}

/*****************************
 * @brief : 检查配置文件中是否存在目标板卡的配置
 * @param : none
 * @return: 0存在 -1不存在
*****************************/
static int config_has_board() 
{
    if(global_config == NULL)
    {
        fprintf(stderr, "[ %s ] : global_config is null, please ensure that the file is normal !\n", __FUNCTION__);
        return -1;
    }

    if(board_name == NULL)
        return -1;

    current_board_config = cJSON_GetObjectItem(global_config, board_name);
    if(current_board_config == NULL)
        return -1;

    return 0;
}

/*****************************
 * @brief : 配置文件初始化
 * @param : none
 * @return: 0初始化成功，可以正常使用 -1初始化失败
*****************************/
int config_init(const char *filename)
{
    int ret;

    /* 检查配置文件是否存在 */
    ret = config_exists(filename);
    if(ret == -1)
    {
        fprintf(stderr, "[ %s ] : can not find %s!\n", __FUNCTION__, filename);
        return ret;
    }

    /* 加载配置文件 */
    config_load(filename);

    /* 从设备树中获取板卡名称 */
    ret = config_get_board_name_in_dts();
    if(ret == -1)
    {
        fprintf(stderr, "[ %s ] : can not get board-name in dts!\n", __FUNCTION__);
        return ret;
    }
    else
    {
        printf("board name : %s\n", board_name);
    }

    /* 检查配置文件中是否存在目标板卡 */
    ret = config_has_board();
    if(ret == -1)
    {
        fprintf(stderr, "[ %s ] : can not find the board-name in %s!\n", __FUNCTION__, filename);
        return ret;
    }

    return 0;
}

/*****************************
 * @brief : 从配置文件中查找指定外设下的指定key的value
 * @param : component 要查找的外设
 * @param : component 要查找的外设下的key
 * @return: 成功返回cJSON指针
*****************************/
cJSON *config_get_value(const char *component, const char *key) 
{
    if(current_board_config == NULL)
    {
        fprintf(stderr, "[ %s ] : current_board_config is null, get value err!\n", __FUNCTION__);
        return NULL;
    }

    cJSON *component_item = cJSON_GetObjectItem(current_board_config, component);
    if(component_item) 
    {
        cJSON *key_item = cJSON_GetObjectItem(component_item, key);
        if(key_item) 
            return key_item;
        else
            fprintf(stderr, "[ %s ] : %s key's value is null!\n", __FUNCTION__, component);
    }
    else
        fprintf(stderr, "[ %s ] : %s key does not exist!\n", __FUNCTION__, component);

    return NULL;
}

/*****************************
 * @brief : 配置文件反初始化，释放相关malloc分配的内存
 * @param : none
 * @return: none
*****************************/
void config_free(void) 
{
    if(global_config) 
    {
        cJSON_Delete(global_config);
        global_config = NULL;
    }

    current_board_config = NULL;
}