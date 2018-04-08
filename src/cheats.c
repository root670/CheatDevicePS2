#include "cheats.h"
#include "database.h"
#include "textcheats.h"
#include "menus.h"
#include "graphics.h"
#include "util.h"
#include "hash.h"
#include "objectpool.h"
#include <debug.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <erl.h>
#include <floatlib.h>

static cheatsGame_t *gamesHead = NULL;
static cheatsGame_t *activeGame = NULL;
static hashTable_t *gameHashes = NULL;
static hashTable_t *cheatHashes = NULL;
static FILE *historyFile;
static int numGames = 0;
static int numEnabledCheats = 0;
static int numEnabledCodes = 0;
static u64 lastSelectedCode = 0;

extern unsigned char _engine_erl_start[];

typedef struct cheatDatabaseHandler {
    char name[28]; // cheat database format name
    char extention[4]; // file extention
    
    cheatsGame_t* (*open)(const char *path, unsigned int *numGames);
    int (*save)(const char *path, const cheatsGame_t *games);
} cheatDatabaseHandler_t;

static cheatDatabaseHandler_t CDBHandler = {"Binary Database (.cdb)", "cdb", cdbOpen, cdbSave};
static cheatDatabaseHandler_t TXTHandler = {"Text File (.txt)", "txt", textCheatsOpen, textCheatsSave};

int killCheats()
{
    printf("\n ** Killing Cheats **\n");

    objectPoolKill();
    hashDestroyTable(gameHashes);

    return 1;
}

static void populateGameHashTable()
{
    if(gameHashes != NULL)
        hashDestroyTable(gameHashes);

    gameHashes = hashNewTable(numGames);

    cheatsGame_t *game = gamesHead;
    
    while(game != NULL)
    {
        int titleLen = strlen(game->title);

        if(titleLen > 0)
        {
            unsigned int hash = hashFunction(game->title, titleLen);
            hashAdd(gameHashes, game, hash);
        }

        game = game->next;
    }
}

static void populateCheatHashTable(int numEntries)
{
    if(cheatHashes != NULL)
        hashDestroyTable(cheatHashes);

    cheatHashes = hashNewTable(numEntries);

    cheatsCheat_t *cheat = activeGame->cheats;

    while(cheat != NULL)
    {
        unsigned int hash = hashFunction(activeGame->codeLines + cheat->codeLinesOffset, cheat->numCodeLines * 8);
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
            cheatsGame_t *game =(cheatsGame_t *) lastGameMenu->extra;
            cheatsSetActiveGame(game);
            populateCheatHashTable(game->numCheats);

            for(i = 0; i < historyLength - 4; i+= 4)
            {
                fread(&cheatHash, 4, 1, historyFile);
                cheat = (cheatsCheat_t *)hashFind(cheatHashes, cheatHash);

                if(cheat != NULL)
                    cheatsToggleCheat(cheat);
            }

            menuSetActiveItem(lastGameMenu);
            hashDestroyTable(cheatHashes);
        }

        fclose(historyFile);
    }

    return 1;
}

// Determine cheat database handler by filename.
static cheatDatabaseHandler_t *getCheatDatabaseHandler(const char *path)
{
    const char *extension;
    
    if(!path)
        return NULL;
    
    extension = getFileExtension(path);
    
    if(strcmp(extension, CDBHandler.extention) == 0)
        return &CDBHandler;
    else if(strcmp(extension, TXTHandler.extention) == 0)
        return &TXTHandler;
    else
        return NULL;
}

// CheatDB --> Game --> Cheat --> Code
int cheatsOpenDatabase(const char* path)
{
    cheatDatabaseHandler_t *handler;

    handler = getCheatDatabaseHandler(path);

    if(handler)
    {
        gamesHead = handler->open(path, &numGames);
    }
    else
    {
        char error[255];
        sprintf(error, "Unsupported cheat database filetype: \"%s\"!", getFileExtension(path));
        displayError(error);

        numGames = 0;
    }

    return numGames;
}

int cheatsSaveDatabase(const char* path)
{
    cheatDatabaseHandler_t *handler = getCheatDatabaseHandler(path);

    if(handler)
    {
        return handler->save(path, gamesHead);
    }

    return 0;
}

