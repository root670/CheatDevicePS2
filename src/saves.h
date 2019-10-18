/*
 * Save Manager
 * Backup and restore game saves from a memory card and flash drive.
 */

#ifndef SAVES_H
#define SAVES_H

#include <tamtypes.h>

extern const char *flashDriveDevice;

#define NUM_SAVE_DEVICES    3
#define MC_SLOT_1           (1 << 0)
#define MC_SLOT_2           (1 << 1)
#define FLASH_DRIVE         (1 << 2)

typedef u8 device_t;
typedef struct gameSave gameSave_t;

typedef struct saveHandler {
    char name[28]; // save format name
    char extention[4]; // file extention
    
    int (*verify)(const char *);
    int (*create)(gameSave_t *, device_t);
    int (*extract)(gameSave_t *, device_t);
} saveHandler_t;

struct gameSave {
    char name[100];
    char path[64];
    saveHandler_t *handler;
    
    struct gameSave *next;
};

// Create a menu with save titles.
void savesLoadSaveMenu(device_t dev);
// Get list of saves on a memory card or PSU files on a flash drive.
gameSave_t *savesGetSaves(device_t dev);
// Check which devices are present.
int savesGetAvailableDevices();

// Convert absolute path to use specific device
char *savesGetDevicePath(char *str, device_t dev);

#endif
