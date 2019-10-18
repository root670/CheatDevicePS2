/*
 * Action Replay MAX save file reading and writing
 */

#ifndef SAVEFORMATS_MAX_H
#define SAVEFORMATS_MAX_H

#include "../saves.h"

// Verify file at path is a valid ARMAX save file.
int isMAXFile(const char *path);
int extractMAX(gameSave_t *save, device_t dst);
int createMAX(gameSave_t *save, device_t src);

#endif
