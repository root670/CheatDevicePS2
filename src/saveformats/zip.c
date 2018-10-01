#include "zip.h"
#include "../libraries/minizip/zip.h"
#include "../libraries/minizip/unzip.h"
#include "../graphics.h"
#include "../util.h"
#include <libmc.h>

int extractZIP(gameSave_t *save, device_t dst)
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

int createZIP(gameSave_t *save, device_t src)
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

            if(zipOpenNewFileInZip(zf, strstr(filePath, ":") + 1, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION) == ZIP_OK)
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
