#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fileio.h>
#include <libmc.h>
#include <libpad.h>
#include <sys/stat.h>
#include "saves.h"
#include "menus.h"
#include "graphics.h"
#include "util.h"

static int initialized = 0;
static device_t currentDevice;

typedef struct dirEntry {
	u32 mode;
	u32 length;
	
	struct {
		u8 unknown1;
		u8 sec;      // Entry creation date/time (second)
		u8 min;      // Entry creation date/time (minute)
		u8 hour;     // Entry creation date/time (hour)
		u8 day;      // Entry creation date/time (day)
		u8 month;    // Entry creation date/time (month)
		u16 year;    // Entry creation date/time (year)
	} _create;
	
	u32 cluster;
	u32 dirEntry;

    struct {
		u8 unknown2;
		u8 sec;      // Entry modification date/time (second)
		u8 min;      // Entry modification date/time (minute)
		u8 hour;     // Entry modification date/time (hour)
		u8 day;      // Entry modification date/time (day)
		u8 month;    // Entry modification date/time (month)
		u16 year;    // Entry modification date/time (year)
	} _modify;
	
	u32 attr;
	u8 padding1[0x20 - 4];
	char name[32];
	u8 padding2[0x1A0];
} dirEntry_t;

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

void savesDrawTicker()
{
	static int ticker_x = 0;
	if (ticker_x < 1500)
		ticker_x+= 2;
	else
		ticker_x = 0;
	
	graphicsDrawText(640 - ticker_x, 405, "Press X to choose a game save. Press CIRCLE to return to device menu.", WHITE);
}

static char *getDevicePath(char *str, device_t dev)
{
	char *ret, *mountPath;
	int len;
	
	if(!str)
		return 0;
	
	if (!(dev & (MC_SLOT_1|MC_SLOT_2|FLASH_DRIVE)))
		return 0; // invalid device
	
	if(dev == MC_SLOT_1)
		mountPath = "mc0";
	else if(dev == MC_SLOT_2)
		mountPath = "mc1";
	else if(dev == FLASH_DRIVE)
		mountPath = "mass";
	else
		mountPath = "unk";
	
	len = strlen(str) + 10;
	ret = malloc(len);
	
	if(ret)
		snprintf(ret, len, "%s:%s", mountPath, str);
	
	return ret;
}

