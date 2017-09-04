#include "cheats.h"
#include "database.h"
#include "textcheats.h"
#include "menus.h"
#include "graphics.h"
#include "util.h"
#include "hash.h"
#include <debug.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <erl.h>

static cheatsGame_t *gamesHead = NULL;
static cheatsGame_t *activeGame = NULL;
static cheatDatabaseType_t dbType;
static hashTable_t *gameHashes;
static hashTable_t *cheatHashes;
static FILE *historyFile;
static int numGames = 0;
static int numCheats = 0;
static int numEnabledCheats = 0;
static int numEnabledCodes = 0;

extern unsigned char _engine_erl_start[];

int killCheats()
{
    int first = 1;
    printf("\n ** Killing Cheats **\n");
    dbCloseDatabase();

    cheatsGame_t *game = gamesHead;
    while(game)
    {
        cheatsGame_t *next = game->next;

        if(game->cheats != NULL) {
            if(first && dbType == BINARY) // Binay DB allocates cheats structs and code lines once.
            {
                free(game->cheats);
                free(game->cheats->codeLines);
                first = 0;
            }

            free(game->cheats->codeLines);
            free(game->cheats);
        }

        game = next;
    }
    
    free(gamesHead);

    return 1;
}

static void populateCheatHashTable()
{
    if(!cheatHashes)
        return;

    cheatsCheat_t *cheat = activeGame->cheats;

    while(cheat != NULL)
    {
        unsigned int hash = hashFunction(cheat->codeLines, cheat->numCodeLines * 8);
        hashAdd(cheatHashes, cheat, hash);

        cheat = cheat->next;
    }
}

int cheatsLoadHistory()
{
    int i;
    FILE *historyFile;
    size_t historyLength;
    unsigned int lastGameHash, cheatHash;
    menuItem_t *lastGameMenu;
    cheatsCheat_t *cheat;

    if(!gameHashes)
        return 0;

    historyFile = fopen("CheatHistory.bin", "rb");

    if(historyFile)
    {
        fseek(historyFile, 0, SEEK_END);
        historyLength = ftell(historyFile);
        fseek(historyFile, 0, SEEK_SET);

        fread(&lastGameHash, 4, 1, historyFile);
        lastGameMenu = (menuItem_t *)hashFind(gameHashes, lastGameHash);

        if(lastGameMenu != NULL)
        {
            cheatsSetActiveGame((cheatsGame_t *) lastGameMenu->extra);
            cheatHashes = hashNewTable(activeGame->numCheats * 1.6);
            populateCheatHashTable();

            for(i = 0; i < historyLength - 4; i+= 4)
            {
                fread(&cheatHash, 4, 1, historyFile);
                cheat = (cheatsCheat_t *)hashFind(cheatHashes, cheatHash);

                if(cheat != NULL)
                    cheatsToggleCheat(cheat);
            }

            menuSetActiveItem(lastGameMenu);

            free(cheatHashes);
        }

        fclose(historyFile);
    }

    free(gameHashes);
    return 1;
}

// CheatDB --> Game --> Cheat --> Code
int cheatsOpenDatabase(const char* path)
{
    const char *ext;
    
    ext = path + strlen(path) - 4;
    
    if(strncmp(ext, ".cdb", 4) == 0)
        dbType = BINARY;
    else if(strncmp(ext, ".txt", 4) == 0)
        dbType = TEXT;
    
    switch(dbType)
    {
        case TEXT:
            numGames = textCheatsOpenFile(path);
            gamesHead = textCheatsGetCheatStruct();
            textCheatsClose();
            break;

        case BINARY:
            numGames = dbOpenDatabase(path);
            gamesHead = dbGetCheatStruct();
            dbCloseDatabase();
            break;

        default:
            break;
    }
    
    printf("loaded %d games\n", numGames);

    return numGames;
}

int cheatsSaveDatabase(const char* path, cheatDatabaseType_t t);

int cheatsLoadGameMenu()
{
    if(gamesHead!=NULL)
    {
        cheatsGame_t *node = gamesHead;
        menuItem_t *items = calloc(numGames, sizeof(menuItem_t));
        menuItem_t *item = items;
        unsigned int hash;

        gameHashes = hashNewTable(numGames * 1.6);
        
        while(node)
        {
            item->type = NORMAL;
            item->text = calloc(1, strlen(node->title) + 1);
            strcpy(item->text, node->title);
            item->extra = node;

            hash = hashFunction(item->text, strlen(item->text));
            hashAdd(gameHashes, item, hash);

            menuAppendItem(item);
            node = node->next;
            item++;
        }

        return 1;
    }

    return 0;
}

