#include <debug.h>
#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <erl.h>
#include <floatlib.h>

#include "cheats.h"
#include "textcheats.h"
#include "menus.h"
#include "graphics.h"
#include "util.h"
#include "hash.h"
#include "objectpool.h"
#include "settings.h"

static cheatsGame_t *gamesHead = NULL;
static cheatsGame_t *activeGame = NULL;
static hashTable_t *gameHashes = NULL; // game title -> cheatGames_t*
static hashTable_t *gameMenuHashes = NULL; // game title -> menuItem_t*
static hashTable_t *cheatHashes = NULL; // code lines -> cheatsGame_t*
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
    
    int (*open)(const char *path, cheatsGame_t **gamesAdded, unsigned int *numGamesAdded);
    int (*save)(const char *path, const cheatsGame_t *games, char *error, int errorLen);
} cheatDatabaseHandler_t;

static cheatDatabaseHandler_t cheatDatabaseHandlers[] = {
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

            if(numHookLines > 0 && (numHookLines == cheat->numCodeLines))
                cheat->type = CHEAT_ENABLECODE;

            cheat = cheat->next;
        }
        game = game->next;
    }
}

static int displayAddGame()
{
    cheatsGame_t *newGame = objectPoolAllocate(OBJECTPOOLTYPE_GAME);
    if(!newGame)
    {
        displayError("Maximum number of games created.");
        return 0;
    }

    if(!displayTextEditMenu(newGame->title, 80, NULL, "Enter Game Title"))
    {
        objectPoolRelease(OBJECTPOOLTYPE_GAME, newGame);
        return 0;
    }

    cheatsGame_t *node = gamesHead;
    if(!node)
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

        // Insert at end of game list
        while(node->next)
            node = node->next;

        node->next = newGame;
    }

    numGames++;

    // Add game to menu
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

