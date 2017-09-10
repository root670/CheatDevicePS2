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
int textCheatsSave(const char *path);

#endif