int cheatsLoadGameMenu()
{
    if(gamesHead!=NULL)
    {
        cheatsGame_t *node = gamesHead;
        menuItem_t *items = calloc(numGames, sizeof(menuItem_t));
        menuItem_t *item = items;
        unsigned int hash;

        gameHashes = hashNewTable(numGames);
        
        while(node)
        {
            item->type = NORMAL;
            item->text = strdup(node->title);
            item->extra = node;

            hash = hashFunction(item->text, strlen(item->text));
            hashAdd(gameHashes, item, hash);

            menuInsertItem(item);
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

        while(cheat != NULL)
        {
            if(cheat->type == CHEAT_NORMAL || cheat->type == CHEAT_ENABLECODE)
                item->type = NORMAL;
            else
                item->type = HEADER;

            item->text = strdup(cheat->title);
            item->extra = (void *)cheat;

            menuInsertItem(item);

            cheat = cheat->next;
            item++;
        }

        return game;
    }

    return NULL;
}

cheatsCheat_t* cheatsLoadCodeMenu(cheatsCheat_t *cheat, cheatsGame_t *game)
{
    if(cheat && game)
    {
        /* Build the menu */
        menuItem_t *items = calloc(cheat->numCodeLines, sizeof(menuItem_t));
        menuItem_t *item = items;

        int i;
        for(i = 0; i < cheat->numCodeLines; i++)
        {
            u64 *code = game->codeLines + cheat->codeLinesOffset + i;
            u32 addr = (u32)*((u32 *)code);
            u32 val  = (u32)*((u32 *)code + 1);

            item->text = malloc(18);
            snprintf(item->text, 18, "%08X %08X", addr, val);

            item->type = NORMAL;
            item->extra = (void *)(game->codeLines + cheat->codeLinesOffset + i);

            menuInsertItem(item);

            item++;
        }

        return cheat;
    }

    return NULL;
}

int cheatsAddGame()
{
    cheatsGame_t *node = gamesHead;
    cheatsGame_t *newGame = objectPoolAllocate(OBJECTPOOLTYPE_GAME);
    if(!newGame)
    {
        displayError("Maximum number of games created.");
        return 0;
    }

    if(displayInputMenu(newGame->title, 80, NULL, "Enter Game Title") == 0)
    {
        objectPoolRelease(OBJECTPOOLTYPE_GAME, newGame);
        return 0;
    }

    if(node == NULL)
    {
        gamesHead = newGame;
    }
    else
    {
        unsigned int hash = hashFunction(newGame->title, strlen(newGame->title));
        if(hashFind(gameHashes, hash))
        {
            displayError("Game title already exists!");
            free(newGame);
            return 0;
        }

        while(node->next)
            node = node->next;

        node->next = newGame;
    }

    numGames++;

    menuItem_t *item = calloc(1, sizeof(menuItem_t));
    item->type = NORMAL;
    item->text = strdup(newGame->title);
    item->extra = newGame;

    menuInsertItem(item);
    menuSetActiveItem(item);

    populateGameHashTable();
    return 1;
}

int cheatsRenameGame()
{
    char title[80];

    if(menuGetActive() != GAMEMENU)
        return 0;

    cheatsGame_t *selectedGame = menuGetActiveItemExtra();

    if(!selectedGame)
        return 0;
    
    if(displayInputMenu(title, 80, selectedGame->title, "Enter Game Title") == 0)
        return 0;

    unsigned int hash = hashFunction(title, strlen(title));
    
    if(hashFind(gameHashes, hash))
    {
        displayError("Game title already exists!");
        return 0;
    }
    else
    {
        strncpy(selectedGame->title, title, 80);
        menuRenameActiveItem(selectedGame->title);

        populateGameHashTable();

        return 1;
    }
}

int cheatsDeleteGame()
{
    if(menuGetActive() != GAMEMENU)
        return 0;

    cheatsGame_t *selectedGame = menuGetActiveItemExtra();
    cheatsDeactivateGame(selectedGame);

    if(selectedGame == gamesHead)
    {
        gamesHead = selectedGame->next;
    }
    else
    {
        cheatsGame_t *game = gamesHead;
        while(game->next != selectedGame)
            game = game->next;
        
        game->next = selectedGame->next;
    }

    cheatsCheat_t *cheat = selectedGame->cheats;
    while(cheat != NULL)
    {
        cheatsCheat_t *next = cheat->next;
        objectPoolRelease(OBJECTPOOLTYPE_CHEAT, cheat);
        cheat = next;
    }

    if(selectedGame->codeLines != NULL)
        free(selectedGame->codeLines);

    objectPoolRelease(OBJECTPOOLTYPE_GAME, selectedGame);
    menuRemoveActiveItem();
    numGames--;
    populateGameHashTable();

    return 1;
}

int cheatsGetNumGames()
{
    return numGames;
}

int cheatsAddCheat()
{
    cheatsGame_t *game = menuGetActiveExtra();
    cheatsCheat_t *newCheat = objectPoolAllocate(OBJECTPOOLTYPE_CHEAT);
    if(!newCheat)
    {
        displayError("Maximum number of cheats created.");
        return 0;
    }

    cheatsCheat_t *node = game->cheats;

    if(displayInputMenu(newCheat->title, 80, NULL, "Enter Cheat Title") == 0)
    {
        objectPoolRelease(OBJECTPOOLTYPE_CHEAT, newCheat);
        return 0;
    }

    if(game->cheats == NULL)
    {
        game->cheats = newCheat;
    }
    else
    {
        cheatsCheat_t *node = game->cheats;

        while(node->next)
            node = node->next;

        node->next = newCheat;
    }

    game->numCheats++;
    newCheat->codeLinesOffset = game->codeLinesUsed;

    menuItem_t *item = calloc(1, sizeof(menuItem_t));
    item->type = NORMAL;
    item->text = strdup(newCheat->title);
    item->extra = newCheat;

    menuInsertItem(item);
    menuSetActiveItem(item);

    node = game->cheats;

    return 1;
}

int cheatsRenameCheat()
{
    char title[80];

    if(menuGetActive() != CHEATMENU)
        return 0;

    cheatsCheat_t *selectedCheat = menuGetActiveItemExtra();

    if(!selectedCheat)
        return 0;
    
    if(displayInputMenu(title, 80, selectedCheat->title, "Enter Cheat Title") == 0)
        return 0;

    strncpy(selectedCheat->title, title, 80);
    menuRenameActiveItem(selectedCheat->title);

    return 1;
}

int cheatsDeleteCheat()
{
    if(menuGetActive() != CHEATMENU)
        return 0;

    cheatsCheat_t *selectedCheat = menuGetActiveItemExtra();
    cheatsGame_t *selectedGame = menuGetActiveExtra();

    if(selectedCheat->enabled)
        cheatsToggleCheat(selectedCheat);

    if(selectedCheat->type == NORMAL)
        selectedGame->numCheats--;

    if(selectedCheat == selectedGame->cheats)
    {
        // Delete first cheat
        selectedGame->cheats = selectedCheat->next;
    }
    else
    {
        cheatsCheat_t *cheat = selectedGame->cheats;
        while(cheat->next != selectedCheat)
            cheat = cheat->next;

        cheat->next = selectedCheat->next;
    }

    cheatsCheat_t *cheat = selectedCheat->next;
    while(cheat != NULL)
    {
        cheat->codeLinesOffset -= selectedCheat->numCodeLines;
        cheat = cheat->next;
    }

    u64 *dst = selectedGame->codeLines + selectedCheat->codeLinesOffset;
    u64 *src = dst + selectedCheat->numCodeLines;
    size_t copyLength = selectedGame->codeLinesUsed - (selectedCheat->codeLinesOffset + selectedCheat->numCodeLines);
    memmove(dst, src, copyLength * sizeof(u64));

    selectedGame->codeLinesUsed -= selectedCheat->numCodeLines;

    objectPoolRelease(OBJECTPOOLTYPE_CHEAT, selectedCheat);
    menuRemoveActiveItem();

    return 1;
}

int cheatsAddCodeLine()
{
    u64 newCode = lastSelectedCode;
    char newCodeLine[18];
    menuItem_t *item;

    if(menuGetActive() != CODEMENU)
        return 0;
    
    if(displayCodeEditMenu(&newCode) == 0)
        return 0;

    cheatsCheat_t *cheat = menuGetActiveExtra();
    cheatsGame_t *game = menuGetExtra(CHEATMENU);
    if(!game)
        return 0;

    if(!game->codeLines)
    {
        game->codeLinesCapacity = 1;
        game->codeLines = calloc(1, sizeof(u64));
    }
    else if (game->codeLinesUsed == game->codeLinesCapacity)
    {
        game->codeLinesCapacity *= 2;
        game->codeLines = realloc(game->codeLines, game->codeLinesCapacity * sizeof(u64));
    }

    cheatsCheat_t *cheatNext = cheat->next;
    while(cheatNext != NULL)
    {
        cheatNext->codeLinesOffset++;
        cheatNext = cheatNext->next;
    }

    // Shift all code lines down 1 line
    u64 *src = game->codeLines + cheat->codeLinesOffset + cheat->numCodeLines;
    u64 *dst = src + 1;
    memmove(dst, src, (game->codeLinesUsed - (cheat->codeLinesOffset + cheat->numCodeLines)) * sizeof(u64));

    *src = newCode;
    lastSelectedCode = newCode;
    cheat->numCodeLines++;
    game->codeLinesUsed++;

    menuRemoveAllItems();
    cheatsLoadCodeMenu(cheat, game);
    menuGoToBottom();

    return 1;
}

int cheatsEditCodeLine()
{
    char newCodeLine[18];

    if(menuGetActive() != CODEMENU)
        return 0;

    u64 *selectedCode = menuGetActiveItemExtra();

    if(!selectedCode)
        return 0;

    lastSelectedCode = *selectedCode;

    if(displayCodeEditMenu(selectedCode) == 0)
        return 0;
    
    u32 addr = (u32)*((u32 *)selectedCode);
    u32 val  = (u32)*((u32 *)selectedCode + 1);
    snprintf(newCodeLine, 18, "%08X %08X", addr, val);
    menuRenameActiveItem(newCodeLine);

    return 1;
}

// Delete currently selected code line
int cheatsDeleteCodeLine()
{
    if(menuGetActive() != CODEMENU)
        return 0;

    cheatsCheat_t *cheat = menuGetActiveExtra();
    cheatsGame_t *game = menuGetExtra(CHEATMENU);
    if(!game)
        return 0;

    cheatsCheat_t *cheatNext = cheat->next;
    while(cheatNext != NULL)
    {
        cheatNext->codeLinesOffset--;
        cheatNext = cheatNext->next;
    }

    // Shift all code lines up 1 line
    u64 *src = game->codeLines + cheat->codeLinesOffset + cheat->numCodeLines;
    u64 *dst = src - 1;
    memmove(dst, src, (game->codeLinesUsed - (cheat->codeLinesOffset + cheat->numCodeLines)) * sizeof(u64));

    cheat->numCodeLines--;
    game->codeLinesUsed--;
    menuRemoveActiveItem();

    return 0;
}

int cheatsGetNumCodeLines()
{
    if(menuGetActive() != CODEMENU)
        return 0;

    cheatsCheat_t *cheat = (cheatsCheat_t *)menuGetActiveExtra();
    if(!cheat)
        return 0;

    return cheat->numCodeLines;
}

int cheatsGetNumCheats()
{
    if(menuGetActive() != CHEATMENU)
        return 0;
    
    cheatsGame_t *game = (cheatsGame_t *)menuGetActiveExtra();
    if(!game)
        return 0;

    return game->numCheats;
}

int cheatsGetNumEnabledCheats()
{
    return numEnabledCheats;
}

int cheatsToggleCheat(cheatsCheat_t *cheat)
{
    if(cheat && cheat->type != CHEAT_HEADER)
    {
        if(!cheat->enabled)
        {
            if((numEnabledCodes + cheat->numCodeLines) >= 250)
            {
                displayError("Too many codes enabled. Try disabling some.");
                return 0;
            }
            else if(cheat->numCodeLines == 0)
            {
                displayError("This cheat doesn't contain any code lines.\nPlease add some on the next screen.");
                menuSetActive(CODEMENU);
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

            if(numEnabledCheats == 1)
            {
                // If Enable Code is enabled, disable it
                cheatsCheat_t *cheat = activeGame->cheats;
                while(cheat)
                {
                    if(cheat->type == CHEAT_ENABLECODE)
                    {
                        printf("Enable Code found: %s\n", cheat->title);
                        cheat->enabled = 0;
                        numEnabledCheats--;
                        numEnabledCodes -= cheat->numCodeLines;
                        break;
                    }

                    cheat = cheat->next;
                }
            }

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
    char msg[200];
    
    menuID_t oldMenu = activeMenu;
    activeMenu = menuGetActive();
    
    if(activeMenu != oldMenu)
        x = 0;

    if(activeMenu == GAMEMENU)
    {
        if (x < 1700)
            x+= 2;
        else
            x = 0;

        snprintf(msg, 200, "{CROSS} Cheat List     "
                           "{SQUARE} Options     "
                           "{CIRCLE} Main Menu     "
                           "{L1}/{R1} Page Up/Down     "
                           "{L2}/{R2} Alphabetical Up/Down");
    }

    else if(activeMenu == CHEATMENU)
    {
        if (x < 1500)
            x+= 2;
        else
            x = 0;

        snprintf(msg, 150, "{CROSS} Enable/Disable Cheat     "
                           "{SQUARE} Options     "
                           "{CIRCLE} Game List    "
                           "{L1}/{R1} Page Up/Down");
    }

    else if(activeMenu == CODEMENU)
    {
        if(x < 1200)
            x += 2;
        else
            x = 0;

        snprintf(msg, 100, "{CROSS} Edit Code Line     "
                           "{SQUARE} Options     "
                           "{CIRCLE} Cheat Menu");
    }

    graphicsDrawText(640 - x, 405, msg, WHITE);
}

int cheatsIsActiveGame(const cheatsGame_t *game)
{
    return game == activeGame;
}

void cheatsDeactivateGame(cheatsGame_t *game)
{
    if(game)
    {
        cheatsCheat_t *cheat = game->cheats;
        while(cheat)
        {
            cheat->enabled = 0;
            cheat = cheat->next;
        }

        if(game == activeGame)
        {
            numEnabledCheats = 0;
            numEnabledCodes = 0;
            activeGame = NULL;
        }
    }
}

int cheatsSetActiveGame(cheatsGame_t *game)
{
    /* Disable all active cheats if a new game was selected */
    if(game != activeGame)
    {
        cheatsDeactivateGame(activeGame);

        /* Enable enable code if one exists. */
        cheatsCheat_t *cheat = game->cheats;
        while(cheat)
        {
            if(cheat->type == CHEAT_ENABLECODE)
            {
                printf("Enable Code found: %s\n", cheat->title);
                cheat->enabled = 1;
                numEnabledCheats++;
                numEnabledCodes += cheat->numCodeLines;
            }

            cheat = cheat->next;
        }

        activeGame = game;
    }

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

    while(cheat)
    {
        if(cheat->enabled)
        {
            if(historyFile)
            {
                // Save cheat's hash
                unsigned int cheatHash = hashFunction(activeGame->codeLines + cheat->codeLinesOffset, cheat->numCodeLines * 8);
                fwrite(&cheatHash, 4, 1, historyFile);
            }

            for(i = 0; i < cheat->numCodeLines; ++i)
            {
                u64 *code = activeGame->codeLines + cheat->codeLinesOffset + i;
                addr = (u32)*((u32 *)code);
                val  = (u32)*((u32 *)code + 1);

                if(((addr & 0xfe000000) == 0x90000000) && nextCodeCanBeHook == 1)
                {
                    printf("hook: %08X %08X\n", addr, val);
                    add_hook(addr, val);
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

        printf("Setting up codes for code engine\n");
        readCodes(activeGame->cheats);
        printf("Done setting up codes\n");

        if(historyFile)
            fclose(historyFile);
    }
}
