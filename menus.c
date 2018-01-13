#include "menus.h"
#include "graphics.h"
#include "cheats.h"
#include "settings.h"
#include "saves.h"
#include "startgame.h"
#include <stdio.h>
#include <malloc.h>

static menuState_t menues[NUMMENUS];
static menuState_t *activeMenu = NULL;
static int initialized = 0;
static char *menuTitleSaveMenu = "Save Manager";
static char *menuTitleBootMenu = "Boot Paths";

#define CHUNK_SIZE 1000

int initMenus()
{
    if(!initialized)
    {
        int i;
        for(i = 0; i < NUMMENUS; i++)
        {
            menues[i].identifier = (menuID_t)i;
            menues[i].text = NULL;
            menues[i].items = calloc(CHUNK_SIZE, sizeof(menuItem_t *));
            menues[i].currentItem = 0;
            menues[i].numItems = 0;
            menues[i].chunks = 1;
        }

        menues[GAMEMENU].isSorted = 1;
        menues[SAVEMENU].isSorted = 1;

        activeMenu = &menues[GAMEMENU];
        initialized = 1;
        return 1;
    }

    return 0;
}

int killMenus()
{
    int i;
    
    if(initialized)
    {
        for(i = 0; i < NUMMENUS; i++)
        {
            activeMenu = &menues[i];
            menuRemoveAllItems();
        }
        
        initialized = 0;
        return 1;
    }
    
    return 0;
}

int menuInsertItem(menuItem_t *item)
{
    if(initialized)
    {
        if(activeMenu->numItems == (activeMenu->chunks * CHUNK_SIZE))
        {
            activeMenu->chunks++;
            printf("increasing menu size to %d chunks\n", activeMenu->chunks);
            activeMenu->items = realloc(activeMenu->items, CHUNK_SIZE * sizeof(menuItem_t *) * activeMenu->chunks);
        }

        if(activeMenu->numItems == 0 || !activeMenu->isSorted)
        {
            activeMenu->items[activeMenu->numItems] = item;
        }
        else
        {
            // Insert item while maintaining alphabetical order
            int i;
            for(i = 0; i < activeMenu->numItems; i++)
            {
                if(strcmp(activeMenu->items[i]->text, item->text) > 0)
                    break;
            }

            if(i < activeMenu->numItems)
            {
                // Shift items down by 1 position
                memmove(&activeMenu->items[i + 1], &activeMenu->items[i], sizeof(menuItem_t *) * (activeMenu->numItems - i));
            }

            activeMenu->items[i] = item;
        }

        activeMenu->numItems++;

        return 1;
    }

    return 0;
}

// Remove active item and sets previous item as the new active.
int menuRemoveActiveItem()
{
    if(activeMenu->numItems > 0)
    {
        if(activeMenu->items[activeMenu->currentItem]->text)
            free(activeMenu->items[activeMenu->currentItem]->text);
        
        if(activeMenu->identifier != GAMEMENU && activeMenu->identifier != CHEATMENU && activeMenu->identifier != BOOTMENU)
            free(activeMenu->items[activeMenu->currentItem]);
        
        activeMenu->numItems--;

        if(activeMenu->numItems > 0)
        {
            // Shift up items after curentItem.
            memmove(&activeMenu->items[activeMenu->currentItem], &activeMenu->items[activeMenu->currentItem + 1], sizeof(menuItem_t *) * (activeMenu->numItems - activeMenu->currentItem));
        }

        if(activeMenu->currentItem > 0)
            activeMenu->currentItem--;
        
        return 1;
    }
    else
        return 0;
}

int menuRemoveAllItems()
{
    int i;

    for(i = 0; i < activeMenu->numItems; i++)
    {
        menuItem_t *item = activeMenu->items[i];

        if(item->text)
            free(item->text);
    }

    if(activeMenu->identifier == CHEATMENU && activeMenu->items[0])
    {
        free(activeMenu->items[0]);
    }

    activeMenu->currentItem = 0;
    activeMenu->numItems = 0;

    return 1;
}

int menuSetActiveItem(menuItem_t *item)
{
    int i = 0;
    if(!item)
        return 0;

    for(i = 0; i < activeMenu->numItems; i++)
    {
        if(activeMenu->items[i] == item)
        {
            activeMenu->currentItem = i;
            return 1;
        }
    }

    // didn't find the item
    return 0;
}

