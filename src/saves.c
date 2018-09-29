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
#include "zlib.h"
#include "libraries/minizip/zip.h"
#include "libraries/minizip/unzip.h"
#include "saveformats/cbs.h"

static device_t currentDevice;
static int mc1Free, mc2Free;

#ifdef _NO_MASS
const char *flashDriveDevice = "host";
#else
const char *flashDriveDevice = "mass";
#endif

static int extractPSU(gameSave_t *save, device_t dst);
static int createPSU(gameSave_t *save, device_t src);
static int extractZIP(gameSave_t *save, device_t dst);
static int createZIP(gameSave_t *save, device_t src);

static saveHandler_t PSUHandler = {"EMS Adapter (.psu)", "psu", createPSU, extractPSU};
static saveHandler_t CBSHandler = {"CodeBreaker (.cbs)", "cbs", createCBS, extractCBS};
static saveHandler_t ZIPHandler = {"Zip (.zip)", "zip", createZIP, extractZIP};

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
        mountPath = "mc0";
    else if(dev == MC_SLOT_2)
        mountPath = "mc1";
    else if(dev == FLASH_DRIVE)
        mountPath = flashDriveDevice;
    else
        mountPath = "unk";
    
    len = strlen(str) + 10;
    ret = malloc(len);
    
    if(ret)
        snprintf(ret, len, "%s:%s", mountPath, str);
    
    return ret;
}

void savesDrawTicker()
{
    char *deviceName;
    int freeSpace; // in cluster size. 1 cluster = 1024 bytes.
    
    switch(currentDevice)
    {
        case MC_SLOT_1:
            deviceName = "Memory Card (Slot 1)";
            freeSpace = mc1Free;
            break;
        case MC_SLOT_2:
            deviceName = "Memory Card (Slot 2)";
            freeSpace = mc2Free;
            break;
        case FLASH_DRIVE:
            deviceName = "Flash Drive";
            freeSpace = 0; // TODO: Get free space from flash drive.
            break;
        default:
            deviceName = "";
            freeSpace = 0;
    }
    
    graphicsDrawTextCentered(47, COLOR_WHITE, deviceName);
    if(currentDevice != FLASH_DRIVE)
        graphicsDrawText(30, 47, COLOR_WHITE, "%d KB free", freeSpace);
    
    static int ticker_x = 0;
    if (ticker_x < 1000)
        ticker_x+= 2;
    else
        ticker_x = 0;
    
    graphicsDrawText(graphicsGetDisplayWidth() - ticker_x, 405, COLOR_WHITE, "{CROSS} Copy     {CIRCLE} Device Menu");
}

// Determine save handler by filename.
static saveHandler_t *getSaveHandler(const char *path)
{
    const char *extension;
    
    if(!path)
        return NULL;
    
    extension = getFileExtension(path);
    
    if(strcmp(extension, PSUHandler.extention) == 0)
        return &PSUHandler;
    else if(strcmp(extension, CBSHandler.extention) == 0)
        return &CBSHandler;
    else if(strcmp(extension, ZIPHandler.extention) == 0)
        return &ZIPHandler;
    else
        return NULL;
}

// Display menu to choose save handler.
static saveHandler_t *promptSaveHandler()
{
    char *items[] = {PSUHandler.name, CBSHandler.name, ZIPHandler.name};
    int choice = displayPromptMenu(items, 3, "Choose save format");
    
    if(choice == 0)
        return &PSUHandler;
    else if(choice == 1)
        return &CBSHandler;
    else if(choice == 2)
        return &ZIPHandler;
    else
        return NULL;
}

