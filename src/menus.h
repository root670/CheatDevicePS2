/*
 * Menu Manager
 * Create, modify, transverse, and display menus.
 */

#ifndef MENUS_H
#define MENUS_H

#include <tamtypes.h>

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

typedef enum {
    // Executed after drawing standard menu items
    MENU_CALLBACK_AFTER_DRAW,
    // Executed after user presses square on the pad
    MENU_CALLBACK_PRESSED_SQUARE,
    // Executed after user presses cross on the pad
    MENU_CALLBACK_PRESSED_CROSS
} menuCallbackType_t;

typedef struct menuItem {
    menuItemType_t type;
    char *text;
    void *extra; // Optional: Associate additional data with the menuItem.
} menuItem_t;

typedef struct menuState {
    menuID_t identifier;
    int isSorted;
    int freeTextWhenRemoved;
    const char *text;
    void *extra; // Optional: Associate additional data with the menu.

    void (*callbacks[sizeof(menuCallbackType_t)])(const menuItem_t *selected);

    const char *helpTickerText;
    int helpTickerLength;

    menuItem_t **items;
    unsigned int currentItem;
    unsigned int numItems;
    unsigned int numChunks;
} menuState_t;

int initMenus();
int killMenus();

// Setup
int menuSetActive(menuID_t id);
menuID_t menuGetActive();
const char *menuGetActiveText();
void menuSetActiveText(const char *text);
void *menuGetActiveExtra();
void menuSetActiveExtra(void *extra);
void *menuGetExtra(menuID_t id);

// Set a callback function for the active menu.
void menuSetCallback(menuCallbackType_t id, void (*cb)(const menuItem_t*));
// Set the primary help ticker text for the active menu.
void menuSetHelpTickerText(const char *text);
// Temporarily override the primary help ticker text.
void menuSetTempHelpTickerText(const char *text);
// Revert back to using the primary help ticker text.
void menuClearTempHelpTickerText();

// Item manipulation
int menuInsertItem(menuItem_t *item);
int menuRemoveActiveItem();
int menuRemoveAllItems();
int menuSetActiveItem(menuItem_t *item);
int menuRenameActiveItem(const char *str);
const char *menuGetActiveItemText();
void *menuGetActiveItemExtra();
void menuToggleItem();

// Movement
int menuUp();
int menuDown();
int menuUpRepeat(int n);
int menuDownRepeat(int n);
int menuUpAlpha();
int menuDownAlpha();
int menuGoToTop();
int menuGoToBottom();
int menuGoToNextHeader();
int menuGoToPreviousHeader();

int menuRender();

// Process callbacks that are triggered by pad input. Returns 1 if a callback
// for the current input exists and was called, otherwise zero.
int menuProcessInputCallbacks(u32 padPressed);

#endif
