#include "psu.h"
#include "../graphics.h"
#include "../util.h"
#include <stdio.h>
#include <libmc.h>

int createPSU(gameSave_t *save, device_t src)
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
    snprintf(psuPath, 100, "%s%s.psu", flashDriveDevice, validName);
    
    if(fioGetstat(psuPath, &stat) == 0)
    {
        char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        
        if(choice == 1)
            return 0;
    }
    
    graphicsDrawLoadingBar(50, 350, 0.0);
    graphicsDrawTextCentered(310, COLOR_YELLOW, "Copying save...");
    graphicsRender();
    
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
            graphicsRender();
            
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

int extractPSU(gameSave_t *save, device_t dst)
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
    graphicsRender();
    
    // Skip "." and ".."
    fseek(psuFile, 1024, SEEK_CUR);
    
    // Copy each file entry
    for(i = 0; i < numFiles; i++)
    {
        progress += (float)1/numFiles;
        graphicsDrawLoadingBar(50, 350, progress);
        graphicsRender();
        
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
