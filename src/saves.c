#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fileio.h>
#include <libmc.h>
#include <sys/stat.h>

#include "saves.h"
#include "pad.h"
#include "menus.h"
#include "graphics.h"
#include "util.h"
#include "saveformats/cbs.h"
#include "saveformats/psu.h"
#include "saveformats/zip.h"
#include "saveformats/max.h"

static device_t s_currentDevice;
static int s_mc1Free, s_mc2Free;

#ifdef _NO_MASS
const char *flashDriveDevice = "host:";
#else
const char *flashDriveDevice = "mass:";
#endif

static const char *HELP_TICKER_SAVES = \
    "{CROSS} Copy     "
    "{CIRCLE} Device Menu";

static const char *MEMORY_CARD_1_NAME   = "Memory Card (Slot 1)";
static const char *MEMORY_CARD_2_NAME   = "Memory Card (Slot 2)";
static const char *FLASH_DRIVE_NAME     = "Flash Drive";

static const char *SYSTEM_SAVE_NAME_US = "BADATA-SYSTEM";
static const char *SYSTEM_SAVE_NAME_EU = "BEDATA-SYSTEM";
static const char *SYSTEM_SAVE_NAME_JP = "BIDATA-SYSTEM";

static saveHandler_t s_PSUHandler = {
    "EMS Adapter (.psu)", "psu", NULL, createPSU, extractPSU
};
static saveHandler_t s_CBSHandler = {
    "CodeBreaker (.cbs)", "cbs", isCBSFile, createCBS, extractCBS
};
static saveHandler_t s_ZIPHandler = {
    "Zip (.zip)", "zip", NULL, createZIP, extractZIP
};
static saveHandler_t s_MAXHandler = {
    "Action Replay Max (.max)", "max", isMAXFile, createMAX, extractMAX
};

static const char SJIS_REPLACEMENT_LUT[] = \
    " ,.,..:;?!\"*'`*^"
    "-_????????*---/\\"
    "~||--''\"\"()()[]{"
    "}<><>[][][]+-+X?"
    "-==<><>????*'\"CY"
    "$c&%#&*@S*******"
    "*******T><^_'='";

char *savesGetDevicePath(char *str, device_t dev)
{
    char *ret;
    const char *mountPath;
    int len;
    
    if(!str)
        return NULL;
    
    if (!(dev & (MC_SLOT_1|MC_SLOT_2|FLASH_DRIVE)))
        return NULL; // invalid device
    
    if(dev == MC_SLOT_1)
        mountPath = "mc0:";
    else if(dev == MC_SLOT_2)
        mountPath = "mc1:";
    else if(dev == FLASH_DRIVE)
        mountPath = flashDriveDevice;
    else
        mountPath = "unk";
    
    len = strlen(str) + 10;
    ret = malloc(len);
    
    if(ret)
        snprintf(ret, len, "%s%s", mountPath, str);
    
    return ret;
}

static void drawDecorations(const menuItem_t *selected)
{
    char *deviceName;
    int freeSpace; // in cluster size. 1 cluster = 1024 bytes.
    
    switch(s_currentDevice)
    {
        case MC_SLOT_1:
            deviceName = "Memory Card (Slot 1)";
            freeSpace = s_mc1Free;
            break;
        case MC_SLOT_2:
            deviceName = "Memory Card (Slot 2)";
            freeSpace = s_mc2Free;
            break;
        case FLASH_DRIVE:
            deviceName = "Flash Drive";
            freeSpace = 0; // TODO: Get free space from flash drive.
            break;
        default:
            deviceName = "";
            freeSpace = 0;
    }
    
    if(s_currentDevice != FLASH_DRIVE)
        graphicsDrawText(30, 47, COLOR_WHITE, "%d KB free", freeSpace);

    // TODO: Add menuSetTitle() to menu.c to set menu title for active menu.
    graphicsDrawTextCentered(47, COLOR_WHITE, deviceName);
}

// Determine save handler by filename.
static saveHandler_t *getSaveHandler(const char *path)
{
    if(!path)
        return NULL;
    
    const char *extension = getFileExtension(path);
    if(!extension)
        return NULL;
    
    saveHandler_t *handler = NULL;
    if(strcasecmp(extension, s_PSUHandler.extention) == 0)
        handler = &s_PSUHandler;
    else if(strcasecmp(extension, s_CBSHandler.extention) == 0)
        handler = &s_CBSHandler;
    else if(strcasecmp(extension, s_ZIPHandler.extention) == 0)
        handler = &s_ZIPHandler;
    else if(strcasecmp(extension, s_MAXHandler.extention) == 0)
        handler = &s_MAXHandler;
    else
        return NULL; // Unsupported extension

    if(handler->verify)
    {
        // Verify file contents
        if(handler->verify(path))
            return handler;
        else
            return NULL;
    }
    else
        return handler;
}

