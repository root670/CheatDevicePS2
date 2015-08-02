#include "database.h"
#include "cheats.h"
#include "storage.h"
#include "graphics.h"
#include "zlib.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <kernel.h>

#define DBVERSION 0x02

static int dbIsOpen = 0;
static char dbPath[255];
static u8 *dbBuff = NULL;
static u8 *dbTitleBuffer = NULL;
static u8 *dbCheatsBuffer = NULL;
static dbMasterHeader_t *dbHeader = NULL;
static cheatsGame_t *gamesHead = NULL;

int dbOpenBuffer(unsigned char *buff)
{
	unsigned short numCheats;
	unsigned short numCodeLines;
	unsigned int cheatsOffset;
	u64 *codeLines = NULL;
	cheatsCheat_t *cheatsHead = NULL;
	cheatsCheat_t *cheatsArray = NULL;
	cheatsCheat_t *cheat = NULL;
	
	printf("Loading buffer\n");
	cheatsGame_t *game;
	dbBuff = buff;
	dbIsOpen = 1;
	dbHeader = (dbMasterHeader_t *)dbBuff;

	if(strncmp(dbHeader->magic, "CDB\0", 4) != 0)
	{
		free(dbBuff);
		dbIsOpen = 0;

		char error[255];
		sprintf(error, "%s: invalid database format %c%c%c\n", __FUNCTION__, dbHeader->magic[0], dbHeader->magic[1], dbHeader->magic[2]);
		graphicsDrawText(50, 300, error, WHITE);
		graphicsRender();
		SleepThread();
	}

	if(dbHeader->version > DBVERSION)
	{
		free(dbBuff);
		dbIsOpen = 0;

		char error[255];
		sprintf(error, "%s: unsupported database version\n", __FUNCTION__);
		graphicsDrawText(50, 300, error, WHITE);
		graphicsRender();
		SleepThread();
	}

	printf("magic %s\n", dbHeader->magic);
	printf("version %d\n", dbHeader->version);
	printf("numgames %d\n", dbHeader->numTitles);

	dbTitleBuffer = dbBuff + 16;

	game = calloc(dbHeader->numTitles, sizeof(cheatsGame_t));
	gamesHead = game;

	int i, j;
	for(i = 0; i < dbHeader->numTitles; i++)
	{
		int titlelen = *((u8 *)dbTitleBuffer);
		if(titlelen>81)
			return 0;

		dbTitleBuffer += 1;
		strncpy(game->title, dbTitleBuffer, titlelen);
		dbTitleBuffer += titlelen;
		
		// The values might not be aligned properly, so they're read byte-by-byte
		numCheats = *dbTitleBuffer | (*(dbTitleBuffer+1) << 8);
		dbTitleBuffer += sizeof(u16);

		numCodeLines = *dbTitleBuffer | (*(dbTitleBuffer+1) << 8);
		codeLines = calloc(numCodeLines, sizeof(u64));
		dbTitleBuffer += sizeof(u16);
		
		cheatsOffset = *dbTitleBuffer | (*(dbTitleBuffer+1) << 8) | (*(dbTitleBuffer+2) << 16) | (*(dbTitleBuffer+3) << 24);
		dbCheatsBuffer = dbBuff + cheatsOffset;
		dbTitleBuffer += sizeof(u32);
		
		cheatsArray = calloc(numCheats, sizeof(cheatsCheat_t));
		cheatsHead = &cheatsArray[0];
		cheat = cheatsHead;
		
		/* Read each cheat */
		for(j = 0; j < numCheats; j++)
		{
			titlelen = *dbCheatsBuffer;
			if(titlelen>81)
			{
				printf("ERROR READING DB: title length is too long!\n");
				return 0;
			}

			dbCheatsBuffer++;
			strncpy(cheat->title, dbCheatsBuffer, titlelen-1);

			dbCheatsBuffer+= titlelen;
			cheat->numCodeLines = *((u8 *)dbCheatsBuffer);
			dbCheatsBuffer++;

			if(cheat->numCodeLines > 0)
			{
				cheat->type = CHEATNORMAL;
				cheat->codeLines = codeLines;
				memcpy(codeLines, dbCheatsBuffer, cheat->numCodeLines * sizeof(u64));

				dbCheatsBuffer += cheat->numCodeLines * sizeof(u64);
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
		
		if(i+1 < dbHeader->numTitles)
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
	return dbHeader->numTitles;
}

int dbOpenDatabase(const char *path)
{
	int compressedSize;
	unsigned long decompressedSize;
	int numGames;

	if(!path)
		return 0;

	strncpy(dbPath, path, 255);
	if(storageOpenFile(dbPath, "rb"))
	{
		dbIsOpen = 1;
		u8 *compressed, *decompressed;
		
		compressed = storageGetFileContents(dbPath, &compressedSize);
		storageCloseFile(dbPath);

		decompressedSize = 5*1024*1024; // 5MB
		decompressed = malloc(decompressedSize);
		uncompress(decompressed, &decompressedSize, compressed, compressedSize);
		realloc(decompressed, decompressedSize);
		free(compressed);
		dbBuff = decompressed;

		numGames = dbOpenBuffer(dbBuff);

		return numGames;
	}
	else
	{
		printf("%s: failed to open %s\n", __FUNCTION__, path);
		return 0;
	}
}

int dbCloseDatabase()
{
	if(dbIsOpen)
	{
		free(dbBuff);
		dbTitleBuffer = NULL;
		dbHeader = NULL;
		gamesHead = NULL;
		dbIsOpen = 0;

		return 1;
	}
	else
		return 0;
}

cheatsGame_t *dbGetCheatStruct()
{
	if(dbIsOpen)
		return gamesHead;
	else
		return NULL;
}