cheatsGame_t* cheatsLoadCheatMenu(cheatsGame_t* game)
{
    if(gamesHead!=NULL && game)
    {
        /* Build the menu */
        cheatsCheat_t *cheat = game->cheats;
        menuItem_t *items = calloc(game->numCheats, sizeof(menuItem_t));
        menuItem_t *item = items;
        printf("%d cheats\n", game->numCheats);
        while(cheat != NULL)
        {
            if(!cheat->skip)
            {
                if(cheat->type == CHEATNORMAL)
                    item->type = NORMAL;
                else
                    item->type = HEADER;

                item->text = calloc(1, strlen(cheat->title) + 1);
                strcpy(item->text, cheat->title);

                item->extra = (void *)cheat;

                menuAppendItem(item);
            }
            else
            {
                printf("SKIPPING A CHEAT!!!\n");
            }

            cheat = cheat->next;
            item++;
        }

        numCheats = game->numCheats;
        return game;
    }

    return NULL;
}

int cheatsLoadCodeMenu(const char* game, const char* cheat);

int cheatsAddGame(const char* title)
{
    cheatsGame_t *node = gamesHead;
    cheatsGame_t *newGame = calloc(1, sizeof(cheatsGame_t));
    if(!newGame)
        return 0;

    while(node++->next);

    node->next = newGame;
    strncpy(newGame->title, title, 80);

    return 1;
}

int cheatsAddCheat(const char *title, cheatsCheat_t *cheat);

int cheatsToggleCheat(cheatsCheat_t *cheat)
{
    if(cheat && cheat->type != CHEATHEADER)
    {
        if(!cheat->enabled)
        {
            if((numEnabledCodes + cheat->numCodeLines) >= 250)
            {
                displayError("Too many codes enabled. Try disabling some.");
                return 0;
            }
            
            cheat->enabled = 1;
            numEnabledCheats++;
            numEnabledCodes += cheat->numCodeLines;
        }
        else
        {
            cheat->enabled = 0;
            numEnabledCheats--;
            numEnabledCodes -= cheat->numCodeLines;

            if(numEnabledCheats == 0)
            {
                activeGame = NULL;
            }
        }
    }
    else
        return 0;
    return 1;
}

void cheatsDrawStats()
{
    if(activeGame && activeGame->title)
    {
        if(numEnabledCheats > 0)
        {
            char active_cheats[32];
            if(numEnabledCheats == 1)
                snprintf(active_cheats, 32, "%i active cheat", numEnabledCheats);
            else
                snprintf(active_cheats, 32, "%i active cheats", numEnabledCheats);
            graphicsDrawText(482, 25, active_cheats, WHITE);
        }
    }
    
    static menuID_t activeMenu = -1;
    static int x = 0;
    
    menuID_t oldMenu = activeMenu;
    activeMenu = menuGetActive();
    
    if(activeMenu != oldMenu)
        x = 0;

    if(activeMenu == GAMEMENU)
    {
        if (x < 3000)
            x+= 2;
        else
            x = 0;
        char msg[150];
        snprintf(msg, 150, "%d games available. "
                           "Press X to view a game's cheats. "
                           "Press CIRCLE for main menu. "
                           "Press SELECT to view the credits screen."
                           , numGames);
        graphicsDrawText(640 - x, 405, msg, WHITE);
    }

    if(activeMenu == CHEATMENU)
    {
        if (x < 1700)
            x+= 2;
        else
            x = 0;
        char msg[150];
        snprintf(msg, 150, "%d cheats available. "
                           "Press X to enable or disable a cheat. "
                           "Press CIRCLE to return to the game list."
                           , numCheats);
        graphicsDrawText(640 - x, 405, msg, WHITE);
    }
}

int cheatsIsActiveGame(const cheatsGame_t *game)
{
    return game == activeGame;
}

int cheatsSetActiveGame(cheatsGame_t *game)
{
    /* Disable all active cheats if a new game was selected */
    if(activeGame != NULL && game != activeGame)
    {
        cheatsCheat_t *cheat = activeGame->cheats;
        while(cheat)
        {
            cheat->enabled = 0;
            cheat = cheat->next;
        }

        numEnabledCheats = 0;
        numEnabledCodes = 0;
    }

    activeGame = game;

    return 1;
}

