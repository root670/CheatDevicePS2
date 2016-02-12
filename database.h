/*
 * Binary Database Reader
 * Converts binary cheat database to structures used by the Cheat Manager.
 */
#ifndef DATABASE_H
#define DATABASE_H
#include <tamtypes.h>
#include "cheats.h"

typedef struct dbMasterHeader {
    char magic[4];
    u8 version;
    u16 numTitles;
    char padding[9];
} dbMasterHeader_t;

typedef struct dbCheat {
    char *title;
    u8 numCodeLines;
    u64 *codeEntries;
} dbCheat_t;

// Open and read/decompress the database. Only one database can be open at a time.
int dbOpenDatabase(const char *path);
// Converts database to cheat struct. Returns number of games in database.
int dbOpenBuffer(unsigned char *buff);
// Free database from memory.
int dbCloseDatabase();

// Retrieve data structure from loaded database.
cheatsGame_t *dbGetCheatStruct();

// Load a game's cheats. This is needed if dbGetCheatStruct was called with titlesOnly == 1.
int dbGetCheats(cheatsGame_t *game);

#endif
