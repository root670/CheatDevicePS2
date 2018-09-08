/*
 * Save Utilities
 * Additional functions used by the save manager.
 */

#ifndef SAVEUTIL_H
#define SAVEUTIL_H

#include <string.h>
#include <tamtypes.h>
#include <libmc-common.h>

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

void cbsCrypt(unsigned char *buf, size_t bufLen);

#endif
