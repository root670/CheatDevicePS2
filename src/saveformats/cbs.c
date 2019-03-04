#include <stdio.h>
#include <fileio.h>
#include <libmc.h>
#include <zlib.h>

#include "cbs.h"
#include "util.h"
#include "../graphics.h"
#include "../util.h"

static char *CBS_MAGIC = "CFU\0";

typedef struct cbsHeader {
    char magic[4];
    u32 unk1;
    u32 dataOffset;
    u32 decompressedSize;
    u32 compressedSize;
    char name[32];

    sceMcStDateTime created;
    sceMcStDateTime modified;

    u32 unk2;
    u32 mode;
    char unk3[16];
    char title[72];
    char description[132];
} cbsHeader_t;

typedef struct cbsEntry {
    sceMcStDateTime created;
    sceMcStDateTime modified;

    u32 length;
    u32 mode;
    char unk1[8];
    char name[32];
} cbsEntry_t;

// Source: https://github.com/ps2dev/mymc/blob/master/ps2save.py#L36
static unsigned char cbsPerm[] = {
    0x5f, 0x1f, 0x85, 0x6f, 0x31, 0xaa, 0x3b, 0x18,
    0x21, 0xb9, 0xce, 0x1c, 0x07, 0x4c, 0x9c, 0xb4,
    0x81, 0xb8, 0xef, 0x98, 0x59, 0xae, 0xf9, 0x26,
    0xe3, 0x80, 0xa3, 0x29, 0x2d, 0x73, 0x51, 0x62,
    0x7c, 0x64, 0x46, 0xf4, 0x34, 0x1a, 0xf6, 0xe1,
    0xba, 0x3a, 0x0d, 0x82, 0x79, 0x0a, 0x5c, 0x16,
    0x71, 0x49, 0x8e, 0xac, 0x8c, 0x9f, 0x35, 0x19,
    0x45, 0x94, 0x3f, 0x56, 0x0c, 0x91, 0x00, 0x0b,
    0xd7, 0xb0, 0xdd, 0x39, 0x66, 0xa1, 0x76, 0x52,
    0x13, 0x57, 0xf3, 0xbb, 0x4e, 0xe5, 0xdc, 0xf0,
    0x65, 0x84, 0xb2, 0xd6, 0xdf, 0x15, 0x3c, 0x63,
    0x1d, 0x89, 0x14, 0xbd, 0xd2, 0x36, 0xfe, 0xb1,
    0xca, 0x8b, 0xa4, 0xc6, 0x9e, 0x67, 0x47, 0x37,
    0x42, 0x6d, 0x6a, 0x03, 0x92, 0x70, 0x05, 0x7d,
    0x96, 0x2f, 0x40, 0x90, 0xc4, 0xf1, 0x3e, 0x3d,
    0x01, 0xf7, 0x68, 0x1e, 0xc3, 0xfc, 0x72, 0xb5,
    0x54, 0xcf, 0xe7, 0x41, 0xe4, 0x4d, 0x83, 0x55,
    0x12, 0x22, 0x09, 0x78, 0xfa, 0xde, 0xa7, 0x06,
    0x08, 0x23, 0xbf, 0x0f, 0xcc, 0xc1, 0x97, 0x61,
    0xc5, 0x4a, 0xe6, 0xa0, 0x11, 0xc2, 0xea, 0x74,
    0x02, 0x87, 0xd5, 0xd1, 0x9d, 0xb7, 0x7e, 0x38,
    0x60, 0x53, 0x95, 0x8d, 0x25, 0x77, 0x10, 0x5e,
    0x9b, 0x7f, 0xd8, 0x6e, 0xda, 0xa2, 0x2e, 0x20,
    0x4f, 0xcd, 0x8f, 0xcb, 0xbe, 0x5a, 0xe0, 0xed,
    0x2c, 0x9a, 0xd4, 0xe2, 0xaf, 0xd0, 0xa9, 0xe8,
    0xad, 0x7a, 0xbc, 0xa8, 0xf2, 0xee, 0xeb, 0xf5,
    0xa6, 0x99, 0x28, 0x24, 0x6c, 0x2b, 0x75, 0x5d,
    0xf8, 0xd3, 0x86, 0x17, 0xfb, 0xc0, 0x7b, 0xb3,
    0x58, 0xdb, 0xc7, 0x4b, 0xff, 0x04, 0x50, 0xe9,
    0x88, 0x69, 0xc9, 0x2a, 0xab, 0xfd, 0x5b, 0x1b,
    0x8a, 0xd9, 0xec, 0x27, 0x44, 0x0e, 0x33, 0xc8,
    0x6b, 0x93, 0x32, 0x48, 0xb6, 0x30, 0x43, 0xa5
};

