/*
 * Text Cheats
 * Functions to read and write TXT cheat databases.
 */

#ifndef TEXTCHEATS_H
#define TEXTCHEATS_H

#include "cheats.h"

// Open TXT cheat database. Returns cheat database structure.
cheatsGame_t* textCheatsOpen(const char *path, unsigned int *numGamesRead);
// Save internal cheat database to TXT cheat database.
int textCheatsSave(const char *path, const cheatsGame_t *games);

// Open TXT cheat database contained in a ZIP file. Returns cheat database structure
cheatsGame_t* textCheatsOpenZip(const char *path, unsigned int *numGamesRead);
// Save internal cheat database to TXT cheat database contained in a ZIP file.
int textCheatsSaveZip(const char *path, const cheatsGame_t *games);

#endif