int menuRenameActiveItem(const char *str)
{
    menuItem_t *node;

    if(!activeMenu)
        return 0;

    node = activeMenu->items[activeMenu->currentItem];

    if(activeMenu->isSorted)
    {
        // Reposition item while maintaining alphabetical order
        int i;
        for(i = 0; i < activeMenu->numItems; i++)
        {
            if(strcmp(activeMenu->items[i]->text, str) > 0)
                break;
        }

        if(abs(i - activeMenu->currentItem) == 1)
        {
            // Rename in place
            free(node->text);
            node->text = strdup(str);

            return 1;
        }

        if(i < activeMenu->currentItem)
        {
            // Shift items down
            memmove(&activeMenu->items[i + 1], &activeMenu->items[i], sizeof(menuItem_t *) * (activeMenu->currentItem - i));
        }
        else if(i > activeMenu->currentItem)
        {
            // Shift items up
            memmove(&activeMenu->items[activeMenu->currentItem], &activeMenu->items[activeMenu->currentItem + 1], sizeof(menuItem_t *) * (i - activeMenu->currentItem - 1));
        }

        if(i == activeMenu->numItems)
        {
            // Renamed item went to end of list
            i--;
        }

        activeMenu->items[i] = node;
        activeMenu->currentItem = i;
    }

    free(node->text);
    node->text = strdup(str);

    return 1;
}

void *menuGetActiveItemExtra()
{
    if(!activeMenu)
        return NULL;

    if(!activeMenu->items[activeMenu->currentItem])
        return NULL;

    return activeMenu->items[activeMenu->currentItem]->extra;
}

const char *menuGetActiveItemText()
{
    if(!activeMenu)
        return NULL;
    
    if(!activeMenu->items[activeMenu->currentItem])
        return NULL;

    return activeMenu->items[activeMenu->currentItem]->text;
}

menuID_t menuGetActive()
{
    return activeMenu->identifier;
}

void *menuGetActiveExtra()
{
    return activeMenu->extra;
}

void *menuGetExtra(menuID_t id)
{
    if(id > NUMMENUS-1)
        return NULL;

    return menues[id].extra;
}

int menuSetActive(menuID_t id)
{
    if( id > NUMMENUS-1 )
        return 0;

    if(id == CHEATMENU && cheatsGetNumGames() == 0)
        return 0;

    activeMenu = &menues[id];

    if(id == CHEATMENU && (!activeMenu->text || strcmp(activeMenu->text, menues[GAMEMENU].items[menues[GAMEMENU].currentItem]->text) != 0)) // Refresh cheat menu if a new game was chosen
    {
        menuRemoveAllItems();
        activeMenu->text = menues[GAMEMENU].items[menues[GAMEMENU].currentItem]->text;
        activeMenu->extra = cheatsLoadCheatMenu((cheatsGame_t *)menues[GAMEMENU].items[menues[GAMEMENU].currentItem]->extra);
    }
    else if(id == CODEMENU && (!activeMenu->text || strcmp(activeMenu->text, menues[CHEATMENU].items[menues[CHEATMENU].currentItem]->text) != 0)) // Refresh code menu if a new cheat was chosen
    {
        if(menues[CHEATMENU].items[menues[CHEATMENU].currentItem]->type == NORMAL)
        {
            menuRemoveAllItems();
            activeMenu->text = menues[CHEATMENU].items[menues[CHEATMENU].currentItem]->text;
            activeMenu->extra = cheatsLoadCodeMenu((cheatsCheat_t *)menues[CHEATMENU].items[menues[CHEATMENU].currentItem]->extra,
            (cheatsGame_t *) menues[CHEATMENU].extra);
        }
        else
        {
            activeMenu = &menues[CHEATMENU]; // Header doesn't have any codes to see, so go back
        }
    }
    else if(id == BOOTMENU)
    {
        activeMenu->text = menuTitleBootMenu;
        menuRemoveAllItems();
        settingsLoadBootMenu();
    }
    
    else if (id == SAVEDEVICEMENU)
    {
        activeMenu->text = menuTitleSaveMenu;
    }

    return 1;
}

int menuUp()
{
    if(activeMenu->currentItem > 0)
    {
        activeMenu->currentItem--;
        return 1;
    }
    else
        return 0;
}

