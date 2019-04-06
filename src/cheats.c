#include "cheats.h"
#include "database.h"
#include "textcheats.h"
#include "menus.h"
#include "graphics.h"
#include "util.h"
#include "hash.h"
#include "objectpool.h"
#include "settings.h"
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
static int cheatDatabaseDirty = 0;

extern unsigned char _engine_erl_start[];

typedef struct cheatDatabaseHandler {
    char name[28]; // cheat database format name
    char extension[4]; // file extension
    
    cheatsGame_t* (*open)(const char *path, unsigned int *numGames);
    int (*save)(const char *path, const cheatsGame_t *games);
} cheatDatabaseHandler_t;

static cheatDatabaseHandler_t cheatDatabaseHandlers[] = {
    {"Binary Database (.cdb)", "cdb", cdbOpen, NULL},
    {"Text File (.txt)", "txt", textCheatsOpen, textCheatsSave},
    {"Text File in ZIP (.zip)", "zip", textCheatsOpenZip, NULL}
};

static const char *HELP_TICKER_GAMES = \
    "{CROSS} Cheat List     "
    "{SQUARE} Options     "
    "{CIRCLE} Main Menu     "
    "{L1}/{R1} Page Up/Down     "
    "{L2}/{R2} Alphabetical Up/Down";

static const char *HELP_TICKER_CHEATS = \
    "{CROSS} Enable/Disable Cheat     "
    "{SQUARE} Options     "
    "{CIRCLE} Game List    "
    "{L1}/{R1} Page Up/Down     "
    "{L2}/{R2} Section Up/Down";

static const char *HELP_TICKER_CODES_NORMAL = \
    "{CROSS} Edit Code Line     "
    "{SQUARE} Options     "
    "{CIRCLE} Cheat Menu";

static const char *HELP_TICKER_CODES_NONE = \
    "{CROSS} Add Code Line     "
    "{SQUARE} Options     "
    "{CIRCLE} Cheat Menu";

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

