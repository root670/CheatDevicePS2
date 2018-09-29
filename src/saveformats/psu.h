/*
 * PSU/EMS Adapter save file reading and writing
 */

#ifndef SAVEFORMATS_PSU_H
#define SAVEFORMATS_PSU_H

#include "../saves.h"

int extractPSU(gameSave_t *save, device_t dst);
int createPSU(gameSave_t *save, device_t src);

#endif