static void rc4Crypt(unsigned char *buf, size_t bufLen, const unsigned char *perm)
{
    size_t i;
    unsigned char j = 0;
    unsigned char k = 0;
    unsigned char temp;
    unsigned char s[256];
    
    memcpy(s, perm, 256);
    
    for(i = 0; i < bufLen; i++)
    {
        j += 1;
        k += s[j];
        
        temp = s[j];
        s[j] = s[k];
        s[k] = temp;
        
        buf[i] ^= s[(s[j] + s[k]) & 0xFF];
    }
}

void cbsCrypt(unsigned char *buf, size_t bufLen)
{
    rc4Crypt(buf, bufLen, cbsPerm);
}

int isCBSFile(const char *path)
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
    if(len < sizeof(cbsHeader_t))
    {
        fclose(f);
        return 0;
    }

    // Verify header magic
    char magic[4];
    fread(magic, 1, 4, f);
    fclose(f);

    int i;
    for(i = 0; i < 4; i++)
    {
        if(magic[i] != CBS_MAGIC[i])
            return 0;
    }

    return 1;
}

int extractCBS(gameSave_t *save, device_t dst)
{
    FILE *cbsFile, *dstFile;
    u8 *cbsData;
    u8 *compressed;
    u8 *decompressed;
    u8 *entryData;
    cbsHeader_t *header;
    cbsEntry_t entryHeader;
    unsigned long decompressedSize;
    int cbsLen, offset = 0;
    char *dirName;
    char dstName[100];
    
    if(!save || !(dst & (MC_SLOT_1|MC_SLOT_2)))
        return 0;

    if(!isCBSFile(save->path))
        return 0;
    
    cbsFile = fopen(save->path, "rb");
    if(!cbsFile)
        return 0;
    
    fseek(cbsFile, 0, SEEK_END);
    cbsLen = ftell(cbsFile);
    fseek(cbsFile, 0, SEEK_SET);
    cbsData = malloc(cbsLen);
    fread(cbsData, 1, cbsLen, cbsFile);
    fclose(cbsFile);
    
    header = (cbsHeader_t *)cbsData;    
    dirName = savesGetDevicePath(header->name, dst);
    
    int ret = fioMkdir(dirName);
    if(ret == -4)
    {
        // Prompt user to overwrite save if it already exists
        char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        if(choice == 1)
        {
            free(dirName);
            free(cbsData);
            return 0;
        }
    }
    
    // Get data for file entries
    compressed = cbsData + 0x128;
    // Some tools create .CBS saves with an incorrect compressed size in the header.
    // It can't be trusted!
    cbsCrypt(compressed, cbsLen - 0x128);
    decompressedSize = (unsigned long)header->decompressedSize;
    decompressed = malloc(decompressedSize);
    int z_ret = uncompress(decompressed, &decompressedSize, compressed, cbsLen - 0x128);
    
    if(z_ret != 0)
    {
        // Compression failed.
        free(dirName);
        free(cbsData);
        free(decompressed);
        return 0;
    }
    
    while(offset < (decompressedSize - 64))
    {
        float progress = (float)offset / decompressedSize;
        drawCopyProgress(progress);
        
        /* Entry header can't be read directly because it might not be 32-bit aligned.
        GCC will likely emit an lw instruction for reading the 32-bit variables in the
        struct which will halt the processor if it tries to load from an address
        that's misaligned. */
        memcpy(&entryHeader, &decompressed[offset], 64);
        
        offset += 64;
        entryData = &decompressed[offset];
        
        snprintf(dstName, 100, "%s/%s", dirName, entryHeader.name);
        
        dstFile = fopen(dstName, "wb");
        if(!dstFile)
        {
            free(dirName);
            free(cbsData);
            free(decompressed);
            return 0;
        }
        
        fwrite(entryData, 1, entryHeader.length, dstFile);
        fclose(dstFile);
        
        offset += entryHeader.length;
    }
    
    free(dirName);
    free(decompressed);
    free(cbsData);
    return 1;
}

