#include "textcheats.h"
#include "cheats.h"
#include "graphics.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define TOKEN_TITLE     (1 << 0)
#define TOKEN_CHEAT     (1 << 1)
#define TOKEN_CODE      (1 << 2)
#define TOKEN_VALID     TOKEN_TITLE|TOKEN_CHEAT|TOKEN_CODE
#define TOKEN_START     TOKEN_TITLE

static cheatsGame_t *gamesHead = NULL;
static cheatsCheat_t *cheatsHead = NULL;
static u64 *codesHead = NULL;
static int usedCheats = 0;
static cheatsGame_t *game = NULL;
static cheatsCheat_t *cheat = NULL;
static u64 *codeLine = NULL;

static int numGames = 0;
static int numCheats = 0;
static int numCodeLines = 0;

static u8 *tokens = NULL;

int getToken(const char *line);
int parseLine(const char *line);

static void countTokens(const char *text, size_t length, int *numGames, int *numCheats, int *numCodeLines)
{
    const char *endPtr = text + length;
    const char *end;
    char line[255];
    int lineLen;
    int token;
    unsigned int tokenOffset = 0;
    if(!text || !numGames || !numCheats || !numCodeLines)
        return;
        
    while(text < endPtr)
    {
        float progress = (1.0 - ((endPtr - text)/(float)length))/2.0;
        graphicsDrawLoadingBar(100, 375, progress);
        graphicsRenderNow();
        end = strchr(text, '\n');
        if(!end)
            end = endPtr;
        
        lineLen = end - text;
        if(lineLen)
        {
            strncpy(line, text, 255);
            if(line[lineLen-1] == '\r')
                line[lineLen-1] = '\0';
            
            line[lineLen] = '\0';
            
            token = getToken(line);
            tokens[tokenOffset++] = token;

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
        }
        text += lineLen + 1;
    }
}

// Returns number of games in cheat file.
int textCheatsOpenFile(const char *path)
{
    FILE *txtFile;
    char *text;
    char *end;
    char *endPtr;
    char *buff;
    char line[255];
    unsigned int lineLen;
    size_t txtLen;
    
    if(!path)
        return 0;
    
    txtFile = fopen(path, "r");
    fseek(txtFile, 0, SEEK_END);
    txtLen = ftell(txtFile);
    fseek(txtFile, 0, SEEK_SET);
    
    buff = malloc(txtLen);
    text = buff;
    endPtr = text + txtLen;
    fread(text, 1, txtLen, txtFile);
    fclose(txtFile);
    
    tokens = malloc(512 * 1024); // 512 KB
    
    countTokens(text, txtLen, &numGames, &numCheats, &numCodeLines);
    
    gamesHead = calloc(numGames, sizeof(cheatsGame_t));
    cheatsHead = calloc(numCheats, sizeof(cheatsCheat_t));
    codesHead = calloc(numCodeLines, sizeof(u64));
    codeLine = codesHead;
    
    while(text < endPtr)
    {
        float progress = 0.5 + (1.0 - ((endPtr - text)/(float)txtLen))/2.0;
        graphicsDrawLoadingBar(100, 375, progress);
        graphicsRenderNow();
        end = strchr(text, '\n');
        if(!end) // Reading the last line
            end = endPtr;
        
        lineLen = end - text;
        
        if(lineLen)
        {
            strncpy(line, text, 255);
            if(line[lineLen-1] == '\r')
                line[lineLen-1] = '\0';
            
            line[lineLen] = '\0';
            
            parseLine(line);
        }
        
        text += lineLen + 1;
    }
    
    free(buff);
    free(tokens);
    
    return numGames;
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
    const char *c;
    int numDigits = 0, ret;
    size_t len;
    
    len = strlen(line);
    
    if (!line || len <= 0)
        return 0;
    
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
        else
            ret = TOKEN_CHEAT;
    }
    
    else if(line[0] == '"' && line[len-1] == '"')
        ret = TOKEN_TITLE;
        
    else if(line[0] == '/' && line[1] == '/')
        ret = 0;
    
    else
        ret = TOKEN_CHEAT;
    
    return ret;
}

// Parse line and process token.
int parseLine(const char *line)
{
    static unsigned int tokenOffset = 0;
    int token;
    
    token = tokens[tokenOffset++];
    
    switch(token)
    {
        case TOKEN_TITLE: // Create new game
            if(!game)
            {
                // First game
                game = gamesHead;
            }
            else
            {
                game->next = game + 1;
                game++;
            }
            
            strncpy(game->title, line+1, 81);
            game->title[strlen(line) - 2] = '\0';
            game->next = NULL;
            break;
            
        case TOKEN_CHEAT: // Add new cheat to game
            if(!game)
                return 0;

            if(game->enableCheat == NULL)
            {
                game->enableCheat = &cheatsHead[usedCheats++];
                game->enableCheat->enabled = 1;
                cheat = game->enableCheat;
            }
            else if(game->cheats == NULL)
            {
                // Game's first cheat following enable cheat
                game->cheats = &cheatsHead[usedCheats++];
                cheat = game->cheats;
                game->numCheats++;
            }
            else
            {
                cheat->next = &cheatsHead[usedCheats++];
                cheat = cheat->next;
                game->numCheats++;
            }
            
            strncpy(cheat->title, line, 81);
            cheat->type = CHEATHEADER;
            cheat->next = NULL;
            break;
            
        case TOKEN_CODE: // Add code to cheat
            if(!game || !cheat)
                return 0;
            
            if(!cheat->codeLines)
                cheat->codeLines = codeLine;
            
            sscanf(line, "%08X %08X", (u32 *)codeLine, ((u32 *)codeLine + 1));
            cheat->type = CHEATNORMAL;
            cheat->numCodeLines++;
            codeLine++;
            break;
            
        default:
            break;
    }
    
    return 1;
}
