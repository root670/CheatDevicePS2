#include "textcheats.h"
#include "cheats.h"
#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define TOKEN_TITLE		(1 << 0)
#define TOKEN_CHEAT		(1 << 1)
#define TOKEN_CODE		(1 << 2)
#define TOKEN_VALID		TOKEN_TITLE|TOKEN_CHEAT|TOKEN_CODE
#define TOKEN_START		TOKEN_TITLE

static cheatsGame_t *gamesHead = NULL;
static cheatsGame_t *game = NULL;
static cheatsCheat_t *cheatsHead = NULL;
static int usedCheats = 0;
static cheatsCheat_t *cheat = NULL;
static u64 *codeLines = NULL;
static u8 *text;
static int tokenNext;

static int numGames = 0;
static int numCheats = 0;
static int numCodeLines = 0;

void countTokens(const char *text, int *numGames, int *numCheats, int *numCodeLines)
{
	char line[255];
	int len = 0;
	if(!text || !numGames || !numCheats || !numCodeLines)
		return;
		
	while(*text)
	{
		char *off1 = strchr(text, '\n');
		len = off1 - text;
		
		if(len)
		{
			strncpy(line, text, 255);
			strtok(line, "\r");
			line[len] = '\0';
			
			int token = getToken(line);

			switch(token)
			{
				case TOKEN_TITLE:
					*numGames += 1;
					break;
				case TOKEN_CHEAT:
					*numCheats += 1;
					break;
				case TOKEN_CODE:
					*numCodeLines += 1;
					break;
				default:
					break;
			}
			
			text += len + 1;
		}
	}
}

// Returns number of games in cheat file.
int textCheatsOpenFile(const char *path)
{
	FILE *txtFile;
	char *text;
	char *buff;
	char *c;
	char line[255];
	int numLines = 0, textSize;
	
	if(!path)
		return 0;
	
	txtFile = fopen(path, "r");
	fseek(txtFile, 0, SEEK_END);
	int len = ftell(txtFile);
	fseek(txtFile, 0, SEEK_SET);
	
	buff = malloc(len);
	text = buff;
	fread(text, 1, len, txtFile);
	
	printf(" *** Counting tokens ***\n");
	countTokens(text, &numGames, &numCheats, &numCodeLines);
	fseek(txtFile, 0, SEEK_SET);
	
	printf("Games: %d\nCheats: %d\nLines: %d\n", numGames, numCheats, numCodeLines);
	
	printf(" *** Allocating structs\n");
	gamesHead = calloc(numGames, sizeof(cheatsGame_t));
	cheatsHead = calloc(numCheats, sizeof(cheatsCheat_t));
	
	tokenNext = TOKEN_START;
	
	while(*text)
	{
		char line[255];
		char *off1 = strchr(text, '\n');
		int len = off1 - text;
		
		if(len > 1)
		{
			strncpy(line, text, 255);
			strtok(line, "\r");
			line[len] = '\0';
			
			parseLine(line);
		}
		
		text += len + 1;
	}
	
	free(buff);
	
	return numGames;
}

// Returns number og games in cheat file.
int textCheatsOpenBuffer(unsigned char *buff)
{
	return 0;
}
// Get data structure of loaded cheat file. Returns null on error.
cheatsGame_t *textCheatsGetCheatStruct()
{
	return gamesHead;
}
// Free cheat file from memory.
int textCheatsClose()
{
	return 1;
}

// Determine token type for line.
int getToken(const char *line)
{
	//printf("line = %s\n", line);
	const char *c;
	int len, numDigits = 0, ret;
	
	if (!line || strlen(line) <= 0)
		return 0;
	
	len = strlen(line);	
	
	if(len == 17 && line[8] == ' ')
	{
		c = line;
		while(*c)
		{
			if(isxdigit(*c++))
				numDigits++;
		}
		
		if(numDigits == 16)
			ret = TOKEN_CODE;
	}
	
	else if(line[0] == '"' && line[len-1] == '"')
		ret = TOKEN_TITLE;
		
	else if(line[0] == '/' && line[1] == '/')
		ret = 0;
	
	else
		ret = TOKEN_CHEAT;
	
	return ret;
}

// Find next expected token.
int getNextToken(int tokenCurrent)
{
	if(!(tokenCurrent & TOKEN_VALID))
		return 0;
	
	switch(tokenCurrent)
	{
		case TOKEN_TITLE:
			return TOKEN_TITLE|TOKEN_CHEAT;
		case TOKEN_CHEAT:
		case TOKEN_CODE:
			return TOKEN_TITLE|TOKEN_CHEAT|TOKEN_CODE;
		default:
			return 0;
	}
}

// Parse line and process token.
int parseLine(const char *line)
{
	int token, len;
	
	token = getToken(line);
		
	switch(token)
	{
		case TOKEN_TITLE:
			// create new game
			printf("%s\n", line);
			if(!game)
			{
				//printf(">>>> first game\n");
				game = gamesHead;
			}
			else
			{
				//printf(">>>> not first game\n");
				game->next = ++game;
			}
			strncpy(game->title, strtok(line+1, "\""), 81);
			game->next = NULL;
			
			break;
		case TOKEN_CHEAT:
			// create new cheat
			//printf("  %s\n", line);
			game->numCheats++;
			if(!game)
				return 0;
			if(game->cheats == NULL)
			{
				//printf("  >>>> first cheat in game\n");
				game->cheats = &cheatsHead[usedCheats++];
				cheat = game->cheats;
			}
			else
			{
				//printf("  >>>> not first cheat in game\n");
				cheat->next = &cheatsHead[usedCheats++];
				cheat = cheat->next;
			}
			strncpy(cheat->title, line, 81);
			cheat->type = CHEATHEADER;
			cheat->next = NULL;
			
			break;
		case TOKEN_CODE:
			// add code to cheat
			//printf("Code: %s\n", line);
			//printf("    %s\n", line);
			if(!game || !cheat)
				return 0;
			if(!cheat->codeLines)
			{
				//printf("Allocating space for code lines");
				cheat->codeLines = calloc(1, sizeof(u64));
			}
			else
				//cheat->codeLines = realloc(cheat->codeLines, (cheat->numCodeLines + 1) * sizeof(u64));
			//cheat->codeLines[0] = 5;
			cheat->type = CHEATNORMAL;
			cheat->numCodeLines++;
			
			break;
			
		default:
			//printf("skip\n");
			break;
	}
	
	//printf("Next token type :%d\n", tokenNext);
	return 1;
}