int createCBS(gameSave_t *save, device_t src)
{
    FILE *cbsFile, *mcFile;
    sceMcTblGetDir mcDir[64] __attribute__((aligned(64)));
    cbsHeader_t header;
    cbsEntry_t entryHeader;
    fio_stat_t stat;
    u8 *dataBuff;
    u8 *dataCompressed;
    unsigned long compressedSize;
    int dataOffset = 0;
    char mcPath[100];
    char cbsPath[100];
    char filePath[150];
    char validName[32];
    int i;
    int ret;
    
    if(!save || !(src & (MC_SLOT_1|MC_SLOT_2)))
        return 0;
    
    memset(&header, 0, sizeof(cbsHeader_t));
    memset(&entryHeader, 0, sizeof(cbsEntry_t));
    
    replaceIllegalChars(save->name, validName, '-');
    rtrim(validName);
    snprintf(cbsPath, 100, "%s%s.cbs", flashDriveDevice, validName);
    
    if(fioGetstat(cbsPath, &stat) == 0)
    {
        char *items[] = {"Yes", "No"};
        int choice = displayPromptMenu(items, 2, "Save already exists. Do you want to overwrite it?");
        
        if(choice == 1)
            return 0;
    }
    
    cbsFile = fopen(cbsPath, "wb");
    if(!cbsFile)
        return 0;
    
    snprintf(mcPath, 100, "%s/*", strstr(save->path, ":") + 1);
    
    mcGetDir((src == MC_SLOT_1) ? 0 : 1, 0, mcPath, 0, 54, mcDir);
    mcSync(0, NULL, &ret);
    
    for(i = 0; i < ret; i++)
    {
        if(mcDir[i].AttrFile & MC_ATTR_FILE)
            header.decompressedSize += mcDir[i].FileSizeByte + sizeof(cbsEntry_t);
    }
    
    dataBuff = malloc(header.decompressedSize);
    
    float progress = 0.0;
    for(i = 0; i < ret; i++)
    {
        if(mcDir[i].AttrFile & MC_ATTR_SUBDIR)
        {
            strncpy(header.magic, "CFU\0", 4);
            header.unk1 = 0x1F40;
            header.dataOffset = 0x128;
            strncpy(header.name, strstr(save->path, ":") + 1, 32);
            memcpy(&header.created, &mcDir[i]._Create, sizeof(sceMcStDateTime));
            memcpy(&header.modified, &mcDir[i]._Modify, sizeof(sceMcStDateTime));
            header.mode = 0x8427;
            strncpy(header.title, save->name, 32);
        }
        else if(mcDir[i].AttrFile & MC_ATTR_FILE)
        {
            progress += (float)1/(ret-2);
            drawCopyProgress(progress);
            
            memcpy(&entryHeader.created, &mcDir[i]._Create, sizeof(sceMcStDateTime));
            memcpy(&entryHeader.modified, &mcDir[i]._Modify, sizeof(sceMcStDateTime));
            entryHeader.length = mcDir[i].FileSizeByte;
            entryHeader.mode = mcDir[i].AttrFile;
            strncpy(entryHeader.name, mcDir[i].EntryName, 32);
            
            memcpy(&dataBuff[dataOffset], &entryHeader, sizeof(cbsEntry_t));
            dataOffset += sizeof(cbsEntry_t);
            
            snprintf(filePath, 100, "%s/%s", save->path, entryHeader.name);
            mcFile = fopen(filePath, "rb");
            fread(&dataBuff[dataOffset], 1, entryHeader.length, mcFile);
            fclose(mcFile);
            
            dataOffset += entryHeader.length;
        }
    }
    
    compressedSize = compressBound(header.decompressedSize);
    dataCompressed = malloc(compressedSize);
    if(!dataCompressed)
    {
        printf("malloc failed\n");
        free(dataBuff);
        fclose(cbsFile);
        return 0;
    }
    
    ret = compress2(dataCompressed, &compressedSize, dataBuff, header.decompressedSize, Z_BEST_COMPRESSION);
    if(ret != Z_OK)
    {
        printf("compress2 failed\n");
        free(dataBuff);
        free(dataCompressed);
        fclose(cbsFile);
        return 0;
    }
    
    header.compressedSize = compressedSize + 0x128;
    fwrite(&header, 1, sizeof(cbsHeader_t), cbsFile);
    cbsCrypt(dataCompressed, compressedSize);
    fwrite(dataCompressed, 1, compressedSize, cbsFile);
    fclose(cbsFile);
    
    free(dataBuff);
    free(dataCompressed);
    
    return 1;
}
