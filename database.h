/*
 * Binary Database Reader
 * Functions to read and write CDB cheat databases.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <tamtypes.h>
#include "cheats.h"

// Open CDB cheat database. Returns number of games in cheat database.
int dbOpenDatabase(const char *path);
// Free database from memory.
int dbClose();
// Get data structure of loaded cheat database. Returns null on error.
cheatsGame_t *dbGetCheatStruct();

#endif
