#include <stdio.h>
#include <libmc.h>
#include "saves.h"
#include "menus.h"

static int initialized = 0;

int initSaveMan()
{
	if(!initialized)
	{
		printf("\n ** Initializing Save Manager **\n");
		
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
		
		initialized = 0;
		return 1;
	}
	
	return 0;
}

gameSave_t *savesGetSaves(device_t dev)
{
	int ret;
	mcIcon iconSys;
	char iconSysPath[64];
	int fd;
	mcTable mcDir[64] __attribute__((aligned(64)));
	gameSave_t *saves;
	gameSave_t *save;
	
	if(dev == FLASH_DRIVE)
		return 0; // not supported yet.
	
	mcGetDir((dev == MC_SLOT_1) ? 0 : 1, 0, "/*", 0, 54, mcDir);
	mcSync(0, NULL, &ret);
	
	saves = calloc(1, sizeof(gameSave_t));
	save = saves;
	
	int i;
	for(i = 0; i < ret; i++)
	{
		if(mcDir[i].attrFile & MC_ATTR_SUBDIR)
		{
			strncpy(save->dir, mcDir[i].name, 32);
			snprintf(iconSysPath, 64, "%s:%s/icon.sys", (dev == MC_SLOT_1) ? "mc0" : (dev == MC_SLOT_2) ? "mc1" : (dev == FLASH_DRIVE) ? "mass0" : "unk0", save->dir);
	
			fd = fioOpen(iconSysPath, O_RDONLY);
			if(fd)
			{
				fioRead(fd, &iconSys, sizeof(mcIcon));
				fioClose(fd);
				
				// Attempt crude Shift-JIS -> ASCII conversion...
				char ascii[100];
				int asciiOffset = 0;
				unsigned char *jp = iconSys.title;
				int j;
				
				for(j = 0; j < 100; j++)
				{
					if(j == iconSys.nlOffset/2)
						ascii[asciiOffset++] = ' ';
					
					if(*jp == 0x82)
					{
						jp++;
						if(*jp >= 0x4F && *jp <= 0x58) // 0-9
							ascii[asciiOffset++] = *jp - 0x1F;
						else if(*jp >= 0x60 && *jp <= 0x79) // A-Z
							ascii[asciiOffset++] = *jp - 0x1F;
						else if(*jp >= 80 && *jp <= 0xA0) // a-z
							ascii[asciiOffset++] = *jp - 0x20;
						else
							ascii[asciiOffset++] = '?';
					}
					
					else if(*jp == 0x81)
					{
						jp++;
						
						if(*jp == 0)
						{
							ascii[asciiOffset++] = '\0';
							break;
						}
						
						switch(*jp)
						{
							case 0x40:
								ascii[asciiOffset++] = ' ';
								break;
							case 0x46:
								ascii[asciiOffset++] = ':';
								break;
							case 0x95:
								ascii[asciiOffset++] = '&';
								break;
							default:
								ascii[asciiOffset++] = '?';
						}
					}
					else
						ascii[asciiOffset++] = '?';
					
					jp++;
					
					if(*jp == 0)
					{
						ascii[asciiOffset] = '\0';
						break;
					}
				}
				
				strncpy(save->name, strdup(ascii), 100);
			}
			else
				continue; // invalid save
			
			if(i != ret - 1)
			{
				gameSave_t *next = calloc(1, sizeof(gameSave_t));
				save->next = next;
				save = next;
			}
			else
				save->next = NULL;
		}
	}
	
	return saves;
}
int savesGetAvailableDevices()
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

void savesLoadSaveMenu(device_t dev)
{
	int available;
	gameSave_t *saves;
	gameSave_t *save;
	
	available = savesGetAvailableDevices();
	
	if(!(available & dev))
	{
		menuItem_t *item = calloc(1, sizeof(menuItem_t));
		item->type = HEADER;
		item->text = strdup("Unable to access device.\n");
		menuAppendItem(item);
		return;
	}
	
	saves = savesGetSaves(dev);
	save = saves;	
	
	while(save)
	{
		menuItem_t *item = calloc(1, sizeof(menuItem_t));
		item->type = NORMAL;
		item->text = strdup(save->name);
		menuAppendItem(item);
		
		gameSave_t *next = save->next;
		free(save);
		save = next;
	}
}

savesCreatePSU(gameSave_t *save);
savesExtractPSU(gameSave_t *save);
