#include "database.h"
#include "cheats.h"
#include "graphics.h"
#include "zlib.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <kernel.h>
#include <tamtypes.h>

#define DBVERSION 0x02

typedef struct cdbMasterHeader {
    char magic[4];
    u8 version;
    u16 numTitles;
    char padding[9];
} cdbMasterHeader_t;

typedef struct cdbCheat {
    char *title;
    u8 numCodeLines;
    u64 *codeEntries;
} cdbCheat_t;

static u8 *cdbTitleBuffer = NULL;
static u8 *cdbCheatsBuffer = NULL;
static cheatsGame_t *gamesHead = NULL;

static int cdbOpenBuffer(unsigned char *cdbBuff)
{
    int ignoreFirst = 0;
    u16 numCheats;
    u16 numCodeLines;
    unsigned int cheatsOffset;
    u64 *codeLines = NULL;
    cheatsCheat_t *cheatsHead = NULL;
    cheatsCheat_t *cheatsArray = NULL;
    cheatsCheat_t *cheat = NULL;
    
    printf("Loading buffer\n");
    cheatsGame_t *game;
    cdbMasterHeader_t *cdbHeader = (cdbMasterHeader_t *)cdbBuff;

    if(strncmp(cdbHeader->magic, "CDB\0", 4) != 0)
    {
        free(cdbBuff);

        char error[255];
        sprintf(error, "%s: invalid database format %c%c%c\n", __FUNCTION__, cdbHeader->magic[0], cdbHeader->magic[1], cdbHeader->magic[2]);
        graphicsDrawText(50, 300, error, WHITE);
        graphicsRender();
        SleepThread();
    }

    if(cdbHeader->version > DBVERSION)
    {
        free(cdbBuff);

        char error[255];
        sprintf(error, "%s: unsupported database version\n", __FUNCTION__);
        graphicsDrawText(50, 300, error, WHITE);
        graphicsRender();
        SleepThread();
    }

    printf("magic %s\n", cdbHeader->magic);
    printf("version %d\n", cdbHeader->version);
    printf("numgames %d\n", cdbHeader->numTitles);

    cdbTitleBuffer = cdbBuff + 16;

    game = calloc(cdbHeader->numTitles, sizeof(cheatsGame_t));
    gamesHead = game;

    int i, j;
    for(i = 0; i < cdbHeader->numTitles; i++)
    {
        int titlelen = *((u8 *)cdbTitleBuffer);
        if(titlelen>81)
            return 0;

        cdbTitleBuffer += 1;
        strncpy(game->title, cdbTitleBuffer, titlelen);
        cdbTitleBuffer += titlelen;
        
        // The values might not be aligned properly, so they're read byte-by-byte
        numCheats = *cdbTitleBuffer | (*(cdbTitleBuffer+1) << 8);
        cdbTitleBuffer += sizeof(u16);

        numCodeLines = *cdbTitleBuffer | (*(cdbTitleBuffer+1) << 8);
        codeLines = calloc(numCodeLines, sizeof(u64));
        cdbTitleBuffer += sizeof(u16);
        
        cheatsOffset = *cdbTitleBuffer | (*(cdbTitleBuffer+1) << 8) | (*(cdbTitleBuffer+2) << 16) | (*(cdbTitleBuffer+3) << 24);
        cdbCheatsBuffer = cdbBuff + cheatsOffset;
        cdbTitleBuffer += sizeof(u32);
        
        cheatsArray = calloc(numCheats, sizeof(cheatsCheat_t));
        cheatsHead = &cheatsArray[0];
        cheat = cheatsHead;
        
        ignoreFirst = 0;
        /* Read each cheat */
        for(j = 0; j < numCheats; j++)
        {
            titlelen = *cdbCheatsBuffer;
            if(titlelen>81)
            {
                printf("ERROR READING DB: title length is too long!\n");
                return 0;
            }

            cdbCheatsBuffer++;
            strncpy(cheat->title, cdbCheatsBuffer, titlelen-1);

            cdbCheatsBuffer+= titlelen;
            cheat->numCodeLines = *((u8 *)cdbCheatsBuffer);
            cdbCheatsBuffer++;

            int numHookLines = 0;
            int line = 0;
            if(cheat->numCodeLines > 0)
            {
                cheat->type = CHEATNORMAL;
                cheat->codeLines = codeLines;
                memcpy(codeLines, cdbCheatsBuffer, cheat->numCodeLines * sizeof(u64));

                /*
                If cheat only contains 9-type hook codes:
                -> don't display cheat in the menu
                -> automatically enable cheat when launching game
                -> engine will NOT look for a hook if 1 or more 9-type hook codes are found
                */
                while(line < cheat->numCodeLines && (cheat->codeLines[line] & 0xF0000000) == 0x90000000)
                {
                    numHookLines++;
                    line++;
                }

                if(numHookLines == cheat->numCodeLines)
                {
                    cheat->skip = 1;
                    game->enableCheats = cheat;
                    game->numCheats--;
                }

                cdbCheatsBuffer += cheat->numCodeLines * sizeof(u64);
            }
            else
            {
                cheat->type = CHEATHEADER;
                cheat->codeLines = NULL;
            }

            if(j+1 < numCheats)
                cheat->next = &cheatsHead[j+1];
            else
                cheat->next = NULL;

            codeLines += cheat->numCodeLines;
            cheat = cheat->next;
        }

        game->cheats = cheatsHead;
        game->numCheats = numCheats;

        if(i+1 < cdbHeader->numTitles)
        {
            game->next = game + 1;
            game = game->next;
        }
        else
        {
            game->next = NULL;
        }
    }

    printf("done loading db\n");
    return cdbHeader->numTitles;
}

cheatsGame_t* cdbOpen(const char *path, unsigned int *numGames)
{
    u8 *compressed, *decompressed;
    int compressedSize;
    unsigned long decompressedSize;
    FILE *dbFile;

    if(!path)
        return NULL;
    if(!numGames)
        return NULL;
    
    dbFile = fopen(path, "rb");
    if(!dbFile)
    {
        printf("%s: failed to open %s\n", __FUNCTION__, path);
        return 0;
    }
    
    fseek(dbFile, 0, SEEK_END);
    compressedSize = ftell(dbFile);
    fseek(dbFile, 0, SEEK_SET);
    compressed = malloc(compressedSize);
    fread(compressed, 1, compressedSize, dbFile);
    fclose(dbFile);

    decompressedSize = 5*1024*1024; // 5MB
    decompressed = malloc(decompressedSize);
    uncompress(decompressed, &decompressedSize, compressed, compressedSize);
    realloc(decompressed, decompressedSize);
    free(compressed);

    *numGames = cdbOpenBuffer(decompressed);

    free(decompressed);

    return gamesHead;
}

int cdbSave(const char *path)
{
    return 1;
}