void SetupERL()
{
    struct erl_record_t *erl;

    erl_add_global_symbol("GetSyscallHandler", (u32)GetSyscallHandler);
    erl_add_global_symbol("SetSyscall", (u32)SetSyscall);

    /* Install cheat engine ERL */
    erl = load_erl_from_mem_to_addr(_engine_erl_start, 0x00080000, 0, NULL);
    if(!erl)
    {
        printf("Error loading cheat engine ERL!\n");
        SleepThread();
    }

    erl->flags |= ERL_FLAG_CLEAR;
    FlushCache(0);

    printf("Installed cheat engine ERL. Getting symbols...\n");
    struct symbol_t *sym;
    #define GET_SYMBOL(var, name) \
    sym = erl_find_local_symbol(name, erl); \
    if (sym == NULL) { \
        printf("%s: could not find symbol '%s'\n", __FUNCTION__, name); \
        SleepThread(); \
    } \
    printf("%08x %s\n", (u32)sym->address, name); \
    var = (typeof(var))sym->address

    GET_SYMBOL(get_max_hooks, "get_max_hooks");
    GET_SYMBOL(get_num_hooks, "get_num_hooks");
    GET_SYMBOL(add_hook, "add_hook");
    GET_SYMBOL(clear_hooks, "clear_hooks");
    GET_SYMBOL(get_max_codes, "get_max_codes");
    GET_SYMBOL(set_max_codes, "set_max_codes");
    GET_SYMBOL(get_num_codes, "get_num_codes");
    GET_SYMBOL(add_code, "add_code");
    GET_SYMBOL(clear_codes, "clear_codes");

    printf("Symbols loaded.\n");
}

static void readCodes(cheatsCheat_t *cheats)
{
    int i;
    u32 addr, val;
    int nextCodeCanBeHook = 1;
    cheatsCheat_t *cheat = cheats;

    printf("readCodes(%x)\n", cheats);

    while(cheat)
    {
        if(cheat->enabled && !cheat->skip)
        {
            if(historyFile)
            {
                // Save cheat's hash
                unsigned int cheatHash = hashFunction(cheat->codeLines, cheat->numCodeLines * 8);
                fwrite(&cheatHash, 4, 1, historyFile);
            }

            for(i = 0; i < cheat->numCodeLines; ++i)
            {
                addr = (u32)*((u32 *)cheat->codeLines + 2*i);
                val = (u32)*((u32 *)cheat->codeLines + 2*i + 1);

                if(((addr & 0xfe000000) == 0x90000000) && nextCodeCanBeHook == 1)
                {
                    printf("hook: %08X %08X\n", addr, val);
                    add_hook(addr, val);
                }
                if((addr & 0xf0000000) == 0xf0000000)
                {
                    // ignore F-type enable codes
                    continue;
                }
                else
                {
                    printf("code: %08X %08X\n", addr, val);
                    add_code(addr, val);
                }

                if ((addr & 0xf0000000) == 0x40000000 || (addr & 0xf0000000) == 0x30000000)
                    nextCodeCanBeHook = 0;
                else
                    nextCodeCanBeHook = 1;
            }
        }

        cheat = cheat->next;
    }
}

/* TODO: Only reads one enable cheat for now. */
void readEnableCodes(cheatsCheat_t *enableCheats)
{
    int i;
    u32 addr, val;
    cheatsCheat_t *cheat = enableCheats;

    printf("readEnableCodes(%x)\n", enableCheats);

    for(i = 0; i < cheat->numCodeLines; ++i)
    {
        addr = (u32)*((u32 *)cheat->codeLines + 2*i);
        val = (u32)*((u32 *)cheat->codeLines + 2*i + 1);

        if((addr & 0xfe000000) == 0x90000000)
        {
            printf("hook: %08X %08X\n", addr, val);
            add_hook(addr, val);
        }
    }
}

void cheatsInstallCodesForEngine()
{
    if(activeGame != NULL)
    {
        SetupERL();

        historyFile = fopen("CheatHistory.bin", "wb");
        if(historyFile)
        {
            unsigned int gameHash = hashFunction(activeGame->title, strlen(activeGame->title));
            fwrite(&gameHash, 4, 1, historyFile);
        }

        printf("Reading enable cheats\n");
        readEnableCodes(activeGame->enableCheats);
        printf("Reading cheats\n");
        readCodes(activeGame->cheats);
        printf("Done readin cheats\n");

        if(historyFile)
            fclose(historyFile);
    }
}
