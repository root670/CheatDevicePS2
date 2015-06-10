#include "menus.h"
#include "graphics.h"
#include "cheats.h"
#include "settings.h"
#include "saves.h"
#include <stdio.h>
#include <malloc.h>

static menuState_t menues[NUMMENUS];
static menuState_t *activeMenu = NULL;
static int initialized = 0;

int initMenuMan()
{
	if(!initialized)
	{
		int i;
		for(i = 0; i < NUMMENUS; i++)
		{
			menues[i].identifier = (menuID_t)i;
			menues[i].text = NULL;
			menues[i].head = NULL;
			menues[i].current = NULL;
			menues[i].tail = NULL;
		}

		activeMenu = &menues[GAMEMENU];
		initialized = 1;
		return 1;
	}

	return 0;
}

// free all menu items. free all menustates
int killMenuMan();

int menuAppendItem(menuItem_t *item)
{
	if(initialized)
	{
		if(activeMenu->head != NULL)
		{
			activeMenu->tail->next = item;
			item->prev = activeMenu->tail;
			item->next = NULL;
			activeMenu->tail = item;
		}
		else // first item
		{
			activeMenu->head = item;
			activeMenu->tail = item;
			activeMenu->current = activeMenu->head;
			activeMenu->current->prev = NULL;
			activeMenu->current->next = NULL;
		}

		return 1;
	}

	return 0;
}

// Remove active item and sets previous item as the new active.
int menuRemoveActiveItem()
{
	if(activeMenu->current != NULL)
	{
		menuItem_t *prev = activeMenu->current->prev;
		menuItem_t *next = activeMenu->current->next;
		free(activeMenu->current);
		activeMenu->current = prev;
		if(activeMenu->current)
			activeMenu->current->next = next;

		return 1;
	}
	else
		return 0;
}

int menuRemoveAllItems()
{
	menuItem_t *node = activeMenu->head;
	
	while(node)
	{
		menuItem_t *next = node->next;
		if(node->text)
			free(node->text);
		
		free(node);
		node = next;
	}

	activeMenu->head = NULL;
	activeMenu->current = NULL;
	return 1;
}

menuID_t menuGetActive()
{
	return activeMenu->identifier;
}

int menuSetActive(menuID_t id)
{
	if( id > NUMMENUS-1 )
		return 0;

	activeMenu = &menues[id];

	if(id == CHEATMENU && (!activeMenu->text || strcmp(activeMenu->text, menues[GAMEMENU].current->text) != 0)) // Refresh cheat menu if a new game was chosen
	{
		menuRemoveAllItems();
		activeMenu->text = menues[GAMEMENU].current->text;
		activeMenu->game = cheatsLoadCheatMenu(menues[GAMEMENU].current->text);
	}

	else if(id == CODEMENU && (!activeMenu->text || strcmp(activeMenu->text, menues[CHEATMENU].current->text) != 0)) // Refresh code menu if a new cheat was chosen
	{
		if(menues[CHEATMENU].current->type == NORMAL)
		{
			menuRemoveAllItems();
			activeMenu->text = menues[CHEATMENU].current->text;
			// TODO: cheatsLoadCodeMenu(menues[GAMEMENU].current->text, menues[CHEATMENU].current->text);
		}
		else
		{
			activeMenu = &menues[CHEATMENU]; // Header doesn't have any codes to see, so go back
			// TODO: Open a folder containing cheats under the header. Would require creating closed folders beforehand.
		}
	}
	
	else if(id == BOOTMENU)
	{
		char **paths;
		int numPaths;
		
		activeMenu->text = strdup("Boot paths");
		
		paths = settingsGetBootPaths(&numPaths);
		int i;
		
		/* Disc boot; default option */
		menuItem_t *discBoot = calloc(1, sizeof(menuItem_t));
		discBoot->type = NORMAL;
		discBoot->text = strdup("==Disc==");
		menuAppendItem(discBoot);
		
		for(i = 0; i < numPaths; i++)
		{
			menuItem_t *item = calloc(1, sizeof(menuItem_t));
			item->type = NORMAL;
			item->text = strdup(paths[i]);
			menuAppendItem(item);
		}
	}
	
	else if(id == SAVEMENU)
	{
		initSaveMan();
		activeMenu->text = strdup("Save Manager");
	}

	return 1;
}

int menuUp()
{
	if(activeMenu->current && activeMenu->current->prev)
	{
		activeMenu->current = activeMenu->current->prev;
		return 1;
	}
	else
		return 0;
}

int menuDown()
{
	if(activeMenu->current && activeMenu->current->next)
	{
		activeMenu->current = activeMenu->current->next;
		return 1;
	}
	else
		return 0;
}

int menuUpAlpha()
{
	if(activeMenu->current && activeMenu->current->prev)
	{
		menuItem_t *item = activeMenu->current->prev;
		activeMenu->current = item;
		
		char firstChar = activeMenu->current->text[0];
		
		while(item->prev && (item->prev->text[0] == firstChar))
			item = item->prev;
			
		activeMenu->current = item;
		return 1;
	}
	
	return 0;
}

int menuDownAlpha()
{
	if(activeMenu->current && activeMenu->current->next)
	{
		char firstChar = activeMenu->current->text[0];
		menuItem_t *item = activeMenu->current;
		
		while(item->next && (item->text[0] == firstChar))
			item = item->next;
		
		activeMenu->current = item;
		return 1;
	}
	
	return 0;
}

void menuToggleItem()
{
	if(activeMenu->identifier == CHEATMENU && ((cheatsCheat_t *) activeMenu->current->extra)->type != CHEATHEADER)
	{
		cheatsSetActiveGame(activeMenu->game);
		cheatsToggleCheat((cheatsCheat_t *) activeMenu->current->extra);
	}
	
	if(activeMenu->identifier == BOOTMENU)
	{
		startgameExecute(activeMenu->current->text);
	}
	
	if(activeMenu->identifier == SAVEMENU && activeMenu->current->type != HEADER)
	{
		savesCopySavePrompt((gameSave_t *) activeMenu->current->extra);
	}
}

int menuRender()
{
	int yItems = 14;
	int yAbove = 7;
	int y = 76;
	menuItem_t *item = activeMenu->current;

	switch(activeMenu->identifier)
	{
		case SAVEMENU:
			savesDrawTicker();
		case CHEATMENU:
		case CODEMENU:
		case BOOTMENU:
			graphicsDrawText(50, 46, activeMenu->text, WHITE);
			break;
		case ABOUTMENU:
			graphicsDrawAboutPage();
			return 1;
			break;
		case MAINMENU:
			return 1;
			break;
		case GAMEMENU:
		case CHEATMINIMENU:
		case CODEMINIMENU:
		case SETTINGSMENU:
			break;
	};

	while(yAbove-- > 0 && item && item->prev)
		item = item->prev;
	
	/* Render visible items */
	int i;
	for(i = 0; i < yItems; ++i)
	{
		if(item != NULL)
		{
			if(item->type == NORMAL)
			{
				if(activeMenu->identifier == CHEATMENU && item->extra && ((cheatsCheat_t *) item->extra)->enabled)
					graphicsDrawText(50, y, item->text, YELLOW);
				else
					graphicsDrawText(50, y, item->text, WHITE);
			}
			else
			{
				graphicsDrawText(50, y, item->text, GREEN);
			}

			if(item == activeMenu->current)
				graphicsDrawPointer(28, y+5);

			y += 22;
			item = item->next;
		}
	}

	return 1;
}
