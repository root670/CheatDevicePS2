/*
 * Binary Database Reader
 * Functions to read and write CDB cheat databases.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include "cheats.h"

// Open CDB cheat database. Returns number of games in cheat database.
cheatsGame_t* cdbOpen(const char *path, unsigned int *numGames);
// Save internal cheat database to CDB cheat database.
int cdbSave(const char *path);

#endif
