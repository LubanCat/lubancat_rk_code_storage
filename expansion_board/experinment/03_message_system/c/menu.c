/*
*
*   file: menu.c
*   update: 2024-09-09
*
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "menu.h"

/*****************************
 * @brief : 菜单项初始化
 * @param : name 菜单项名称
 * @param : next_menu 所要关联的下一级菜单
 * @param : func 所要执行的函数
 * @param : params 所要执行的函数的参数
 * @return: 分配一个item_info_t，并返回其地址
*****************************/
item_info_t *item_info_init(const char *name, menu_t *next_menu, void *func, void *params)
{
    if (name == NULL || strlen(name) > TITLE_LENGTH_MAX)  
        return NULL;
    
    item_info_t *item_info = (item_info_t *)malloc(sizeof(item_info_t));
    if(item_info == NULL)
    {
        fprintf(stderr, "item_info malloc err!\n");
        return NULL;
    }

    /* item info init */
    /* name */
    memcpy(item_info->name, name, strlen(name));
    
    /* next_menu */
    item_info->next_menu = next_menu;

    /* func init */
    item_info->func = func;

    /* params init */
    item_info->params = params;

    return item_info;
}

/*****************************
 * @brief : 菜单初始化
 * @param : title 菜单名称
 * @return: 分配一个menu_t，并返回其地址
*****************************/
menu_t *menu_init(const char *title)
{
    int i = 0;

    if (title == NULL || strlen(title) > TITLE_LENGTH_MAX)  
        return NULL; 

    menu_t *menu = (menu_t *)malloc(sizeof(menu_t));
    if(menu == NULL)
    {
        fprintf(stderr, "menu malloc err!\n");
        return NULL;
    }

    /* menu init */
    /* title */
    memcpy(menu->title, title, strlen(title));
    
    /* info */
    for(i = 0; i < ITEM_INFO_NUM_MAX; i++)
    {
        menu->info[i] = (item_info_t *)malloc(sizeof(item_info_t));
        if(menu->info[i] == NULL)
        {
            fprintf(stderr, "menu->info[%d] malloc err!\n", i);
            return NULL;
        }

        memset(menu->info[i]->name, 0, sizeof(menu->info[i]->name));
        menu->info[i]->next_menu = NULL;
        menu->info[i]->func = NULL;
        menu->info[i]->params = NULL;
    }
    
    /* parent menu */
    menu->parent = NULL;

    /* page */
    menu->current_page = 1;
    menu->total_page = 0;

    return menu;
}

/*****************************
 * @brief : 将菜单项添加进菜单
 * @param : menu 菜单
 * @param : item_info 菜单项
 * @return: 0成功 -1失败
*****************************/
int menu_add_item_info(menu_t *menu, item_info_t *item_info)
{
    if(menu == NULL || item_info == NULL)
        return -1;

    if(menu->total_page > ITEM_INFO_NUM_MAX)
    {
        fprintf(stderr, "meun->info full!\n");
        return -1;
    }

    memcpy(menu->info[menu->total_page]->name, item_info->name, strlen(item_info->name));
    menu->info[menu->total_page]->next_menu = item_info->next_menu;
    menu->info[menu->total_page]->func = item_info->func;
    menu->info[menu->total_page]->params = item_info->params;
    menu->total_page++;

    free(item_info);
    item_info = NULL;

    return 0;
}

/*****************************
 * @brief : 打印指定菜单
 * @param : ptr 要打印的菜单
 * @return: none
*****************************/
void menu_print(menu_t *ptr)
{
    int i = 0;
    int j = 0;
    static int count = 1;

    if(ptr != NULL)
    {
        printf("|- %s\n", ptr->title);

        for(i = 0; i < ptr->total_page; i++)
        {
            for(j = 0; j < count; j++)
                printf("|---");

            if(ptr->info[i]->next_menu != NULL)
            {
                count++;
                menu_print(ptr->info[i]->next_menu);
                count--;
                continue;
            }
            printf("|- %s\n", ptr->info[i]->name);
        }
    }
}

/*****************************
 * @brief : 释放指定菜单
 * @param : ptr 要释放的菜单
 * @return: none
*****************************/
void menu_free(menu_t *ptr)
{
    int i = 0;

    if(ptr != NULL)
    {
        for(i = 0; i < ptr->total_page; i++)
        {
            if(ptr->info[i]->next_menu != NULL)
            {
                menu_free(ptr->info[i]->next_menu);
                continue;
            }
            free(ptr->info[i]);
        }
        free(ptr);
        ptr = NULL;
    }
}

/*****************************
 * @brief : 切换到下一个菜单项
 * @param : menu 当前菜单
 * @return: none
*****************************/
void menu_go_next_info(menu_t **menu)
{
    if((*menu)->current_page >= (*menu)->total_page)
        return;
    (*menu)->current_page++;
}

/*****************************
 * @brief : 切换到上一个菜单项
 * @param : menu 当前菜单
 * @return: none
*****************************/
void menu_go_pre_info(menu_t **menu)
{
    if((*menu)->current_page <= 1)
        return;
    (*menu)->current_page--;
}

/*****************************
 * @brief : 进入当前菜单项所关联的菜单
 * @param : menu 当前菜单
 * @return: none
*****************************/
void menu_go_next_menu(menu_t **menu)
{
    item_info_t *info = (*menu)->info[(*menu)->current_page - 1];

    if(info->next_menu == NULL)
    {
        if(info->func == NULL)
            return;

        info->func((void *)(info->params));
        return;
    }

    info->next_menu->parent = *menu;
    *menu = info->next_menu;
}

/*****************************
 * @brief : 退出当前菜单
 * @param : menu 当前菜单
 * @return: none
*****************************/
void menu_go_pre_menu(menu_t **menu)
{
    if((*menu)->parent == NULL)
        return;

    *menu = (*menu)->parent;
}

