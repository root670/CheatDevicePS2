#ifndef SAVEUTIL_H
#define SAVEUTIL_H

#include <string.h>
#include <tamtypes.h>

typedef struct cbsHeader {
    char magic[4];
    u32 unk1;
    u32 dataOffset;
    u32 decompressedSize;
    u32 compressedSize;
    char name[32];
    
    struct {
        u8 unknown1;
        u8 sec;
        u8 min;
        u8 hour;
        u8 day;
        u8 month;
        u16 year;
    } create;
    
    struct {
        u8 unknown1;
        u8 sec;
        u8 min;
        u8 hour;
        u8 day;
        u8 month;
        u16 year;
    } modify;
    
    u32 unk2;
    u32 mode;
    char unk3[16];
    char title[72];
    char description[132];
} cbsHeader_t;

typedef struct cbsEntry {
    struct {
        u8 unknown1;
        u8 sec;
        u8 min;
        u8 hour;
        u8 day;
        u8 month;
        u16 year;
    } create;
    
    struct {
        u8 unknown1;
        u8 sec;
        u8 min;
        u8 hour;
        u8 day;
        u8 month;
        u16 year;
    } modify;
    
    u32 length;
    u32 mode;
    char unk1[8];
    char name[32];
} cbsEntry_t;

void cbsCrypt(unsigned char *buf, size_t bufLen);

#endif
