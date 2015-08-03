#include "storage.h"
#include "util.h"
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <debug.h>
#include <fileio.h>
#include <libmc.h>
#include <sbv_patches.h>
#include <string.h>

extern u8  _iomanX_irx_start[];
extern int _iomanX_irx_size;
extern u8  _ps2kbd_irx_start[];
extern int _ps2kbd_irx_size;
extern u8  _usbd_irx_start[];
extern int _usbd_irx_size;
extern u8  _usb_mass_irx_start[];
extern int _usb_mass_irx_size;

typedef struct fileSlot {
	int used;
	FILE* fileP;
	size_t len;
	int hash;
} fileSlot_t;

fileSlot_t fileSlots[128] = {{0}};
static int initialized = 0;

int initStorageMan()
{
	if(!initialized)
	{
		int ret;

		printf("\n ** Initializing Storage Manager **\n");

		SifLoadModule("rom0:MCMAN", 0, NULL);
		SifLoadModule("rom0:MCSERV", 0, NULL);
		SifExecModuleBuffer(_iomanX_irx_start, _iomanX_irx_size, 0, NULL, &ret);
		SifExecModuleBuffer(_usbd_irx_start, _usbd_irx_size, 0, NULL, &ret);
		SifExecModuleBuffer(_usb_mass_irx_start, _usb_mass_irx_size, 0, NULL, &ret);

		mcInit(MC_TYPE_MC);

		initialized = 1;
		return 1;
	}

	return 0;
}

int killStorageMan()
{
	printf("\n ** Killing Storage Manager **\n");
	return 1;
}

static int generatePathHash(const char *path)
{
	if(!path)
		return 0;

	return mycrc32(0, path, strlen(path));
}

// Return value: pointer to slot, or 0 if slot couldn't be found
static fileSlot_t* getSlot(const char *path)
{
	if(!path)
		return 0;

	fileSlot_t *slot = fileSlots;
	int i = 0;
	int hash = generatePathHash(path);

	while(i < 128 && slot->hash != hash)
		slot++; i++;

	if(i < 128)
		return slot;
	else
		return 0;
}

int storageHDDIsPresent();
int storageHDDPartitionPresent();
int storageHDDCreatePartition();

int storageOpenFile(const char* path, const char* mode)
{
	if(initialized)
	{
		fileSlot_t *slot = fileSlots;

		// Find empty slot
		int i = 0;
		while(i < 128 && slot->used == 1)
			slot++; i++;

		slot->fileP = fopen(path, mode);
		if(slot->fileP != NULL)
		{
			fseek(slot->fileP, 0, SEEK_END);
			slot->len = ftell(slot->fileP);
			fseek(slot->fileP, 0, SEEK_SET);
			slot->hash = generatePathHash(path);
			slot->used = 1;

			return 1;
		}
		else
			return 0;
	}
	else
		return 0;
}
int storageCloseFile(const char* path)
{
	if(initialized)
	{
		fileSlot_t *slot = getSlot(path);
		if(slot)
		{
			fclose(slot->fileP);
			slot->fileP = 0;
			slot->used = 0;
			slot->len = 0;
			slot->hash = 0;

			return 1;
		}
		else
			return 0;
	}
	else
		return 0;
}

int storageGetFileSize(const char* path)
{
	if(initialized)
	{
		fileSlot_t *slot = getSlot(path);
		if(slot)
			return slot->len;
		else
			return 0;
	}
	else
		return 0;
}

u8 *storageGetFileContents(const char* path, int *len)
{
	int bytesRead = 0;
	u8 *buff = 0;

	if(initialized)
	{
		fileSlot_t *slot = getSlot(path);
		if(slot)
		{
			buff = malloc(slot->len);
			if(buff)
			{
				bytesRead = (int)fread(buff, 1, slot->len, slot->fileP);
				fseek(slot->fileP, 0, SEEK_SET);
			}
			else
			{
				printf("ERROR: malloc failed!!!!\n");
			}
		}
		else
			printf("slot was not found!\n");
	}
	
	*len = bytesRead;

	return buff;
}
