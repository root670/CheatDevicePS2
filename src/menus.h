/*
 * Menu Manager
 * Create, modify, transverse, and display menus.
 */

#ifndef MENUS_H
#define MENUS_H

#include <tamtypes.h>
#include "cheats.h"

typedef enum {
    MENU_GAMES,
    MENU_CHEATS,
    MENU_CODES,
    MENU_MAIN,
    MENU_BOOT,
    MENU_SAVE_DEVICES,
    MENU_SAVES,
    NUMMENUS
} menuID_t;

typedef enum {
    MENU_ITEM_NORMAL,
    MENU_ITEM_HEADER
} menuItemType_t;

typedef struct menuItem {
    menuItemType_t type;
    char *text;
    void *extra; // Optional: Associate additional data with the menuItem.
} menuItem_t;

typedef struct menuState {
    menuID_t identifier;
    int isSorted;
    int freeTextWhenRemoved;
    char *text;
    void *extra; // Optional: Associate additional data with the menu.
    
    menuItem_t **items;
    unsigned int currentItem;
    unsigned int numItems;
    unsigned int numChunks;
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
int menuUpRepeat(int n);
int menuDownRepeat(int n);
int menuUpAlpha();
int menuDownAlpha();
int menuGoToTop();
int menuGoToBottom();
void menuToggleItem();

int menuRender();

#endif
