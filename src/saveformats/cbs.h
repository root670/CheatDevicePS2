/*
 * CodeBreaker save file reading and writing
 */

#ifndef SAVEFORMATS_CBS_H
#define SAVEFORMATS_CBS_H

#include "../saves.h"

// Verify file at path is a valid CBS file.
int isCBSFile(const char *path);
// Extract a CBS file to the device dst.
int extractCBS(gameSave_t *save, device_t dst);
// Extract a CBS file from the device src.
int createCBS(gameSave_t *save, device_t src);

#endif
