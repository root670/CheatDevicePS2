#include <stdio.h>
#include "saves.h"

typedef struct saveDevice {
	device_t identifier;
	char *rootPath;
	gameSave_t *saves;
} saveDevice_t;

static char *devicePaths[] = {"mass:\\", "mc0:\\", "mc1:\\"};
static saveDevice_t devices[NUM_SAVE_DEVICES];
static int initialized = 0;

int initSaveMan()
{
	if(!initialized)
	{
		int i;
		for(i = 0; i < NUM_SAVE_DEVICES; i++)
		{
			devices[i].identifier = (device_t)i;
			devices[i].rootPath = devicePaths[i];
			devices[i].saves = NULL;
		}
		
		initialized = 1;
		return 1;
	}
	
	return 0;
}

int killSaveMan()
{
	if(initialized)
	{
		int i;
		for(i = 0; i < NUM_SAVE_DEVICES; i++)
		{
			gameSave_t *save = devices[i].saves;
			if(save)
			{
				gameSave_t *next = save->next;
				free(save);
				save = next;
			}
		}
		initialized = 0;
		return 1;
	}
	
	return 0;
}

gameSave_t *savesGetSaves(device_t dev);
int savesGetAvailibleSlots();

savesCreatePSU(gameSave_t *save);
savesExtractPSU(gameSave_t *save);
