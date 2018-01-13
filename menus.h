/*
 * Menu Manager
 * Create, modify, transverse, and display menus.
 */

#ifndef MENUS_H
#define MENUS_H

#include <tamtypes.h>
#include "cheats.h"

typedef enum { GAMEMENU, CHEATMENU, CODEMENU, SETTINGSMENU, MAINMENU, BOOTMENU, SAVEDEVICEMENU, SAVEMENU, NUMMENUS } menuID_t;
typedef enum { NORMAL, HEADER } menutype_t;
typedef struct menuItem {
    menutype_t type;
    char *text;
    void *extra; // Optional: Associate additional data with the menuItem.
} menuItem_t;

typedef struct menuState {
    menuID_t identifier;
    int isSorted;
    char *text;
    void *extra; // Optional: Associate additional data with the menu.
    
    menuItem_t **items;
    unsigned int currentItem;
    unsigned int numItems;
    unsigned int chunks;
} menuState_t;

int initMenus();
int killMenus();

int menuSetActive(menuID_t id);
menuID_t menuGetActive();
void *menuGetActiveExtra();
void *menuGetExtra(menuID_t id);

int menuInsertItem(menuItem_t *item);
int menuRemoveActiveItem();
int menuRemoveAllItems();
int menuSetActiveItem(menuItem_t *item);
int menuRenameActiveItem(const char *str);
const char *menuGetActiveItemText();
void *menuGetActiveItemExtra();

int menuUp();
int menuDown();
int menuUpAlpha();
int menuDownAlpha();
int menuGoToBottom();
void menuToggleItem();

int menuRender();

#endif