static int displayRenameGame()
{
    if(menuGetActive() != MENU_GAMES)
        return 0;

    cheatsGame_t *selectedGame = menuGetActiveItemExtra();
    if(!selectedGame || selectedGame->readOnly)
        return 0;
    
    char title[80];
    if(!displayTextEditMenu(title, 80, selectedGame->title, "Enter Game Title"))
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

static int displayDeleteGame()
{
    if(menuGetActive() != MENU_GAMES)
        return 0;

    cheatsGame_t *selectedGame = menuGetActiveItemExtra();
    if(!selectedGame || selectedGame->readOnly)
        return 0;

    cheatsDeactivateGame(selectedGame);

    if(selectedGame == gamesHead)
    {
        // Deleting first game
        gamesHead = selectedGame->next;
    }
    else
    {
        cheatsGame_t *game = gamesHead;
        while(game->next != selectedGame)
            game = game->next;
        
        game->next = selectedGame->next;
    }

    // Delete game's cheats
    cheatsCheat_t *cheat = selectedGame->cheats;
    while(cheat != NULL)
    {
        cheatsCheat_t *next = cheat->next;
        objectPoolRelease(OBJECTPOOLTYPE_CHEAT, cheat);
        cheat = next;
    }

    // Delete game's code lines
    if(selectedGame->codeLines != NULL)
        free(selectedGame->codeLines);

    objectPoolRelease(OBJECTPOOLTYPE_GAME, selectedGame);
    menuRemoveActiveItem();
    numGames--;
    populateGameHashTable();
    cheatDatabaseDirty = 1;

    return 1;
}

static int displayAddCheat(cheatsGame_t *game)
{
    if(!game)
        return 0;

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

    if(!game->cheats)
    {
        // Add first cheat
        game->cheats = newCheat;
    }
    else
    {
        // Insert at end of list
        cheatsCheat_t *node = game->cheats;
        while(node->next)
            node = node->next;

        node->next = newCheat;
    }

    game->numCheats++;
    newCheat->codeLinesOffset = game->codeLinesUsed;

    // Add cheat to menu
    menuItem_t *item = calloc(1, sizeof(menuItem_t));
    item->type = MENU_ITEM_NORMAL;
    item->text = strdup(newCheat->title);
    item->extra = newCheat;
    menuInsertItem(item);
    menuSetActiveItem(item);
    
    cheatDatabaseDirty = 1;

    return 1;
}

static int displayRenameCheat()
{
    if(menuGetActive() != MENU_CHEATS)
        return 0;

    cheatsCheat_t *selectedCheat = menuGetActiveItemExtra();
    if(!selectedCheat)
        return 0;
    
    char title[80];
    if(displayTextEditMenu(title, 80, selectedCheat->title, "Enter Cheat Title") == 0)
        return 0;

    strncpy(selectedCheat->title, title, 80);
    menuRenameActiveItem(selectedCheat->title);
    cheatDatabaseDirty = 1;

    return 1;
}

static int displayDeleteCheat()
{
    if(menuGetActive() != MENU_CHEATS)
        return 0;

    cheatsCheat_t *selectedCheat = menuGetActiveItemExtra();
    cheatsGame_t *selectedGame = menuGetActiveExtra();

    if(!selectedGame)
        return 0;

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

    // Move all subsequent cheat's code lines into the space occupied by this
    // cheat's code lines.
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

static int displayAddCodeLine(cheatsGame_t *game, cheatsCheat_t *cheat)
{
    if(!game || !cheat)
        return 0;
    
    u64 newCode = lastSelectedCode;
    if(displayNewCodeEditMenu(&newCode) == 0)
        return 0;

    if(!game->codeLines)
    {
        // First code line
        game->codeLinesCapacity = 1;
        game->codeLines = calloc(1, sizeof(u64));
    }
    else if (game->codeLinesUsed == game->codeLinesCapacity)
    {
        // Need to allocate more space to add another code line
        game->codeLinesCapacity *= 2;
        game->codeLines = realloc(game->codeLines, game->codeLinesCapacity * sizeof(u64));
    }

    // Move all subsequent cheat's code lines to make room for the new code
    cheatsCheat_t *cheatNext = cheat->next;
    while(cheatNext)
    {
        cheatNext->codeLinesOffset++;
        cheatNext = cheatNext->next;
    }

    u64 *src = game->codeLines + cheat->codeLinesOffset + cheat->numCodeLines;
    u64 *dst = src + 1;
    memmove(dst, src, (game->codeLinesUsed - (cheat->codeLinesOffset + cheat->numCodeLines)) * sizeof(u64));

    *src = newCode;
    lastSelectedCode = newCode;
    cheat->numCodeLines++;
    game->codeLinesUsed++;

    // Reload code menu
    cheatsLoadCodeMenu(cheat, game);
    menuGoToBottom();
    cheatDatabaseDirty = 1;

    return 1;
}

static int displayEditCodeLine()
{
    if(menuGetActive() != MENU_CODES)
        return 0;

    u64 *selectedCode = menuGetActiveItemExtra();
    if(!selectedCode)
        return 0;

    lastSelectedCode = *selectedCode;

    if(!displayExistingCodeEditMenu(selectedCode))
        return 0;
    
    u32 addr = (u32)*((u32 *)selectedCode);
    u32 val  = (u32)*((u32 *)selectedCode + 1);
    
    char newCodeLine[18];
    snprintf(newCodeLine, 18, "%08X %08X", addr, val);
    menuRenameActiveItem(newCodeLine);
    cheatDatabaseDirty = 1;

    return 1;
}

// Delete currently selected code line
static int displayDeleteCodeLine()
{
    if(menuGetActive() != MENU_CODES)
        return 0;

    cheatsGame_t *game = menuGetExtra(MENU_CHEATS);
    if(!game)
        return 0;

    // Move all subsequent cheat's code lines to fill in the space occupied by
    // this code
    cheatsCheat_t *cheat = menuGetActiveExtra();
    cheatsCheat_t *cheatNext = cheat->next;
    while(cheatNext != NULL)
    {
        cheatNext->codeLinesOffset--;
        cheatNext = cheatNext->next;
    }

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

int cheatsLoadHistory()
{
    if(!gameHashes)
        return 0;

    FILE *historyFile = fopen("CheatHistory.bin", "rb");
    if(!historyFile)
        return 0;

    fseek(historyFile, 0, SEEK_END);
    size_t historyLength = ftell(historyFile);
    fseek(historyFile, 0, SEEK_SET);

    unsigned int lastGameHash;
    fread(&lastGameHash, 4, 1, historyFile);
    menuItem_t *lastGameMenu = (menuItem_t *)hashFind(gameMenuHashes, lastGameHash);

    if(lastGameMenu)
    {
        cheatsGame_t *game = (cheatsGame_t *)lastGameMenu->extra;
        cheatsSetActiveGame(game);
        populateCheatHashTable(game->numCheats);

        int i;
        for(i = 0; i < historyLength - 4; i+= 4)
        {
            unsigned int cheatHash;
            fread(&cheatHash, 4, 1, historyFile);
            cheatsCheat_t *cheat = (cheatsCheat_t *)hashFind(cheatHashes, cheatHash);

            if(!cheat || cheat->type == CHEAT_ENABLECODE)
                continue;
            
            cheatsToggleCheat(cheat);
            menuSetActiveItem(lastGameMenu);
        }

        hashDestroyTable(cheatHashes);
    }

    fclose(historyFile);
    hashDestroyTable(gameMenuHashes);

    return 1;
}

// Determine cheat database handler by filename.
static cheatDatabaseHandler_t *getCheatDatabaseHandler(const char *path)
{   
    if(!path)
        return NULL;

    const char *extension = getFileExtension(path);
    if(!extension)
        return NULL;

    int i = 0;
    for(i = 0; i < (sizeof(cheatDatabaseHandlers) / sizeof(cheatDatabaseHandler_t)); i++)
    {
        if(strcasecmp(extension, cheatDatabaseHandlers[i].extension) == 0)
            return &cheatDatabaseHandlers[i];
    }

    return NULL;
}

// Prompt user to choose a cheat database format that supports writing. Returns
// 1 if a format was chosen and choice was updated to the chosen handler.
static int promptSaveToFormat(cheatDatabaseHandler_t **choice)
{
    cheatDatabaseHandler_t handlersWithSaveSupport[5];
    int numHandlers = 0;

    if(!choice)
        return 0;

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
    int option = displayPromptMenu(items, numHandlers + 1,
        "Save Cheat Database\n"
        "Choose cheat database format");

    free(items);

    if(option >= numHandlers)
        return 0;

    cheatDatabaseHandler_t *handler = &handlersWithSaveSupport[option];

    // Change file extension
    char *basename = getFileBasename(settingsGetReadWriteDatabasePath());
    char newPath[100];
    snprintf(newPath, sizeof(newPath), "%s.%s", basename, handler->extension);
    free(basename);
    settingsSetReadWriteDatabasePath(newPath);

    *choice = handler;

    return 0;
}

// CheatDB --> Game --> Cheat --> Code
int cheatsOpenDatabase(const char* path, int readOnly)
{
    cheatDatabaseHandler_t *handler = getCheatDatabaseHandler(path);
    if(!handler)
    {
        char error[255];
        sprintf(error, "Unsupported cheat database filetype: \"%s\"!", getFileExtension(path));
        displayError(error);

        return 0;
    }

    unsigned int numGamesAdded = 0;
    cheatsGame_t *gamesAdded = NULL;
    
    if(!handler->open(path, &gamesAdded, &numGamesAdded))
        return 0;

    printf("Added %d games from %s\n", numGamesAdded, path);

    if(readOnly)
    {
        // Set read-only flag for all games and cheats that were loaded from
        // this database.
        cheatsGame_t *game = gamesAdded;
        while(game)
        {
            game->readOnly = 1;

            cheatsCheat_t *cheat = game->cheats;
            while(cheat)
            {
                cheat->readOnly = 1;
                cheat = cheat->next;
            }

            game = game->next;
        }
    }

    if(!gamesHead)
    {
        // No games had previously been loaded
        gamesHead = gamesAdded;
    }
    else
    {
        // Append list of games we just loaded to the tail of the existing list.
        cheatsGame_t *game = gamesHead;
        while(game->next)
            game = game->next;

        game->next = gamesAdded;
    }

    numGames += numGamesAdded;

    populateGameHashTable();
    findEnableCodes();

    return 1;
}

int cheatsSaveDatabase()
{
    if(!cheatDatabaseDirty)
        return 1;
    
    const char *path = settingsGetReadWriteDatabasePath();
    if(!path)
    {
        // TODO: Prompt for new location
        path = "CheatDatabase_RW.txt";
        settingsSetReadWriteDatabasePath(path);
    }

    // Confirm user wants to write save changes
    const char *items2[] = {"Yes", "No"};
    char msg[128];
    snprintf(msg, sizeof(msg), 
        "Cheat database has been modified.\n"
        "Save changes to \"%s\"?", path);
    int choice = displayPromptMenu(items2, 2, msg);
    if(choice != 0)
        return 1;

    cheatDatabaseHandler_t *handler = getCheatDatabaseHandler(path);

    if(!handler || !handler->save)
    {
        displayError("Current cheat format doesn't support saving and needs to be saved\n"
                     "to a new format. Please choose a format on the next screen.");
        if(!promptSaveToFormat(&handler))
            return 0;
    }

    char errorMsg[256];
    if(!handler->save(path, gamesHead, errorMsg, sizeof(errorMsg)))
    {
        displayError(errorMsg);
        return 0;
    }

    if(!settingsSave(errorMsg, sizeof(errorMsg)))
    {
        displayError(errorMsg);
        return 0;
    }

    return 0;
}

static void onGameSelected(const menuItem_t *selected)
{
    menuSetActive(MENU_CHEATS);
    if(selected && selected->extra != menuGetActiveExtra())
    {
        menuRemoveAllItems();
        menuSetActiveText(selected->text);
        menuSetActiveExtra(selected->extra);

        cheatsLoadCheatMenu(selected->extra);
    }
}

static void onCheatSelected(const menuItem_t *selected)
{
    cheatsCheat_t *cheat = selected ? (cheatsCheat_t *)selected->extra : NULL;
    if(cheat && cheat->type == CHEAT_NORMAL)
    {
        cheatsSetActiveGame(menuGetActiveExtra());
        cheatsToggleCheat(cheat);
    }
    else if(cheat && cheat->type == CHEAT_VALUE_MAPPED)
    {
        if(cheat->enabled)
        {
            cheatsToggleCheat(cheat);
        }
        else
        {
            cheatsGame_t *game = menuGetActiveExtra();
            cheatsValueMap_t *map = &game->valueMaps[cheat->valueMapIndex];
            menuSetActive(MENU_MAP_VALUES);
            menuSetActiveExtra(cheat);
            cheatsLoadValueMapMenu(map);
        }
    }
}

static void onCodeSelected(const menuItem_t *selected)
{
    if(cheatsGetNumCodeLines() > 0)
        displayEditCodeLine();
    else
        displayAddCodeLine(menuGetExtra(MENU_CHEATS), menuGetActiveExtra());
}

static void onValueMapSelected(const menuItem_t *selected)
{
    cheatsCheat_t *cheat = menuGetActiveExtra();
    cheatsGame_t *game = (cheatsGame_t *)menuGetExtra(MENU_CHEATS);
    cheat->valueMapChoice = (u32)selected->extra;
    cheatsSetActiveGame(game);
    cheatsToggleCheat(cheat);
    menuSetActive(MENU_CHEATS);
}

static void onDisplayGameContextMenu(const menuItem_t *selected)
{
    cheatsGame_t *game = selected ? (cheatsGame_t *)selected->extra : NULL;
    if(numGames == 0 || (game && game->readOnly))
    {
        if(cheatDatabaseDirty)
        {
            const char *items[] = {"Add Game", "Save Database", "Cancel"};
            int ret = displayPromptMenu(items, 3, "Game Options");

            if(ret == 0)
                displayAddGame();
            else if(ret == 1)
                cheatsSaveDatabase();
        }
        else
        {
            const char *items[] = {"Add Game", "Cancel"};
            int ret = displayPromptMenu(items, 2, "Game Options");

            if(ret == 0)
                displayAddGame();
        }
    }
    else
    {
        if(cheatDatabaseDirty)
        {
            const char *items[] = {"Add Game", "Rename Game", "Delete Game", "Save Database", "Cancel"};
            int ret = displayPromptMenu(items, 5, "Game Options");

            if(ret == 0)
                displayAddGame();
            else if(ret == 1)
                displayRenameGame();
            else if(ret == 2)
            {
                const char *items2[] = {"Yes", "No"};
                int choice = displayPromptMenu(items2, 2, "Are you sure you want to delete this game?");

                if(choice == 0)
                    displayDeleteGame();
            }
            else if(ret == 3)
                cheatsSaveDatabase();
        }
        else
        {
            const char *items[] = {"Add Game", "Rename Game", "Delete Game", "Cancel"};
            int ret = displayPromptMenu(items, 4, "Game Options");

            if(ret == 0)
                displayAddGame();
            else if(ret == 1)
                displayRenameGame();
            else if(ret == 2)
            {
                const char *items2[] = {"Yes", "No"};
                int choice = displayPromptMenu(items2, 2, "Are you sure you want to delete this game?");

                if(choice == 0)
                    displayDeleteGame();
            }
        }
    }
}

static void onDisplayCheatContextMenu(const menuItem_t *selected)
{
    int ret;

    cheatsCheat_t *cheat = selected ? (cheatsCheat_t *)selected->extra : NULL;

    if(cheatsGetNumCheats() > 0 && cheat && !cheat->readOnly)
    {
        const char *items[] = {"Add Cheat", "Edit Code Lines", "Rename Cheat", "Delete Cheat", "Cancel"};
        ret = displayPromptMenu(items, 5, "Cheat Options");

        if(ret == 0)
            displayAddCheat(menuGetActiveExtra());
        else if(ret == 1 && (cheat->type == CHEAT_NORMAL || cheat->type == CHEAT_ENABLECODE))
        {
            menuSetActive(MENU_CODES);
            cheatsGame_t *game = (cheatsGame_t *)menuGetExtra(MENU_CHEATS);
            cheatsLoadCodeMenu(cheat, game);
        }
        else if(ret == 2)
            displayRenameCheat();
        else if(ret == 3)
        {
            const char *items2[] = {"Yes", "No"};
            int choice = displayPromptMenu(items2, 2, "Are you sure you want to delete this cheat?");

            if(choice == 0)
                displayDeleteCheat();
        }
    }
    else
    {
        const char *items[] = {"Add Cheat", "Cancel"};
        ret = displayPromptMenu(items, 2, "Cheat Options");

        if(ret == 0)
            displayAddCheat(menuGetActiveExtra());
    }
}

static void onDisplayCodeContextMenu(const menuItem_t *selected)
{
    int ret;

    if(cheatsGetNumCodeLines() > 0)
    {
        const char *items[] = {"Add Line", "Edit Line", "Delete Line", "Cancel"};
        ret = displayPromptMenu(items, 4, "Code Options");

        if(ret == 0)
            displayAddCodeLine(menuGetExtra(MENU_CHEATS), menuGetActiveExtra());
        else if(ret == 1)
            displayEditCodeLine();
        else if(ret == 2)
        {
            const char *items2[] = {"Yes", "No"};
            int choice = displayPromptMenu(items2, 2, "Are you sure you want to delete this code line?");

            if(choice == 0)
                displayDeleteCodeLine();
        }
    }
    else
    {
        const char *items[] = {"Add Line", "Cancel"};
        ret = displayPromptMenu(items, 2, "Code Options");

        if(ret == 0)
            displayAddCodeLine(menuGetExtra(MENU_CHEATS), menuGetActiveExtra());
    }
}

int cheatsLoadGameMenu()
{
    menuSetCallback(MENU_CALLBACK_AFTER_DRAW, cheatsDrawStats);
    menuSetCallback(MENU_CALLBACK_PRESSED_SQUARE, onDisplayGameContextMenu);
    menuSetCallback(MENU_CALLBACK_PRESSED_CROSS, onGameSelected);
    menuSetHelpTickerText(HELP_TICKER_GAMES);

    if(!gamesHead)
        return 0;

    cheatsGame_t *game = gamesHead;
    menuItem_t *items = calloc(numGames, sizeof(menuItem_t));
    menuItem_t *item = items;
    gameMenuHashes = hashNewTable(numGames);
    
    while(game)
    {
        item->type = MENU_ITEM_NORMAL;
        item->text = game->title;
        item->extra = game;
        unsigned int hash = hashFunction(game->title, strlen(game->title));
        hashAdd(gameMenuHashes, item, hash);

        menuInsertItem(item);
        game = game->next;
        item++;
    }

    return 1;
}

void cheatsLoadCheatMenu(cheatsGame_t* game)
{
    if(!gamesHead || !game)
        return;

    /* Build the menu */
    cheatsCheat_t *cheat = game->cheats;
    menuItem_t *items = calloc(game->numCheats, sizeof(menuItem_t));
    menuItem_t *item = items;

    while(cheat != NULL)
    {
        if(cheat->type == CHEAT_NORMAL || cheat->type == CHEAT_ENABLECODE)
            item->type = MENU_ITEM_NORMAL;
        else if(cheat->type == CHEAT_VALUE_MAPPED)
            item->type = MENU_ITEM_HAMBURGER_BUTTON;
        else
            item->type = MENU_ITEM_HEADER;

        item->text = cheat->title;
        item->extra = (void *)cheat;

        menuInsertItem(item);

        cheat = cheat->next;
        item++;
    }

    menuSetCallback(MENU_CALLBACK_AFTER_DRAW, cheatsDrawStats);
    menuSetCallback(MENU_CALLBACK_PRESSED_SQUARE, onDisplayCheatContextMenu);
    menuSetCallback(MENU_CALLBACK_PRESSED_CROSS, onCheatSelected);
    menuSetHelpTickerText(HELP_TICKER_CHEATS);
}

cheatsCheat_t* cheatsLoadCodeMenu(cheatsCheat_t *cheat, cheatsGame_t *game)
{
    if(!cheat || !game)
        return NULL;

    if(menuGetActive() != MENU_CODES)
        return NULL;

    menuRemoveAllItems();
    menuSetActiveText(cheat->title);
    menuSetActiveExtra(cheat);

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

    menuSetCallback(MENU_CALLBACK_AFTER_DRAW, cheatsDrawStats);
    menuSetCallback(MENU_CALLBACK_PRESSED_SQUARE, onDisplayCodeContextMenu);
    menuSetCallback(MENU_CALLBACK_PRESSED_CROSS, onCodeSelected);

    if(cheat->numCodeLines > 0)
        menuSetHelpTickerText(HELP_TICKER_CODES_NORMAL);
    else
        menuSetHelpTickerText(HELP_TICKER_CODES_NONE);

    return cheat;
}

void cheatsLoadValueMapMenu(cheatsValueMap_t *map)
{
    if(!map)
        return;

    if(menuGetActive() != MENU_MAP_VALUES)
        return;

    menuRemoveAllItems();
    menuSetActiveText(map->title);

    // Build the menu
    menuItem_t *items = calloc(map->numEntries, sizeof(menuItem_t));
    menuItem_t *item = items;

    int i;
    for(i = 0; i < map->numEntries; i++)
    {
        char *name = (char *)getNthString(map->keys, i);
        item->text = name;
        item->extra = (void *)i;
        menuInsertItem(item);

        item++;
    }

    menuSetCallback(MENU_CALLBACK_PRESSED_CROSS, onValueMapSelected);
}

cheatsGame_t* cheatsFindGame(const char *name)
{
    if(!name)
        return NULL;

    unsigned int hash = hashFunction(name, strlen(name));
    cheatsGame_t *game = hashFind(gameHashes, hash);

    return game;
}

int cheatsGetNumGames()
{
    return numGames;
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

static void enableAllEnableCodes(cheatsGame_t *game)
{
    if(!game)
        return;

    cheatsCheat_t *cheat = game->cheats;
    while(cheat)
    {
        if(cheat->type == CHEAT_ENABLECODE)
        {
            cheat->enabled = 1;
            numEnabledCheats++;
            numEnabledCodes += cheat->numCodeLines;
        }

        cheat = cheat->next;
    }
}

static void disableAllEnableCodes(cheatsGame_t *game)
{
    if(!game)
        return;

    cheatsCheat_t *cheat = activeGame->cheats;
    while(cheat)
    {
        if(cheat->type == CHEAT_ENABLECODE)
        {
            cheat->enabled = 0;
            numEnabledCheats--;
            numEnabledCodes -= cheat->numCodeLines;
        }

        cheat = cheat->next;
    }
}

int cheatsToggleCheat(cheatsCheat_t *cheat)
{
    if(!cheat || cheat->type == CHEAT_HEADER)
        return 0;

    if(!cheat->enabled)
    {
        if((numEnabledCodes + cheat->numCodeLines) >= 250)
        {
            displayError("Too many codes enabled. Try disabling some.");
            return 0;
        }
        else if(cheat->numCodeLines == 0)
        {
            displayError(
                "This cheat doesn't contain any code lines.\n"
                "Please add some on the next screen.");
            menuSetActive(MENU_CODES);
            cheatsGame_t *game = (cheatsGame_t *)menuGetExtra(MENU_CHEATS);
            cheatsLoadCodeMenu(cheat, game);
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
            disableAllEnableCodes(activeGame);
        }

        if(numEnabledCheats == 0)
            activeGame = NULL;
    }

    return 1;
}

void cheatsDrawStats(const menuItem_t *selected)
{
    if(!activeGame || numEnabledCheats == 0)
        return;

    if(numEnabledCheats > 1)
        graphicsDrawText(482, 25, COLOR_WHITE, "%i active cheats", numEnabledCheats);
    else
        graphicsDrawText(482, 25, COLOR_WHITE, "%i active cheat", numEnabledCheats);
}

int cheatsIsActiveGame(const cheatsGame_t *game)
{
    return game == activeGame;
}

void cheatsDeactivateGame(cheatsGame_t *game)
{
    if(!game)
        return;

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

int cheatsSetActiveGame(cheatsGame_t *game)
{
    if(game == activeGame)
        return 0;

    // Disable all active cheats for previous active game
    cheatsDeactivateGame(activeGame);

    // Enable enable code for the new game if one exists
    enableAllEnableCodes(game);

    activeGame = game;

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
        if(!cheat->enabled)
        {
            cheat = cheat->next;
            continue;
        }

        if(historyFile && cheat->type == CHEAT_NORMAL)
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

            if(cheat->type == CHEAT_VALUE_MAPPED &&
               cheat->valueMapLine == i)
            {
                cheatsValueMap_t *map = &activeGame->valueMaps[cheat->valueMapIndex];
                u32 mapValue = map->values[cheat->valueMapChoice];

                // Clear bits that will be modified
                if(mapValue <= 0xFF)
                    val &= 0xFFFFFF00;
                else if(mapValue <= 0xFFFF)
                    val &= 0xFFFF0000;
                else if(mapValue <= 0xFFFFFF)
                    val &= 0xFF000000;
                else
                    val &= 0;

                val |= map->values[cheat->valueMapChoice];
            }

            if(((addr & 0xfe000000) == 0x90000000) && nextCodeCanBeHook)
            {
                printf("hook: %08X %08X\n", addr, val);
                add_hook(addr, val);
            }
            else
            {
                printf("code: %08X %08X\n", addr, val);
                add_code(addr, val);
            }

            // Prevent second line of a 2-line code from being treated as a hook if it starts with a 9
            if ((addr & 0xf0000000) == 0x40000000 || (addr & 0xf0000000) == 0x30000000)
                nextCodeCanBeHook = 0;
            else
                nextCodeCanBeHook = 1;
        }

        cheat = cheat->next;
    }
}

void cheatsInstallCodesForEngine()
{
    if(!activeGame)
        return;

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
