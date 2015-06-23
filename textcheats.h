#ifndef TEXTCHEATS_H
#define TEXTCHEATS_H
#include "cheats.h"
#include <stdio.h>

// Returns number of games in cheat file.
int textCheatsOpenFile(const char *path);
// Returns number og games in cheat file.
int textCheatsOpenBuffer(unsigned char *buff);
// Get data structure of loaded cheat file. Returns null on error.
cheatsGame_t *textCheatsGetCheatStruct();
// Free cheat file from memory.
int textCheatsClose();

// Determine token type for line.
int getToken(const char *line);
// Find next expected token.
int getNextToken(int tokenCurrent);
// Parse line and process token.
int parseLine(const char *line);
// Determine number of each token type
void countTokens(const char *text, int *numGames, int *numCheats, int *numCodeLines);

#endif
