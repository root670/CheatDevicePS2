/*
 * Menu Manager
 * Create, modify, transverse, and display menus.
 */

#ifndef MENUS_H
#define MENUS_H

#include <tamtypes.h>
#include "cheats.h"

#define NUMMENUS 9
typedef enum { GAMEMENU, CHEATMENU, CHEATMINIMENU, CODEMENU, CODEMINIMENU, SETTINGSMENU, ABOUTMENU, MAINMENU, BOOTMENU } menuID_t;
typedef enum { NORMAL, HEADER } menutype_t;
typedef struct menuItem {
	menutype_t type;
	char *text;
	cheatsCheat_t *cheat; // Optional: Cheat menu will use this to associate cheat.
	
	struct menuItem *prev;
	struct menuItem *next;
} menuItem_t;

typedef struct menuState {
	menuID_t identifier;
	char *text;
	cheatsGame_t *game; // Optional: Cheat menu uses this to associate entry with a game.
	
	menuItem_t *head; // linked list
	menuItem_t *current;
	menuItem_t *tail;
} menuState_t;

int initMenuMan();
int killMenuMan();

int menuSetActive(menuID_t id);
menuID_t menuGetActive();

int menuAppendItem(menuItem_t *item);
int menuRemoveActiveItem();
int menuRemoveAllItems();

int menuUp();
int menuDown();
int menuUpAlpha();
int menuDownAlpha();
void menuToggleItem();

int menuRender();

#endif