int menuDown()
{
    if(activeMenu->numItems > 0)
    {
        if(activeMenu->currentItem < activeMenu->numItems - 1)
        {
            activeMenu->currentItem++;
            return 1;
        }
    }

    return 0;
}

int menuUpAlpha()
{
    int idx;
    char firstChar;

    if(activeMenu->currentItem > 0)
    {
        activeMenu->currentItem--;
        idx = activeMenu->currentItem;

        firstChar = activeMenu->items[idx]->text[0];

        while(idx > 0 && (activeMenu->items[idx - 1]->text[0] == firstChar))
            idx--;

        activeMenu->currentItem = idx;
        return 1;
    }


    return 0;
}

int menuDownAlpha()
{
    int idx;
    char firstChar;

    if(activeMenu->numItems > 0)
    {
        if(activeMenu->currentItem < activeMenu->numItems - 1)
        {
            activeMenu->currentItem++;
            idx = activeMenu->currentItem;

            firstChar = activeMenu->items[idx]->text[0];

            while(idx < (activeMenu->numItems - 1) && (activeMenu->items[idx]->text[0] == firstChar))
                idx++;

            activeMenu->currentItem = idx;
            return 1;
        }
    }

    return 0;
}

int menuGoToBottom()
{
    if(activeMenu->numItems > 0)
    {
        activeMenu->currentItem = activeMenu->numItems - 1;
        return 1;
    }

    return 0;
}

void menuToggleItem()
{
    void *extra;
    char *text;
    menutype_t type;

    if(activeMenu->numItems > 0)
    {
        extra = activeMenu->items[activeMenu->currentItem]->extra;
        text = activeMenu->items[activeMenu->currentItem]->text;
        type = activeMenu->items[activeMenu->currentItem]->type;

        if(activeMenu->identifier == CHEATMENU && ((cheatsCheat_t *) extra)->type == CHEATNORMAL)
        {
            cheatsSetActiveGame(activeMenu->extra);
            cheatsToggleCheat((cheatsCheat_t *) extra);
        }
        else if(activeMenu->identifier == BOOTMENU)
        {
            settingsSave();
            startgameExecute(text);
        }
        else if(activeMenu->identifier == SAVEMENU && type != HEADER)
        {
            savesCopySavePrompt((gameSave_t *) extra);
        }
    }
}

void drawMenuItems()
{
    int yItems = 14;
    int yAbove = 7;
    int y = 76;
    int idx;

    /* Find top of viewport */
    if((activeMenu->numItems - activeMenu->currentItem) >= yAbove || (activeMenu->numItems < 8))
        idx = activeMenu->currentItem;
    else
        idx = activeMenu->numItems - yAbove;

    while(yAbove-- > 0 && idx > 0)
        idx--;
    
    /* Render visible items */
    int i;
    for(i = 0; i < yItems; ++i)
    {
        if(idx < activeMenu->numItems)
        {
            menuItem_t *item = activeMenu->items[idx];

            if(item->type == NORMAL)
            {
                if(activeMenu->identifier == CHEATMENU && item->extra && ((cheatsCheat_t *) item->extra)->enabled)
                    graphicsDrawText(50, y, item->text, YELLOW);
                else if(activeMenu->identifier == GAMEMENU &&
                        cheatsIsActiveGame((cheatsGame_t *) item->extra) &&
                        cheatsGetNumEnabledCheats() > 0)
                {
                    graphicsDrawText(50, y, item->text, YELLOW);
                }
                else
                    graphicsDrawText(50, y, item->text, WHITE);
            }
            else
            {
                graphicsDrawText(50, y, item->text, GREEN);
            }

            if(idx == activeMenu->currentItem)
                graphicsDrawPointer(28, y+5);

            y += 22;
            idx++;
        }
    }
}

int menuRender()
{
    if(activeMenu->identifier == MAINMENU)
    {
        return 1;
    }
    else if(activeMenu->identifier == SAVEMENU)
    {
        savesDrawTicker();
    }
    else if(activeMenu->identifier != GAMEMENU)
    {
        if(activeMenu->text != NULL)
        {
            graphicsDrawTextCentered(47, activeMenu->text, WHITE);
        }
    }

    drawMenuItems();

    return 1;
}
