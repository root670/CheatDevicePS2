#include <stdio.h>
#include <libmc.h>
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
		printf("\n ** Initializing Save Manager **\n");
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
		printf("\n ** Killing Save Manager **\n");
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
int savesGetAvailibleDevices()
{
	if(initialized)
	{
		int mcType, mcFree, mcFormat, ret;
		int available = 0;
		
		// Memory card slot 1
		mcGetInfo(0, 0, &mcType, &mcFree, &mcFormat);
		mcSync(0, NULL, &ret);
		if(ret == 0 || ret == -1)
		{
			available |= MC_SLOT_1;
			printf("mem card slot 1 available\n");
		}

		// Memory card slot 2
		mcGetInfo(1, 0, &mcType, &mcFree, &mcFormat);
		mcSync(0, NULL, &ret);
		if(ret == 0 || ret == -1)
		{
			available |= MC_SLOT_2;
			printf("mem card slot 2 available\n");
		}
		
		printf("available = %d\n", available);
		return available;
	}
	
	return 0;
}

savesCreatePSU(gameSave_t *save);
savesExtractPSU(gameSave_t *save);
