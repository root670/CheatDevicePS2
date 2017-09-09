/*
 * Text Cheats
 * Functions to read and write TXT cheat databases.
 */

#ifndef TEXTCHEATS_H
#define TEXTCHEATS_H

#include "cheats.h"
#include <stdio.h>

// Open TXT cheat database. Returns number of games in cheat database.
int textCheatsOpenDatabase(const char *path);
// Free cheat file from memory.
int textCheatsClose();
// Get data structure of loaded cheat database. Returns null on error.
cheatsGame_t *textCheatsGetCheatStruct();

#endif