// Display menu to choose save handler.
static saveHandler_t *promptSaveHandler()
{
    const char *items[] = {
        s_PSUHandler.name,
        s_CBSHandler.name,
        s_ZIPHandler.name
    };
    int choice = displayPromptMenu(items, 3, "Choose save format");
    
    if     (choice == 0)    return &s_PSUHandler;
    else if(choice == 1)    return &s_CBSHandler;
    else if(choice == 2)    return &s_ZIPHandler;
    else                    return NULL;
}

gameSave_t *savesGetSaves(device_t dev)
{
    sceMcTblGetDir mcDir[64] __attribute__((aligned(64)));
    gameSave_t *saves = NULL;
    gameSave_t *save = NULL;
    saveHandler_t *handler;
    mcIcon iconSys;
    int ret, fd;
    char iconSysPath[64];
    
    if(dev == FLASH_DRIVE)
    {
        fio_dirent_t record;
        int fs = fioDopen(flashDriveDevice);
        
        if(!fs)
            return NULL;
        
        while(fioDread(fs, &record) > 0)
        {
            if(FIO_SO_ISDIR(record.stat.mode))
                continue;
            
            handler = getSaveHandler(record.name);
            if(!handler)
            {
                printf("Ignoring file \"%s\"\n", record.name);
                continue;
            }

            if(!saves)
            {
                saves = calloc(1, sizeof(gameSave_t));
                save = saves;
            }
            else
            {
                gameSave_t *next = calloc(1, sizeof(gameSave_t));
                save->next = next;
                save = next;
                next->next = NULL;
            }
            
            save->handler = handler;
            strncpy(save->name, record.name, 100);
            rtrim(save->name);
            snprintf(save->path, 64, "%s%s", flashDriveDevice, record.name);
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
            if(!(mcDir[i].AttrFile & MC_ATTR_SUBDIR))
                continue; // Only check subdirectories

            if (strcmp(mcDir[i].EntryName, SYSTEM_SAVE_NAME_US) == 0 ||
                strcmp(mcDir[i].EntryName, SYSTEM_SAVE_NAME_EU) == 0 ||
                strcmp(mcDir[i].EntryName, SYSTEM_SAVE_NAME_JP) == 0
            )
                continue; // Ignore "Your System Configuration" save

            char *path = savesGetDevicePath(mcDir[i].EntryName, dev);
            snprintf(iconSysPath, 64, "%s/icon.sys", path);
            free(path);
    
            fd = fioOpen(iconSysPath, O_RDONLY);
            if(!fd)
                continue; // invalid save

            if(!saves)
            {
                saves = calloc(1, sizeof(gameSave_t));
                save = saves;
            }
            else
            {
                gameSave_t *next = calloc(1, sizeof(gameSave_t));
                save->next = next;
                save = next;
                next->next = NULL;
            }

            strncpy(save->path, path, 64);

            fioRead(fd, &iconSys, sizeof(mcIcon));
            fioClose(fd);
            
            // Attempt crude Shift-JIS -> ASCII conversion...
            char ascii[100];
            char c;
            int asciiOffset = 0;
            unsigned char *jp = (unsigned char *)iconSys.title;
            int j;
            
            for(j = 0; j < 100; j++)
            {
                if(j > 0 && j == iconSys.nlOffset/2)
                    ascii[asciiOffset++] = ' ';
                
                if(*jp == 0x82)
                {
                    jp++;
                    if(*jp == 0x3F) // spaces
                        c = ' ';
                    else if(*jp >= 0x4F && *jp <= 0x58) // 0-9
                        c = *jp - 0x1F;
                    else if(*jp >= 0x60 && *jp <= 0x79) // A-Z
                        c = *jp - 0x1F;
                    else if(*jp >= 80 && *jp <= 0xA0) // a-z
                        c = *jp - 0x20;
                    else
                        c = '?';
                }
                else if(*jp == 0x81)
                {
                    jp++;
                    if(*jp == 0)
                    {
                        ascii[asciiOffset] = '\0';
                        break;
                    }
                    
                    if(*jp >= 0x40 && *jp <= 0xAC)
                        c = SJIS_REPLACEMENT_LUT[*jp - 0x40];
                    else
                        c = '?';
                }
                else
                    c = '?';
                
                ascii[asciiOffset++] = c;
                jp++;
                
                if(*jp == 0)
                {
                    ascii[asciiOffset] = '\0';
                    break;
                }
            }

            strncpy(save->name, ascii, 100);
            rtrim(save->name);
        }
    }
    
    return saves;
}