gameSave_t *savesGetSaves(device_t dev)
{
    sceMcTblGetDir mcDir[64] __attribute__((aligned(64)));
    gameSave_t *saves;
    gameSave_t *save;
    saveHandler_t *handler;
    mcIcon iconSys;
    int ret, fd;
    char iconSysPath[64];
    int first = 1;
    
    saves = calloc(1, sizeof(gameSave_t));
    save = saves;
    
    if(dev == FLASH_DRIVE)
    {
        fio_dirent_t record;
        int fs = fioDopen(flashDriveDevice);
        
        if(!fs)
        {
            free(saves);
            return NULL;
        }
        
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
            
            if(!first)
            {
                gameSave_t *next = calloc(1, sizeof(gameSave_t));
                save->next = next;
                save = next;
                next->next = NULL;
            }
            
            save->handler = handler;
            strncpy(save->name, record.name, 100);
            rtrim(save->name);
            snprintf(save->path, 64, "%s:%s", flashDriveDevice, record.name);
            
            first = 0;
        }
        
        if(first) // Didn't find any saves
        {
            free(saves);
            fioDclose(fs);
            return NULL;
        }
        
        fioDclose(fs);
    }
    
    else
    {
        mcGetDir((dev == MC_SLOT_1) ? 0 : 1, 0, "/*", 0, 54, mcDir);
        mcSync(0, NULL, &ret);
        
        if(ret == 0)
        {
            free(saves);
            return NULL;
        }
        
        int i;
        for(i = 0; i < ret; i++)
        {
            if(mcDir[i].AttrFile & MC_ATTR_SUBDIR)
            {
                char *path = savesGetDevicePath(mcDir[i].EntryName, dev);
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
                            {
                                const char replacements[] = {' ', ',', '.', ',', '.', '.', ':', ';', '?', '!', '"', '*', '\'', '`', '*', '^',
                                                             '-', '_', '?', '?', '?', '?', '?', '?', '?', '?', '*', '-', '-', '-', '/', '\\', 
                                                             '~', '|', '|', '-', '-', '\'', '\'', '"', '"', '(', ')', '(', ')', '[', ']', '{', 
                                                             '}', '<', '>', '<', '>', '[', ']', '[', ']', '[', ']', '+', '-', '+', 'X', '?',
                                                             '-', '=', '=', '<', '>', '<', '>', '?', '?', '?', '?', '*', '\'', '"', 'C', 'Y', 
                                                             '$', 'c', '&', '%', '#', '&', '*', '@', 'S', '*', '*', '*', '*', '*', '*', '*', 
                                                             '*', '*', '*', '*', '*', '*', '*', 'T', '>', '<', '^', '_', '='};
                                c = replacements[*jp - 0x40];
                            }
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
    mc1Free = mcFree;

    // Memory card slot 2
    mcGetInfo(1, 0, &mcType, &mcFree, &mcFormat);
    mcSync(0, NULL, &ret);
    if(ret == 0 || ret == -1)
    {
        available |= MC_SLOT_2;
        printf("mem card slot 2 available\n");
    }
    mc2Free = mcFree;
    
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

void savesLoadSaveMenu(device_t dev)
{
    int available;
    gameSave_t *saves;
    gameSave_t *save;
    
    currentDevice = dev;
    
    graphicsDrawText(450, 400, COLOR_WHITE, "Please wait...");
    graphicsRenderNow();
    
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

// Create PSU file and save it to a flash drive.
static int createPSU(gameSave_t *save, device_t src)
{
    FILE *psuFile, *mcFile;
    sceMcTblGetDir mcDir[64] __attribute__((aligned(64)));
    McFsEntry dir, file;
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
    
    memset(&dir, 0, sizeof(McFsEntry));
    memset(&file, 0, sizeof(McFsEntry));
    
    replaceIllegalChars(save->name, validName, '-');
    rtrim(validName);
    snprintf(psuPath, 100, "%s:%s.psu", flashDriveDevice, validName);
    
    if(fioGetstat(psuPath, &stat) == 0)
    {
        char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        
        if(choice == 1)
            return 0;
    }
    
    graphicsDrawLoadingBar(50, 350, 0.0);
    graphicsDrawTextCentered(310, COLOR_YELLOW, "Copying save...");
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
        if(mcDir[i].AttrFile & MC_ATTR_SUBDIR)
        {
            dir.mode = 0x8427;
            memcpy(&dir.created, &mcDir[i]._Create, sizeof(sceMcStDateTime));
            memcpy(&dir.modified, &mcDir[i]._Modify, sizeof(sceMcStDateTime));
        }
        
        else if(mcDir[i].AttrFile & MC_ATTR_FILE)
        {
            progress += (float)1/(ret-2);
            graphicsDrawLoadingBar(50, 350, progress);
            graphicsRenderNow();
            
            file.mode = mcDir[i].AttrFile;
            file.length = mcDir[i].FileSizeByte;
            memcpy(&file.created, &mcDir[i]._Create, sizeof(sceMcStDateTime));
            memcpy(&file.modified, &mcDir[i]._Modify, sizeof(sceMcStDateTime));
            strncpy(file.name, mcDir[i].EntryName, 32);         
            
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
static int extractPSU(gameSave_t *save, device_t dst)
{
    FILE *psuFile, *dstFile;
    int numFiles, next, i;
    char *dirName;
    char dstName[100];
    u8 *data;
    McFsEntry entry;
    float progress = 0.0;
    
    if(!save || !(dst & (MC_SLOT_1|MC_SLOT_2)))
        return 0;
    
    psuFile = fopen(save->path, "rb");
    if(!psuFile)
        return 0;
    
    // Read main directory entry
    fread(&entry, 1, 512, psuFile);
    numFiles = entry.length - 2;
    
    dirName = savesGetDevicePath(entry.name, dst);
    int ret = fioMkdir(dirName);
    
    // Prompt user to overwrite save if it already exists
    if(ret == -4)
    {
        char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        if(choice == 1)
        {
            fclose(psuFile);
            free(dirName);
            return 0;
        }
    }
    
    graphicsDrawLoadingBar(50, 350, 0.0);
    graphicsDrawTextCentered(310, COLOR_YELLOW, "Copying save...");
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

static int extractZIP(gameSave_t *save, device_t dst)
{
    FILE *dstFile;
    unzFile zf;
    unz_file_info fileInfo;
    unz_global_info info;
    char fileName[100];
    char dirNameTemp[100];
    int numFiles;
    char *dirName;
    char dstName[100];
    u8 *data;
    float progress = 0.0;
    
    if(!save || !(dst & (MC_SLOT_1|MC_SLOT_2)))
        return 0;
    
    zf = unzOpen(save->path);
    if(!zf)
        return 0;

    unzGetGlobalInfo(zf, &info);
    numFiles = info.number_entry;

    // Get directory name
    if(unzGoToFirstFile(zf) != UNZ_OK)
    {
        unzClose(zf);
        return 0;
    }

    unzGetCurrentFileInfo(zf, &fileInfo, fileName, 100, NULL, 0, NULL, 0);
    printf("Filename: %s\n", fileName);

    strcpy(dirNameTemp, fileName);

    dirNameTemp[(unsigned int)(strstr(dirNameTemp, "/") - dirNameTemp)] = 0;

    printf("Directory name: %s\n", dirNameTemp);

    dirName = savesGetDevicePath(dirNameTemp, dst);
    int ret = fioMkdir(dirName);
    
    // Prompt user to overwrite save if it already exists
    if(ret == -4)
    {
        char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        if(choice == 1)
        {
            unzClose(zf);
            free(dirName);
            return 0;
        }
    }
    
    graphicsDrawLoadingBar(50, 350, 0.0);
    graphicsDrawTextCentered(310, COLOR_YELLOW, "Copying save...");
    graphicsRenderNow();
    
    // Copy each file entry
    do
    {
        progress += (float)1/numFiles;
        graphicsDrawLoadingBar(50, 350, progress);
        graphicsRenderNow();

        unzGetCurrentFileInfo(zf, &fileInfo, fileName, 100, NULL, 0, NULL, 0);
        
        data = malloc(fileInfo.uncompressed_size);
        unzOpenCurrentFile(zf);
        unzReadCurrentFile(zf, data, fileInfo.uncompressed_size);
        unzCloseCurrentFile(zf);
        
        snprintf(dstName, 100, "%s/%s", dirName, strstr(fileName, "/") + 1);

        printf("Writing %s...", dstName);

        dstFile = fopen(dstName, "wb");
        if(!dstFile)
        {
            printf(" failed!!!\n");
            unzClose(zf);
            free(dirName);
            free(data);
            return 0;
        }
        fwrite(data, 1, fileInfo.uncompressed_size, dstFile);
        fclose(dstFile);
        free(data);

        printf(" done!\n");
    } while(unzGoToNextFile(zf) != UNZ_END_OF_LIST_OF_FILE);

    free(dirName);
    unzClose(zf);
    
    return 1;
}

static int createZIP(gameSave_t *save, device_t src)
{
    FILE *mcFile;
    zipFile zf;
    zip_fileinfo zfi;
    sceMcTblGetDir mcDir[64] __attribute__((aligned(64)));
    fio_stat_t stat;
    char mcPath[100];
    char zipPath[100];
    char filePath[150];
    char validName[32];
    char *data;
    int i;
    int ret;
    float progress = 0.0;
    
    if(!save || !(src & (MC_SLOT_1|MC_SLOT_2)))
        return 0;
    
    replaceIllegalChars(save->name, validName, '-');
    rtrim(validName);
    snprintf(zipPath, 100, "%s:%s.zip", flashDriveDevice, validName);
    
    if(fioGetstat(zipPath, &stat) == 0)
    {
        char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        
        if(choice == 1)
            return 0;
    }
    
    graphicsDrawLoadingBar(50, 350, 0.0);
    graphicsDrawTextCentered(310, COLOR_YELLOW, "Copying save...");
    graphicsRenderNow();
    
    zf = zipOpen(zipPath, APPEND_STATUS_CREATE);
    if(!zf)
        return 0;
    
    snprintf(mcPath, 100, "%s/*", strstr(save->path, ":") + 1);
    
    mcGetDir((src == MC_SLOT_1) ? 0 : 1, 0, mcPath, 0, 54, mcDir);
    mcSync(0, NULL, &ret);
    
    for(i = 0; i < ret; i++)
    {
        if(mcDir[i].AttrFile & MC_ATTR_SUBDIR)
            continue;

        else if(mcDir[i].AttrFile & MC_ATTR_FILE)
        {
            progress += (float)1/(ret-2);
            graphicsDrawLoadingBar(50, 350, progress);
            graphicsRenderNow();

            snprintf(filePath, 100, "%s/%s", save->path, mcDir[i].EntryName);
            
            mcFile = fopen(filePath, "rb");
            data = malloc(mcDir[i].FileSizeByte);
            fread(data, 1, mcDir[i].FileSizeByte, mcFile);
            fclose(mcFile);

            if(zipOpenNewFileInZip(zf, strstr(filePath, ":") + 1, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION) == ZIP_OK)
            {
                zipWriteInFileInZip(zf, data, mcDir[i].FileSizeByte);
                zipCloseFileInZip(zf);
            }
            else
            {
                zipClose(zf, NULL);
                free(data);
                return 0;
            }

            free(data);
        }
    }

    zipClose(zf, NULL);

    return 1;
}

static int doCopy(device_t src, device_t dst, gameSave_t *save)
{
    int available;
    
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
    
    available = savesGetAvailableDevices();
    
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

int savesCopySavePrompt(gameSave_t *save)
{
    u32 pad_pressed;
    int selectedDevice = 0;
    
    do
    {
        padPoll(DELAYTIME_SLOW);
        pad_pressed = padPressed();
        
        graphicsDrawTextCentered(47, COLOR_WHITE, save->name);
        graphicsDrawDeviceMenu(selectedDevice);
        graphicsDrawTextCentered(150, COLOR_WHITE, "Select device to copy save to");
        graphicsRender();
        graphicsDrawBackground();
        menuRender();
        
        if(pad_pressed & PAD_CROSS)
        {
            if(!doCopy(currentDevice, 1 << selectedDevice, save))
                continue;
            else
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
