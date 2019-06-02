/*
 * Text Cheats
 * Functions to read and write TXT cheat databases.
 */

#ifndef TEXTCHEATS_H
#define TEXTCHEATS_H

#include "cheats.h"

// Open TXT cheat database. If the loaded database contains new games,
// gamesAdded will point to the first new game and numGamesAdded will be greater
// than zero. Returns 1 if the file was successfully read or doesn't exist,
// otherwise 0.
int textCheatsOpen(const char *path, cheatsGame_t **gamesAdded, unsigned int *numGamesAdded);
// Save internal cheat database to TXT cheat database. Only games and cheats
// added by the user or originally loaded from a read-write database will be
// written. Returns 1 if the file was successfully written, otherwise returns
// zero and an error message is written to error.
int textCheatsSave(const char *path, const cheatsGame_t *games, char *error, int errorLen);

// Open TXT cheat database contained in a ZIP file. If the loaded database
// contains new games, gamesAdded will point to the first new game and
// numGamesAdded will be greater than zero. Returns 1 if the file was
// successfully read or doesn't exist, otherwise 0.
int textCheatsOpenZip(const char *path, cheatsGame_t **gamesAdded, unsigned int *numGamesAdded);
// Save internal cheat database to TXT cheat database contained in a ZIP file.
int textCheatsSaveZip(const char *path, const cheatsGame_t *games, char *error, int errorLen);

#endif
