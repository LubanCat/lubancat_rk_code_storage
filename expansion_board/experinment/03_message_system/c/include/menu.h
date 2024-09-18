/*
*
*   file: menu.h
*   update: 2024-09-09
*
*/

#ifndef _MENU_H
#define _MENU_H

#include "oled.h"

#define TITLE_LENGTH_MAX		(20)
#define ITEM_INFO_NUM_MAX		(5)

typedef struct item_info {
	char name[TITLE_LENGTH_MAX];           	// 菜单项名称
	struct menu *next_menu; 				// 该菜单项所关联的下一个菜单
    void (*func)(void *params);             // 该菜单项所关联的可执行函数
	void *params;                           // 可执行函数参数
} item_info_t;

typedef struct menu {
	char title[TITLE_LENGTH_MAX];      		// 菜单名称
	struct item_info *info[ITEM_INFO_NUM_MAX];// 菜单下的菜单项
	struct menu *parent;					// 该菜单项所关联的父菜单
	unsigned int current_page;              // 当前菜单项的页码
    unsigned int total_page;                // 当前菜单的菜单项总数
} menu_t;

item_info_t *item_info_init(const char *name, menu_t *next_menu, void *func, void *params);
menu_t *menu_init(const char *title);

int menu_add_item_info(menu_t *menu, item_info_t *item_info);

void menu_free(menu_t *ptr);
void menu_print(menu_t *menu);

void menu_go_next_info(menu_t **menu);
void menu_go_pre_info(menu_t **menu);
void menu_go_next_menu(menu_t **menu);
void menu_go_pre_menu(menu_t **menu);

#endif