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
int killMenuMan()
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

int menuInsertItemSorted(menuItem_t *item)
{
    if(initialized)
    {
        if(activeMenu->head != NULL)
        {
            menuItem_t *node = activeMenu->head;
            while(node->next && strcmp(node->text, item->text) < 0)
                node = node->next;

            printf("inserting %s before %s\n", item->text, node->text);

            if(node->next == NULL) // insert at end of list
                menuAppendItem(item);
            else // insert within list
            {
                item->next = node;
                item->prev = node->prev;
                if(node->prev)
                    node->prev->next = item;
                else // first item of list
                    activeMenu->head = item;
                node->prev = item;
            }

            return 1;
        }
        else
            return menuAppendItem(item);
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

        if(activeMenu->current->text)
            free(activeMenu->current->text);

        if(!prev) // first game in list
        {
            next->prev = NULL;
            activeMenu->head = next;
            activeMenu->current = next;
        }
        else
        {
            prev->next = next;
            next->prev = prev;
            activeMenu->current = prev;
        }

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

int menuSetActiveItem(menuItem_t *item)
{
    if(!item)
        return 0;

    activeMenu->current = item;

    return 1;
}/*
            menuItem_t *node = activeMenu->head;
            while(node->next && strcmp(node->text, item->text) < 0)
                node = node->next;

            printf("inserting %s before %s\n", item->text, node->text);

            if(node->next == NULL) // insert at end of list
                menuAppendItem(item);
            else // insert within list
            {
                item->next = node;
                item->prev = node->prev;
                if(node->prev)
                    node->prev->next = item;
                else // first item of list
                    activeMenu->head = item;
                node->prev = item;
            }
*/
int menuRenameActiveItem(const char *str)
{
    menuItem_t *node;
    int direction;

    if(!activeMenu)
        return 0;

    if(!activeMenu->current)
        return 0;

    direction = strcmp(activeMenu->current->text, str);
    node = activeMenu->current;

    printf("direction %d\n", direction);

    if(direction < 0)
    {
        // Move down list
        while(node->next && strcmp(node->text, str) < 0)
            node = node->next;
        printf("inserting %s before %s\n", str, node->text);

        activeMenu->current->next->prev = activeMenu->current->prev;
        activeMenu->current->prev->next = activeMenu->current->next;

        if(node == activeMenu->tail)
        {
            printf("it's the new tail!\n");
            activeMenu->tail = activeMenu->current;
            activeMenu->current->next = NULL;
            activeMenu->current->prev = node;
            node->next = activeMenu->current;
        }
        else
        {
            activeMenu->current->next = node;
            activeMenu->current->prev = node->prev;
            node->prev->next = activeMenu->current;
            node->prev = activeMenu->current;
        }
    }
    else if(direction > 0)
    {
        // Move up list
        while(node->prev && strcmp(node->text, str) > 0)
            node = node->prev;
        printf("inserting %s after %s\n", str, node->text);

        activeMenu->current->prev->next = activeMenu->current->next;
        activeMenu->current->next->prev = activeMenu->current->prev;

        if(node == activeMenu->head)
        {
            printf("it's the new head!\n");

            activeMenu->head = activeMenu->current;
            activeMenu->current->next = node;
            activeMenu->current->prev = NULL;
            node->prev = activeMenu->current;
        }
        else
        {
            activeMenu->current->next = node->next;
            activeMenu->current->prev = node;
            node->next->prev = activeMenu->current;
            node->next = activeMenu->current;
        }
    }
    else
    {
        // Name not changed
        return 0;
    }

    free(activeMenu->current->text);
    activeMenu->current->text = calloc(1, strlen(str) + 1);
    strcpy(activeMenu->current->text, str);

    return 1;
}

void *menuGetActiveItemExtra()
{
    if(!activeMenu)
        return NULL;

    if(!activeMenu->current)
        return NULL;

    return activeMenu->current->extra;
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
        //activeMenu->game = cheatsLoadCheatMenu(menues[GAMEMENU].current->text);
        activeMenu->game = cheatsLoadCheatMenu((cheatsGame_t *)menues[GAMEMENU].current->extra);
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
        const char **paths;
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
    
    else if (id == SAVEDEVICEMENU)
    {
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
    if(activeMenu->identifier == CHEATMENU && activeMenu->current && ((cheatsCheat_t *) activeMenu->current->extra)->type != CHEATHEADER)
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
            break;
        case SAVEDEVICEMENU:
        case CHEATMENU:
        case CODEMENU:
        case BOOTMENU:
            graphicsDrawTextCentered(47, activeMenu->text, WHITE);
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
        case NUMMENUS:
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
                else if(activeMenu->identifier == GAMEMENU && cheatsIsActiveGame((cheatsGame_t *) item->extra))
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
