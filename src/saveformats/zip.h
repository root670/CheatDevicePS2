/*
 * ZIP save file reading and writing
 */

#ifndef SAVEFORMATS_ZIP_H
#define SAVEFORMATS_ZIP_H

#include "../saves.h"

int extractZIP(gameSave_t *save, device_t dst);
int createZIP(gameSave_t *save, device_t src);

#endif
