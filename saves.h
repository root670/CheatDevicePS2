/*
 * Save Manager
 * Backup and restore game saves from a memory card and flash drive. Saves are
 * backed up to PSU files when stored on a flash drive.
 */

#ifndef SAVES
#define SAVES

typedef enum {FLASH_DRIVE, MC_SLOT_1, MC_SLOT_2, NUM_SAVE_DEVICES} device_t;

typedef struct gameSave {
	char name[100];
	char dir[32];
	
	struct gameSave *next;
} gameSave_t;

int initSaveMan();
int killSaveMan();

// Get list of saves on a memory card or PSU files on a flash drive.
gameSave_t *savesGetSaves(device_t dev);
// Check which memory card slots are populated.
int savesGetAvailibleSlots();

// Create PSU file and save it to a flash drive.
int savesCreatePSU(gameSave_t *save);
// Extract PSU file from a flash drive to a memory card.
int savesExtractPSU(gameSave_t *save);

#endif
