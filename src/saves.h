/*
 * Save Manager
 * Backup and restore game saves from a memory card and flash drive.
 */

#ifndef SAVES
#define SAVES

#include <tamtypes.h>

#define NUM_SAVE_DEVICES    3
#define MC_SLOT_1           (1 << 0)
#define MC_SLOT_2           (1 << 1)
#define FLASH_DRIVE         (1 << 2)

typedef u8 device_t;
typedef struct gameSave gameSave_t;

// Create a menu with save titles.
void savesLoadSaveMenu(device_t dev);
// Get list of saves on a memory card or PSU files on a flash drive.
gameSave_t *savesGetSaves(device_t dev);
// Check which devices are present.
int savesGetAvailableDevices();
// Display help text.
void savesDrawTicker();

// Prompt user for destination device, then copy the save.
int savesCopySavePrompt(gameSave_t *save);

#endif