gameSave_t *savesGetSaves(device_t dev)
{
	mcTable mcDir[64] __attribute__((aligned(64)));
	gameSave_t *saves;
	gameSave_t *save;
	mcIcon iconSys;
	int ret, fd;
	char iconSysPath[64];
	int first = 1;
	
	saves = calloc(1, sizeof(gameSave_t));
	save = saves;
	
	if(dev == FLASH_DRIVE)
	{
		fio_dirent_t record;
		int fs = fioDopen("mass:");
		
		if(!fs)
			return NULL;
		
		while(fioDread(fs, &record) > 0)
		{
			if(FIO_SO_ISDIR(record.stat.mode))
				continue;
			
			int len = strlen(record.name);
			if(!(tolower(record.name[len-4]) == '.' &&
				 tolower(record.name[len-3]) == 'p' &&
				 tolower(record.name[len-2]) == 's' &&
				 tolower(record.name[len-1]) == 'u'))
			{
				printf("Ignoring file \"%s\"\n", record.name);
				continue;
			}
			
			if(!first)
			{
				gameSave_t *next = calloc(1, sizeof(gameSave_t));
				save->next = next;
				save = next;
				next->next = NULL;
			}
			
			strncpy(save->name, record.name, 100);
			snprintf(save->path, 64, "mass:%s", record.name);
			
			first = 0;
		}
		
		fioDclose(fs);
	}
	
	else
	{
		mcGetDir((dev == MC_SLOT_1) ? 0 : 1, 0, "/*", 0, 54, mcDir);
		mcSync(0, NULL, &ret);
		
		int i;
		for(i = 0; i < ret; i++)
		{
			if(mcDir[i].attrFile & MC_ATTR_SUBDIR)
			{
				char *path = getDevicePath(mcDir[i].name, dev);
				strncpy(save->path, path, 64);
				free(path);
				
				snprintf(iconSysPath, 64, "%s/icon.sys", save->path);
		
				fd = fioOpen(iconSysPath, O_RDONLY);
				if(fd)
				{
					fioRead(fd, &iconSys, sizeof(mcIcon));
					fioClose(fd);
					
					// Attempt crude Shift-JIS -> ASCII conversion...
					char ascii[100];
					int asciiOffset = 0;
					unsigned char *jp = (unsigned char *)iconSys.title;
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
								case 0x5B:
								case 0x5C:
								case 0x5D:
									ascii[asciiOffset++] = '-';
									break;
								case 0x6D:
									ascii[asciiOffset++] = '[';
									break;
								case 0x6E:
									ascii[asciiOffset++] = ']';
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
		
		// Flash drive
		int f = fioDopen("mass:");
		if(f > 0)
		{
			available |= FLASH_DRIVE;
			fioDclose(f);
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
	
	graphicsDrawText(450, 400, "Please wait...", WHITE);
	graphicsRenderNow();
	
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
		item->extra = save;
		menuAppendItem(item);
		
		gameSave_t *next = save->next;
		save = next;
	}
	
	currentDevice = dev;
}

// Create PSU file and save it to a flash drive.
int savesCreatePSU(gameSave_t *save, device_t src)
{
	FILE *psuFile, *mcFile;
	mcTable mcDir[64] __attribute__((aligned(64)));
	dirEntry_t dir, file;
	fio_stat_t stat;
	char mcPath[100];
	char psuPath[100];
	char filePath[150];
	char validName[32];
	char *data;
	int numFiles = 0;
	int i, j, padding;
	int ret;
	float progress = 0.0;
	
	if(!save || !(src & (MC_SLOT_1|MC_SLOT_2)))
		return 0;
	
	memset(&dir, 0, sizeof(dirEntry_t));
	memset(&file, 0, sizeof(dirEntry_t));
	
	replaceIllegalChars(save->name, validName, '-');
	rtrim(validName);
	snprintf(psuPath, 100, "mass:%s.psu", validName);
	
	if(fioGetstat(psuPath, &stat) == 0)
	{
		char *items[] = {"Yes", "No"};
		int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
		
		if(choice == 1)
			return 0;
	}
	
	graphicsDrawLoadingBar(50, 350, 0.0);
	graphicsDrawTextCentered(310, "Copying save...", YELLOW);
	graphicsRenderNow();
	
	psuFile = fopen(psuPath, "wb");
	if(!psuFile)
		return 0;
	
	snprintf(mcPath, 100, "%s/*", strstr(save->path, ":") + 1);
	
	mcGetDir((src == MC_SLOT_1) ? 0 : 1, 0, mcPath, 0, 54, mcDir);
	mcSync(0, NULL, &ret);
	
	// Leave space for 3 directory entries (root, '.', and '..').
	for(i = 0; i < 512*3; i++)
		fputc(0, psuFile);
	
	for(i = 0; i < ret; i++)
	{
		if(mcDir[i].attrFile & MC_ATTR_SUBDIR)
		{
			dir.mode = 0x8427;
			dir._create.year = mcDir[i]._create.year;
			dir._create.month = mcDir[i]._create.month;
			dir._create.day = mcDir[i]._create.day;
			dir._create.hour = mcDir[i]._create.hour;
			dir._create.min = mcDir[i]._create.min;
			dir._create.sec = mcDir[i]._create.sec;
			dir._modify.year = mcDir[i]._modify.year;
			dir._modify.month = mcDir[i]._modify.month;
			dir._modify.day = mcDir[i]._modify.day;
			dir._modify.hour = mcDir[i]._modify.hour;
			dir._modify.min = mcDir[i]._modify.min;
			dir._modify.sec = mcDir[i]._modify.sec;
		}
		
		else if(mcDir[i].attrFile & MC_ATTR_FILE)
		{
			progress += (float)1/(ret-2);
			graphicsDrawLoadingBar(50, 350, progress);
			graphicsRenderNow();
			
			file.mode = mcDir[i].attrFile;
			file.length = mcDir[i].fileSizeByte;
			file._create.year = mcDir[i]._create.year;
			file._create.month = mcDir[i]._create.month;
			file._create.day = mcDir[i]._create.day;
			file._create.hour = mcDir[i]._create.hour;
			file._create.min = mcDir[i]._create.min;
			file._create.sec = mcDir[i]._create.sec;
			file._modify.year = mcDir[i]._modify.year;
			file._modify.month = mcDir[i]._modify.month;
			file._modify.day = mcDir[i]._modify.day;
			file._modify.hour = mcDir[i]._modify.hour;
			file._modify.min = mcDir[i]._modify.min;
			file._modify.sec = mcDir[i]._modify.sec;
			strncpy(file.name, mcDir[i].name, 32);			
			
			snprintf(filePath, 100, "%s/%s", save->path, file.name);
			mcFile = fopen(filePath, "rb");
			data = malloc(file.length);
			fread(data, 1, file.length, mcFile);
			fclose(mcFile);
			
			fwrite(&file, 1, 512, psuFile);
			fwrite(data, 1, file.length, psuFile);
			free(data);
			numFiles++;
			
			padding = 1024 - (file.length % 1024);
			if(padding < 1024)
			{
				for(j = 0; j < padding; j++)
					fputc(0, psuFile);
			}
		}
	}
	
	fseek(psuFile, 0, SEEK_SET);
	dir.length = numFiles + 2;
	strncpy(dir.name, strstr(save->path, ":") + 1, 32);
	fwrite(&dir, 1, 512, psuFile); // root directory
	dir.length = 0;
	strncpy(dir.name, ".", 32);
	fwrite(&dir, 1, 512, psuFile); // .
	strncpy(dir.name, "..", 32);
	fwrite(&dir, 1, 512, psuFile); // ..
	fclose(psuFile);
	
	return 1;
}

// Extract PSU file from a flash drive to a memory card.
int savesExtractPSU(gameSave_t *save, device_t dst)
{
	FILE *psuFile, *dstFile;
	int numFiles, next, i;
	char *dirName;
	char dstName[100];
	u8 *data;
	dirEntry_t entry;
	float progress = 0.0;
	
	if(!save || !(dst & (MC_SLOT_1|MC_SLOT_2)))
		return 0;
	
	psuFile = fopen(save->path, "rb");
	if(!psuFile)
		return 0;
	
	// Read main directory entry
	fread(&entry, 1, 512, psuFile);
	numFiles = entry.length - 2;
	
	dirName = getDevicePath(entry.name, dst);
	int ret = fioMkdir(dirName);
	
	// Prompt user to overwrite save if it already exists
	if(ret == -4)
	{
		char *items[] = {"Yes", "No"};
		int choise = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
		if(choise == 1)
		{
			fclose(psuFile);
			free(dirName);
			return 0;
		}
	}
	
	graphicsDrawLoadingBar(50, 350, 0.0);
	graphicsDrawTextCentered(310, "Copying save...", YELLOW);
	graphicsRenderNow();
	
	// Skip "." and ".."
	fseek(psuFile, 1024, SEEK_CUR);
	
	// Copy each file entry
	for(i = 0; i < numFiles; i++)
	{
		progress += (float)1/numFiles;
		graphicsDrawLoadingBar(50, 350, progress);
		graphicsRenderNow();
		
		fread(&entry, 1, 512, psuFile);
		
		data = malloc(entry.length);
		fread(data, 1, entry.length, psuFile);
		
		snprintf(dstName, 100, "%s/%s", dirName, entry.name);
		dstFile = fopen(dstName, "wb");
		if(!dstFile)
		{
			fclose(psuFile);
			free(dirName);
			free(data);
			return 0;
		}
		fwrite(data, 1, entry.length, dstFile);
		fclose(dstFile);
		free(data);
		
		next = 1024 - (entry.length % 1024);
		if(next < 1024)
			fseek(psuFile, next, SEEK_CUR);
	}

	free(dirName);
	fclose(psuFile);
	
	return 1;
}

static void doCopy(device_t src, device_t dst, gameSave_t *save)
{
	int available;
	
	if(src == dst)
	{
		graphicsDrawTextCentered(320, "Can't copy to the same device", YELLOW);
		graphicsRenderNow();
		sleep(2);
		return;
	}
	
	if((src|dst) == (MC_SLOT_1|MC_SLOT_2))
	{
		graphicsDrawTextCentered(320, "Can't copy between memory cards", YELLOW);
		graphicsRenderNow();
		sleep(2);
		return;
	}
	
	available = savesGetAvailableDevices();
	
	if(!(available & src))
	{
		graphicsDrawTextCentered(320, "Source device is not connected", YELLOW);
		graphicsRenderNow();
		sleep(2);
		return;
	}
	
	if(!(available & dst))
	{
		graphicsDrawTextCentered(320, "Destination device is not connected", YELLOW);
		graphicsRenderNow();
		sleep(2);
		return;
	}
	
	if((src & (MC_SLOT_1|MC_SLOT_2)) && (dst == FLASH_DRIVE))
		savesCreatePSU(save, src);
	else if((src == FLASH_DRIVE) && (dst & (MC_SLOT_1|MC_SLOT_2)))
		savesExtractPSU(save, dst);
}

int savesCopySavePrompt(gameSave_t *save)
{
	struct padButtonStatus padStat;
	u32 old_pad = PAD_CROSS;
	u32 pad_pressed = 0;
	int state;
	int selectedDevice = 0;
	
	do
	{
		state = padGetState(0, 0);
		while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
			state = padGetState(0, 0);
	
		padRead(0, 0, &padStat);
	
		pad_pressed = (0xFFFF ^ padStat.btns) & ~old_pad;
		old_pad = 0xFFFF ^ padStat.btns;
		
		graphicsDrawBackground();
		graphicsDrawDeviceMenu(selectedDevice);
		graphicsDrawTextCentered(150, "Select device to copy save to", WHITE);
		graphicsRender();
		
		if(pad_pressed & PAD_CROSS)
		{
			doCopy(currentDevice, 1 << selectedDevice, save);
			return 1;
		}
		
		else if(pad_pressed & PAD_RIGHT)
		{
			if(selectedDevice >= 2)
				selectedDevice = 0;
			else
				++selectedDevice;
		}

		else if(pad_pressed & PAD_LEFT)
		{
			if (selectedDevice == 0)
				selectedDevice = 2;
			else
				--selectedDevice;
		}
	} while(!(pad_pressed & PAD_CIRCLE));
	
	return 1;
}
