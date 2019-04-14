/*
 * Text Cheats
 * Functions to read and write TXT cheat databases.
 */

#ifndef TEXTCHEATS_H
#define TEXTCHEATS_H

#include "cheats.h"

// Open TXT cheat database. Returns list of new games added.
cheatsGame_t* textCheatsOpen(const char *path, unsigned int *numGamesAdded);
// Save internal cheat database to TXT cheat database.
int textCheatsSave(const char *path, const cheatsGame_t *games);

// Open TXT cheat database contained in a ZIP file. Returns list of new games added.
cheatsGame_t* textCheatsOpenZip(const char *path, unsigned int *numGamesAdded);
// Save internal cheat database to TXT cheat database contained in a ZIP file.
int textCheatsSaveZip(const char *path, const cheatsGame_t *games);

#endif