static void findEnableCodes()
{
    cheatsGame_t *game = gamesHead;
    while(game)
    {
        cheatsCheat_t *cheat = game->cheats;
        while(cheat)
        {
            if(cheat->type == CHEAT_HEADER)
            {
                cheat = cheat->next;
                continue;
            }

            u64 *codeLine = game->codeLines + cheat->codeLinesOffset;
            int line = 0;
            int numHookLines = 0;
            while(line < cheat->numCodeLines)
            {
                u32 addr = codeLine[line];
                if((addr & 0xF0000000) == 0x90000000)
                    numHookLines++;

                line++;
            }

            if(numHookLines == cheat->numCodeLines)
                cheat->type = CHEAT_ENABLECODE;
            else
                cheat->type = CHEAT_NORMAL;

            cheat = cheat->next;
        }
        game = game->next;
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

                if(cheat != NULL && cheat->type != CHEAT_ENABLECODE)
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
    if(!path)
        return NULL;
    
    const char *extension = getFileExtension(path);

    int i = 0;
    for(i = 0; i < (sizeof(cheatDatabaseHandlers) / sizeof(cheatDatabaseHandler_t)); i++)
    {
        if(strcasecmp(extension, cheatDatabaseHandlers[i].extension) == 0)
            return &cheatDatabaseHandlers[i];
    }

    return NULL;
}

// Save cheat database to a chosen format.
static int promptSaveToFormat()
{
    int ret = 1;
    cheatDatabaseHandler_t handlersWithSaveSupport[5];
    int numHandlers = 0;
    int i = 0;
    for(i = 0; i < (sizeof(cheatDatabaseHandlers) / sizeof(cheatDatabaseHandler_t)); i++)
    {
        if(cheatDatabaseHandlers[i].save != NULL)
            memcpy(&handlersWithSaveSupport[numHandlers++], &cheatDatabaseHandlers[i], sizeof(cheatDatabaseHandler_t));
    }

    // Build menu
    const char **items = calloc(numHandlers + 1, sizeof(char *));
    items[numHandlers] = "Cancel";
    for(i = 0; i < numHandlers; i++)
        items[i] = handlersWithSaveSupport[i].name;
    
    // Display menu
    int option = displayPromptMenu(items, numHandlers + 1, "Save Cheat Database\nChoose cheat database format");
    if(option < numHandlers)
    {
        cheatDatabaseHandler_t *handler = &handlersWithSaveSupport[option];
        
        // Change file extension
        char *basename = getFileBasename(settingsGetDatabasePath());
        char newPath[100];
        snprintf(newPath, 100, "%s.%s", basename, handler->extension);
        free(basename);
        settingsSetDatabasePath(newPath);

        ret = handler->save(newPath, gamesHead);
    }

    free(items);
    return ret;
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

    findEnableCodes();
    return numGames;
}

int cheatsSaveDatabase()
{
    if(!cheatDatabaseDirty)
        return 1;
    
    const char *path = settingsGetDatabasePath();
    cheatDatabaseHandler_t *handler = getCheatDatabaseHandler(path);

    if(handler && handler->save)
    {
        return handler->save(path, gamesHead);
    }
    else if((handler && !handler->save) || !handler)
    {
        displayError("Current cheat format doesn't support saving and needs to be saved\n"
                     "to a new format. Please choose a format on the next screen.");
        return promptSaveToFormat();
    }

    return 1;
}

int cheatsLoadGameMenu()
{
    if(gamesHead)
    {
        cheatsGame_t *node = gamesHead;
        menuItem_t *items = calloc(numGames, sizeof(menuItem_t));
        menuItem_t *item = items;
        unsigned int hash;

        gameHashes = hashNewTable(numGames);
        
        while(node)
        {
            item->type = MENU_ITEM_NORMAL;
            item->text = node->title;
            item->extra = node;

            hash = hashFunction(item->text, strlen(item->text));
            hashAdd(gameHashes, item, hash);

            menuInsertItem(item);
            node = node->next;
            item++;
        }

        menuSetDecorationsCB(cheatsDrawStats);
        menuSetHelpTickerText(HELP_TICKER_GAMES);

        return 1;
    }

    return 0;
}

cheatsGame_t* cheatsLoadCheatMenu(cheatsGame_t* game)
{
    if(gamesHead && game)
    {
        /* Build the menu */
        cheatsCheat_t *cheat = game->cheats;
        menuItem_t *items = calloc(game->numCheats, sizeof(menuItem_t));
        menuItem_t *item = items;

        while(cheat != NULL)
        {
            if(cheat->type == CHEAT_NORMAL || cheat->type == CHEAT_ENABLECODE)
                item->type = MENU_ITEM_NORMAL;
            else
                item->type = MENU_ITEM_HEADER;

            item->text = cheat->title;
            item->extra = (void *)cheat;

            menuInsertItem(item);

            cheat = cheat->next;
            item++;
        }

        menuSetDecorationsCB(cheatsDrawStats);
        menuSetHelpTickerText(HELP_TICKER_CHEATS);

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
            u32 addr  = (u32)*((u32 *)code);
            u32 val   = (u32)*((u32 *)code + 1);

            item->text = malloc(18);
            snprintf(item->text, 18, "%08X %08X", addr, val);

            item->type = MENU_ITEM_NORMAL;
            item->extra = (void *)(game->codeLines + cheat->codeLinesOffset + i);

            menuInsertItem(item);

            item++;
        }

        menuSetDecorationsCB(cheatsDrawStats);

        if(cheat->numCodeLines > 0)
            menuSetHelpTickerText(HELP_TICKER_CODES_NORMAL);
        else
            menuSetHelpTickerText(HELP_TICKER_CODES_NONE);

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

    if(displayTextEditMenu(newGame->title, 80, NULL, "Enter Game Title") == 0)
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
    item->type = MENU_ITEM_NORMAL;
    item->text = strdup(newGame->title);
    item->extra = newGame;
    menuInsertItem(item);
    menuSetActiveItem(item);
    populateGameHashTable();
    cheatDatabaseDirty = 1;
    
    return 1;
}

int cheatsRenameGame()
{
    char title[80];

    if(menuGetActive() != MENU_GAMES)
        return 0;

    cheatsGame_t *selectedGame = menuGetActiveItemExtra();

    if(!selectedGame)
        return 0;
    
    if(displayTextEditMenu(title, 80, selectedGame->title, "Enter Game Title") == 0)
        return 0;

    unsigned int hash = hashFunction(title, strlen(title));
    
    if(hashFind(gameHashes, hash))
    {
        displayError("Game title already exists!");
        return 0;
    }

    strncpy(selectedGame->title, title, 80);
    menuRenameActiveItem(selectedGame->title);
    populateGameHashTable();
    cheatDatabaseDirty = 1;

    return 1;
}

int cheatsDeleteGame()
{
    if(menuGetActive() != MENU_GAMES)
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
    cheatDatabaseDirty = 1;

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

    if(displayTextEditMenu(newCheat->title, 80, NULL, "Enter Cheat Title") == 0)
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
    item->type = MENU_ITEM_NORMAL;
    item->text = strdup(newCheat->title);
    item->extra = newCheat;
    menuInsertItem(item);
    menuSetActiveItem(item);
    cheatDatabaseDirty = 1;

    return 1;
}

int cheatsRenameCheat()
{
    char title[80];

    if(menuGetActive() != MENU_CHEATS)
        return 0;

    cheatsCheat_t *selectedCheat = menuGetActiveItemExtra();

    if(!selectedCheat)
        return 0;
    
    if(displayTextEditMenu(title, 80, selectedCheat->title, "Enter Cheat Title") == 0)
        return 0;

    strncpy(selectedCheat->title, title, 80);
    menuRenameActiveItem(selectedCheat->title);
    cheatDatabaseDirty = 1;

    return 1;
}

int cheatsDeleteCheat()
{
    if(menuGetActive() != MENU_CHEATS)
        return 0;

    cheatsCheat_t *selectedCheat = menuGetActiveItemExtra();
    cheatsGame_t *selectedGame = menuGetActiveExtra();

    if(selectedCheat->enabled)
        cheatsToggleCheat(selectedCheat);

    if(selectedCheat->type == CHEAT_NORMAL)
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
    cheatDatabaseDirty = 1;

    return 1;
}

int cheatsAddCodeLine()
{
    u64 newCode = lastSelectedCode;

    if(menuGetActive() != MENU_CODES)
        return 0;
    
    if(displayNewCodeEditMenu(&newCode) == 0)
        return 0;

    cheatsCheat_t *cheat = menuGetActiveExtra();
    cheatsGame_t *game = menuGetExtra(MENU_CHEATS);
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
    cheatDatabaseDirty = 1;

    return 1;
}

int cheatsEditCodeLine()
{
    char newCodeLine[18];

    if(menuGetActive() != MENU_CODES)
        return 0;

    u64 *selectedCode = menuGetActiveItemExtra();

    if(!selectedCode)
        return 0;

    lastSelectedCode = *selectedCode;

    if(displayExistingCodeEditMenu(selectedCode) == 0)
        return 0;
    
    u32 addr = (u32)*((u32 *)selectedCode);
    u32 val  = (u32)*((u32 *)selectedCode + 1);
    snprintf(newCodeLine, 18, "%08X %08X", addr, val);
    menuRenameActiveItem(newCodeLine);
    cheatDatabaseDirty = 1;

    return 1;
}

// Delete currently selected code line
int cheatsDeleteCodeLine()
{
    if(menuGetActive() != MENU_CODES)
        return 0;

    cheatsCheat_t *cheat = menuGetActiveExtra();
    cheatsGame_t *game = menuGetExtra(MENU_CHEATS);
    if(!game)
        return 0;

    cheatsCheat_t *cheatNext = cheat->next;
    while(cheatNext != NULL)
    {
        cheatNext->codeLinesOffset--;
        cheatNext = cheatNext->next;
    }

    // Shift all code lines up 1 line
    u64 *dst = (u64 *)menuGetActiveItemExtra();
    u64 *src = dst + 1;
    size_t length = (game->codeLines + game->codeLinesUsed - src) * sizeof(u64);
    memmove(dst, src, length);

    cheat->numCodeLines--;
    game->codeLinesUsed--;
    menuRemoveActiveItem();
    cheatDatabaseDirty = 1;

    return 0;
}

int cheatsGetNumCodeLines()
{
    if(menuGetActive() != MENU_CODES)
        return 0;

    cheatsCheat_t *cheat = (cheatsCheat_t *)menuGetActiveExtra();
    if(!cheat)
        return 0;

    return cheat->numCodeLines;
}

int cheatsGetNumCheats()
{
    if(menuGetActive() != MENU_CHEATS)
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
                menuSetActive(MENU_CODES);
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
            if(numEnabledCheats == 1)
                graphicsDrawText(482, 25, COLOR_WHITE, "%i active cheat", numEnabledCheats);
            else
                graphicsDrawText(482, 25, COLOR_WHITE, "%i active cheats", numEnabledCheats);
        }
    }
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

char* cheatsGetActiveGameTitle()
{
    if(!activeGame)
        return NULL;

    return activeGame->title;
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
            if(historyFile && cheat->type != CHEAT_ENABLECODE)
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
