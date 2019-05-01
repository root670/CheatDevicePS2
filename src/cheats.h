/*
 * Cheats Manager
 * Manages cheats used by Cheat Device.
 */

#ifndef CHEATS_H
#define CHEATS_H

#include <tamtypes.h>

#include "menus.h"

typedef enum {
    CHEAT_NORMAL,      // Cheat with code lines
    CHEAT_HEADER,      // Cheat without code lines
    CHEAT_ENABLECODE,  // Cheat with only 9-type code lines
    CHEAT_VALUE_MAPPED // Cheat with value chosen from value map
} cheatTitleType_t;

// Global memory constraints
#define MAX_GAMES   2000
#define MAX_CHEATS  180000

// Per-game memory constraints
#define MAX_VALUE_MAPS 15

/*
Cheats are stored top-down like this:
Cheat Database --> Game --> Cheat --> Code
| Game 1
  | Cheat 1
  | Cheat 2
    | Code Lines
  | Cheat 3
    | Code Lines
| Game 2
*/

typedef struct cheatsCheat {
    char title[81];
    u8 type:2;
    // If readOnly == 1, cannot modify cheat title or codes lines.
    u8 readOnly:1;
    u8 enabled:1;
    // If type is CHEAT_VALUE_MAPPED, this is an index into game->valueMaps
    u8 valueMapIndex:4; // max value == 15

    // Index of value chosen from value map
    u16 valueMapChoice;
    // Code line to use the chosen value
    u8  valueMapLine;

    u8 numCodeLines;
    u16 codeLinesOffset;

    struct cheatsCheat *next;
} cheatsCheat_t;

// List of values with corresponding keys to use in a cheat of type
// CHEAT_VALUE_MAPPED
typedef struct cheatsValueMap {
    char title[16];
    // Keys for each value seperated by null characters
    // (ex: "Potion\0Elixer\0")
    char *keys;
    // Total memory size allocated by keys
    // (ex: 14)
    int keysLength;
    // Values for each key, corresponding to the nth cstring in keys
    // (ex: {0x00000000, 0x00000001})
    u32 *values;
    // Number of values & keys set
    // (ex: 2)
    u8 numEntries;
} cheatsValueMap_t;

typedef struct cheatsGame {
    char title[81];
    // If readOnly == 1, cannot modify game title or read-only cheats. You can,
    // however, add additional cheats.
    u8 readOnly:1;
    u16 numCheats;
    cheatsCheat_t *cheats;

    u64 *codeLines;
    u32 codeLinesCapacity;
    u32 codeLinesUsed;

    cheatsValueMap_t *valueMaps;
    u8 numValueMaps;

    struct cheatsGame *next;
} cheatsGame_t;

int killCheats();

// Open a cheat database and load cheats.
// If readOnly is neq 0, games loaded from the database will not be modifiable.
int cheatsOpenDatabase(const char* path, int readOnly);
// Save cheat database to a file.
int cheatsSaveDatabase();
// Re-enable previously used cheats
int cheatsLoadHistory();

// Create a menu with game titles.
int cheatsLoadGameMenu();
/* Create a menu with cheat titles. Each entry will have a pointer to
   it's associated cheatsCheat_t object for toggling cheats from the menu.
   Game object is returned so the menu manager can associate the created menu
   with a game. */
void cheatsLoadCheatMenu(cheatsGame_t* game);
// Create a menu with a cheat's code lines
cheatsCheat_t* cheatsLoadCodeMenu(cheatsCheat_t *cheat, cheatsGame_t *game);
// Create a menu with a value-mapped cheat's keys
void cheatsLoadValueMapMenu(cheatsValueMap_t *map);

// Find a game by title. Returns NULL if a game couldn't be found.
cheatsGame_t* cheatsFindGame(const char *title);

// Get number of games in the game list
int cheatsGetNumGames();
// Get number of cheats in the cheat list 
int cheatsGetNumCheats();
// Get number of enabled cheats in the cheat list
int cheatsGetNumEnabledCheats();
// Get number of code lines in the cheat
int cheatsGetNumCodeLines();

// Draw helper text.
void cheatsDrawStats(const menuItem_t *selected);
// Sets a cheat as enabled/disabled
int cheatsToggleCheat(cheatsCheat_t *cheat);
// Check if a game is the active game.
int cheatsIsActiveGame(const cheatsGame_t *game);
// Disable all cheats and deactivate game
void cheatsDeactivateGame(cheatsGame_t *game);
// Set the active game. If any cheats were enabled for the previously active
// game, they will all be disabled.
int cheatsSetActiveGame(cheatsGame_t *game);
// Get title for the active game
char* cheatsGetActiveGameTitle();
// Setup up the cheat engine and load active cheats
void cheatsInstallCodesForEngine();

/* used by cheat engine */
int (*get_max_hooks)(void);
int (*get_num_hooks)(void);
int (*add_hook)(u32 addr, u32 val);
void (*clear_hooks)(void);

int (*get_max_codes)(void);
void (*set_max_codes)(int num);
int (*get_num_codes)(void);
int (*add_code)(u32 addr, u32 val);
void (*clear_codes)(void);

#endif