int savesGetAvailableDevices()
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
    s_mc1Free = mcFree;

    // Memory card slot 2
    mcGetInfo(1, 0, &mcType, &mcFree, &mcFormat);
    mcSync(0, NULL, &ret);
    if(ret == 0 || ret == -1)
    {
        available |= MC_SLOT_2;
        printf("mem card slot 2 available\n");
    }
    s_mc2Free = mcFree;
    
    // Flash drive
    #ifdef _NO_MASS
    // In PCSX2 host doesn't support dopen, so just assume it's available.
    available |= FLASH_DRIVE;
    #else
    int f = fioDopen(flashDriveDevice);
    if(f > 0)
    {
        available |= FLASH_DRIVE;
        fioDclose(f);
    }
    #endif
    
    return available;
}

static int doCopy(device_t src, device_t dst, gameSave_t *save)
{
    if(src == dst)
    {
        displayError("Can't copy to the same device.");
        return 0;
    }
    
    if((src|dst) == (MC_SLOT_1|MC_SLOT_2))
    {
        displayError("Can't copy between memory cards.");
        return 0;
    }

    int available = savesGetAvailableDevices();

    if(!(available & src))
    {
        displayError("Source device is not connected.");
        return 0;
    }
    
    if(!(available & dst))
    {
        displayError("Destination device is not connected.");
        return 0;
    }
    
    if((src & (MC_SLOT_1|MC_SLOT_2)) && (dst == FLASH_DRIVE))
    {
        save->handler = promptSaveHandler();
        if(save->handler && !save->handler->create(save, src))
            displayError("Error creating save file.");
    }
    else if((src == FLASH_DRIVE) && (dst & (MC_SLOT_1|MC_SLOT_2)))
    {
        if(save->handler && !save->handler->extract(save, dst))
            displayError("Error extracting save file.");
    }
    
    return 1;
}

// Prompt user for destination device, then copy the save.
static void onSaveSelected(const menuItem_t *selected)
{
    if(!selected || !selected->extra)
        return;

    gameSave_t *save  =(gameSave_t *)selected->extra;
    if(!save)
        return;

    int devices[2];
    const char *items[2];
    int numDevices = -1;
    if(s_currentDevice & (MC_SLOT_1|MC_SLOT_2))
    {
        devices[0] = FLASH_DRIVE;
        items[0] = FLASH_DRIVE_NAME;
        numDevices = 1;
    }
    else if(s_currentDevice & FLASH_DRIVE)
    {
        devices[0] = MC_SLOT_1;
        devices[1] = MC_SLOT_2;
        items[0] = MEMORY_CARD_1_NAME;
        items[1] = MEMORY_CARD_2_NAME;
        numDevices = 2;
    }

    char promptText[128];
    snprintf(promptText, sizeof(promptText),
        "\"%s\"\nSelect device to copy save to", save->name);

    int ret = displayPromptMenu(items, numDevices, promptText);

    if(ret >= 0)
        doCopy(s_currentDevice, devices[ret], save);
}

void savesLoadSaveMenu(device_t dev)
{
    int available;
    gameSave_t *saves;
    gameSave_t *save;
    
    s_currentDevice = dev;

    menuSetCallback(MENU_CALLBACK_AFTER_DRAW, drawDecorations);
    menuSetCallback(MENU_CALLBACK_PRESSED_CROSS, onSaveSelected);
    menuSetHelpTickerText(HELP_TICKER_SAVES);
    
    graphicsDrawText(450, 400, COLOR_WHITE, "Please wait...");
    graphicsRender();
    
    available = savesGetAvailableDevices();
    
    if(!(available & dev))
    {
        menuItem_t *item = calloc(1, sizeof(menuItem_t));
        item->type = MENU_ITEM_HEADER;
        item->text = strdup("Unable to access device.\n");
        menuInsertItem(item);
        return;
    }
    
    saves = savesGetSaves(dev);
    save = saves;

    if(!save)
    {
        menuItem_t *item = calloc(1, sizeof(menuItem_t));
        item->type = MENU_ITEM_HEADER;
        item->text = strdup("No saves on this device\n");
        menuInsertItem(item);
        return;
    }
    
    while(save)
    {
        menuItem_t *item = calloc(1, sizeof(menuItem_t));
        item->type = MENU_ITEM_NORMAL;
        item->text = strdup(save->name);
        item->extra = save;
        menuInsertItem(item);
        
        gameSave_t *next = save->next;
        save = next;
    }
}
