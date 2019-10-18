#include <stdio.h>
#include <libmc.h>

#include "max.h"
#include "util.h"
#include "../graphics.h"
#include "../util.h"
#include "../libraries/lzari.h"

#define  HEADER_MAGIC   "Ps2PowerSave"

typedef struct maxHeader
{
    char    magic[12];
    u32     crc;
    char    dirName[32];
    char    iconSysName[32];
    u32     compressedSize;
    u32     numFiles;

    // This is actually the start of the LZARI stream, but we need it to
    // allocate the buffer.
    u32     decompressedSize;
} maxHeader_t;

typedef struct maxEntry
{
    u32     length;
    char    name[32];
} maxEntry_t;

int createMAX(gameSave_t *save, device_t src)
{
    return 0;
}

static void printMAXHeader(const maxHeader_t *header)
{
    if(!header)
        return;

    printf("Magic            : %.*s\n", sizeof(header->magic), header->magic);
    printf("CRC              : %08X\n", header->crc);
    printf("dirName          : %.*s\n", sizeof(header->dirName), header->dirName);
    printf("iconSysName      : %.*s\n", sizeof(header->iconSysName), header->iconSysName);
    printf("compressedSize   : %u\n", header->compressedSize);
    printf("numFiles         : %u\n", header->numFiles);
    printf("decompressedSize : %u\n", header->decompressedSize);
}

static int roundUp(int i, int j)
{
    return (i + j - 1) / j * j;
}

int isMAXFile(const char *path)
{
    if(!path)
        return 0;

    FILE *f = fopen(path, "rb");
    if(!f)
        return 0;

    // Verify file size
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(len < sizeof(maxHeader_t))
    {
        fclose(f);
        return 0;
    }

    // Verify header
    maxHeader_t header;
    fread(&header, 1, sizeof(maxHeader_t), f);
    fclose(f);

    printMAXHeader(&header);

    return (header.compressedSize > 0) &&
           (header.decompressedSize > 0) &&
           (header.numFiles > 0) &&
           strncmp(header.magic, HEADER_MAGIC, sizeof(header.magic)) == 0 &&
           strlen(header.dirName) > 0 &&
           strlen(header.iconSysName) > 0;
}

int extractMAX(gameSave_t *save, device_t dst)
{
    if(!save || !(dst & (MC_SLOT_1|MC_SLOT_2)))
        return 0;

    isMAXFile(save->path);
    
    FILE *f = fopen(save->path, "rb");
    if(!f)
        return 0;

    maxHeader_t header;
    fread(&header, 1, sizeof(maxHeader_t), f);

    char dirNameTerminated[sizeof(header.dirName) + 1];
    memcpy(dirNameTerminated, header.dirName, sizeof(header.dirName));
    dirNameTerminated[32] = '\0';

    char *dirPath = savesGetDevicePath(dirNameTerminated, dst);

    int ret = fioMkdir(dirPath);
    if(ret == -4)
    {
        // Prompt user to overwrite save if it already exists
        const char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        if(choice == 1)
        {
            free(dirPath);
            fclose(f);
            return 0;
        }
    }

    // Get compressed file entries
    u8 *compressed = malloc(header.compressedSize);

    fseek(f, sizeof(maxHeader_t) - 4, SEEK_SET); // Seek to beginning of LZARI stream.
    ret = fread(compressed, 1, header.compressedSize, f);
    if(ret != header.compressedSize)
    {
        printf("Compressed size: actual=%d, expected=%d\n", ret,
            header.compressedSize);
        free(compressed);
        free(dirPath);
        return 0;
    }

    fclose(f);
    u8 *decompressed = malloc(header.decompressedSize);

    ret = unlzari(compressed, header.compressedSize, decompressed,
        header.decompressedSize);
    // As with other save formats, decompressedSize isn't acccurate.
    if(ret == 0)
    {
        printf("Decompression failed.\n");
        free(decompressed);
        free(compressed);
        free(dirPath);
        return 0;
    }

    free(compressed);

    u32 offset = 0;
    int i;
    for(i = 0; i < (int)header.numFiles; i++)
    {
        float progress = (float)offset / header.decompressedSize;
        drawCopyProgress(progress);

        maxEntry_t entry;
        memcpy(&entry, &decompressed[offset], sizeof(maxEntry_t));
        offset += sizeof(maxEntry_t);
        u8 *entryData = &decompressed[offset];

        char dstName[100];
        snprintf(dstName, sizeof(dstName), "%s/%.*s", dirPath,
            sizeof(entry.name), entry.name);

        FILE *dstFile = fopen(dstName, "wb");
        if(!dstFile)
        {
            free(decompressed);
            free(dirPath);
            return 0;
        }

        fwrite(entryData, 1, entry.length, dstFile);
        fclose(dstFile);

        offset += entry.length;
        offset = roundUp(offset + 8, 16) - 8;
    }

    free(decompressed);
    free(dirPath);
    
    return 1;
}
