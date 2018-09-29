/*
 * CodeBreaker save file reading and writing
 */

#ifndef SAVEFORMATS_CBS_H
#define SAVEFORMATS_CBS_H

#include "../saves.h"

int extractCBS(gameSave_t *save, device_t dst);
int createCBS(gameSave_t *save, device_t src);

#endif
