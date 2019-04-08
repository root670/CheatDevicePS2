/*
 * Cheats Manager
 * Manages cheats used by Cheat Device.
 */

#ifndef CHEATS_H
#define CHEATS_H

#include <tamtypes.h>

typedef enum {
    CHEAT_NORMAL,       // Cheat with code lines
    CHEAT_HEADER,       // Cheat without code lines
    CHEAT_ENABLECODE    // Cheat with only 9-type code lines
} cheatTitleType_t;

// Memory constraints
#define MAX_GAMES   2000
#define MAX_CHEATS  180000

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
    u8 type:7;
    u8 enabled:1;

    u8 numCodeLines;
    u16 codeLinesOffset;

    struct cheatsCheat *next;
} cheatsCheat_t;

typedef struct cheatsGame {
    char title[81];
    u16 numCheats;
    u8 readOnly;
    cheatsCheat_t *cheats;

    u64 *codeLines;
    u32 codeLinesCapacity;
    u32 codeLinesUsed;

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
cheatsGame_t* cheatsLoadCheatMenu(cheatsGame_t* game);
// Create a menu with a cheat's code lines
cheatsCheat_t* cheatsLoadCodeMenu(cheatsCheat_t *cheat, cheatsGame_t *game);

// Create a new game and add it to the game list
int cheatsAddGame();
// Rename currently selected game from game list
int cheatsRenameGame();
// Delete currently selected game from game list
int cheatsDeleteGame();
// Get number of games in the game list
int cheatsGetNumGames();

// Add a cheat to a game's cheat list
int cheatsAddCheat();
// Rename currently selected cheat from cheat list
int cheatsRenameCheat();
// Delect currently selected cheat from cheat list
int cheatsDeleteCheat();
// Get number of cheats in the cheat list 
int cheatsGetNumCheats();
// Get number of enabled cheats in the cheat list
int cheatsGetNumEnabledCheats();

// Add a code line to a cheat
int cheatsAddCodeLine();
// Edit currently selected code line
int cheatsEditCodeLine();
// Delete currently selected code line
int cheatsDeleteCodeLine();
// Get number of code lines in the cheat
int cheatsGetNumCodeLines();

// Draw helper text.
void cheatsDrawStats();
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
