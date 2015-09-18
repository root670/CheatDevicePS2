#ifndef TEXTCHEATS_H
#define TEXTCHEATS_H
#include "cheats.h"
#include <stdio.h>

// Returns number of games in cheat file.
int textCheatsOpenFile(const char *path);
// Get data structure of loaded cheat file. Returns null on error.
cheatsGame_t *textCheatsGetCheatStruct();
// Free cheat file from memory.
int textCheatsClose();

#endif
