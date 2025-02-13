#include "gaodemap.h"

// 解析XML数据的函数
int gaode_parse_xml(const char *xmlData, ResponseData *response) {
    xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;
    xmlChar *key = NULL;
 
    // 解析XML文档
    doc = xmlReadMemory(xmlData, strlen(xmlData), "noname.xml", NULL, 0);
    if (doc == NULL) {
        fprintf(stderr, "Failed to parse XML\n");
        return -1;
    }
 
    // 获取根节点
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        fprintf(stderr, "Empty document\n");
        xmlFreeDoc(doc);
        return -1;
    }
 
    // 遍历节点并提取数据
    cur = cur->xmlChildrenNode;
    while (cur != NULL) {
        if (cur->type == XML_ELEMENT_NODE) {
            key = xmlNodeListGetString(doc, cur->children, 1);
            if (key != NULL) {
                if (xmlStrcmp(cur->name, BAD_CAST "status") == 0) {
                    response->status = atoi((const char *)key);
                } else if (xmlStrcmp(cur->name, BAD_CAST "info") == 0) {
                    strncpy(response->info, (const char *)key, sizeof(response->info) - 1);
                    response->info[sizeof(response->info) - 1] = '\0'; // 确保字符串以null结尾
                } else if (xmlStrcmp(cur->name, BAD_CAST "infocode") == 0) {
                    strncpy(response->infocode, (const char *)key, sizeof(response->infocode) - 1);
                    response->infocode[sizeof(response->infocode) - 1] = '\0'; // 确保字符串以null结尾
                } else if (xmlStrcmp(cur->name, BAD_CAST "locations") == 0) {
                    strncpy(response->locations, (const char *)key, sizeof(response->locations) - 1);
                    response->locations[sizeof(response->locations) - 1] = '\0'; // 确保字符串以null结尾
                }
                xmlFree(key);
            }
        }
        cur = cur->next;
    }
 
    // 释放文档
    xmlFreeDoc(doc);
    xmlCleanupParser();
 
    return 0;
}
