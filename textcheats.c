#include "textcheats.h"
#include "cheats.h"
#include "graphics.h"
#include "objectpool.h"
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
static cheatsGame_t *game = NULL;
static cheatsCheat_t *cheat = NULL;

static unsigned int g_numGamesRead = 0;

static int parseLine(const char *line, const int len);

cheatsGame_t* textCheatsOpen(const char *path, unsigned int *numGamesRead)
{
    FILE *txtFile;
    char *text;
    char *end;
    char *endPtr;
    char *buff;
    unsigned int lineLen;
    size_t txtLen;
    
    if(!path)
        return NULL;
    if(!numGamesRead)
        return NULL;
    
    txtFile = fopen(path, "r");
    fseek(txtFile, 0, SEEK_END);
    txtLen = ftell(txtFile);
    fseek(txtFile, 0, SEEK_SET);
    
    buff = malloc(txtLen + 1);
    text = buff;
    endPtr = text + txtLen;
    fread(text, 1, txtLen, txtFile);
    fclose(txtFile);
    
    u32 lineNum = 0;
    while(text < endPtr)
    {
        if((lineNum % 100) == 0)
        {
            float progress = 1.0 - ((endPtr - text)/(float)txtLen);
            graphicsDrawLoadingBar(100, 375, progress);
            graphicsRenderNow();
        }
        end = strchr(text, '\n');
        if(!end) // Reading the last line
            end = endPtr;
        
        lineLen = end - text;
        
        if(lineLen)
        {
            // remove trailing whitespace;
            char *c;
            for(c = text + lineLen; (*c == ' ') || (*c == '\r') || (*c == '\n') || (*c == '\t'); --c)
                *c = '\0';

            parseLine(text, c - text + 1);
        }
        
        text += lineLen + 1;
        lineNum++;
    }
    
    free(buff);
    *numGamesRead = g_numGamesRead;
    
    return gamesHead;
}

int textCheatsSave(const char *path, const cheatsGame_t *games)
{
    return 1;
}

// Determine token type for line.
static inline int getToken(const char *line, const int len)
{
    const char *c;
    int numDigits = 0, ret;
    
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
static int parseLine(const char *line, const int len)
{
    cheatsGame_t *newGame;
    int token;
    
    token = getToken(line, len);
    
    switch(token)
    {
        case TOKEN_TITLE: // Create new game
            newGame = objectPoolAllocate(OBJECTPOOLTYPE_GAME);

            if(!game)
            {
                // First game
                gamesHead = newGame;
                game = gamesHead;
            }
            else
            {
                if(game->codeLinesUsed > 0)
                {
                    game->codeLinesCapacity = game->codeLinesUsed;
                    game->codeLines = realloc(game->codeLines, game->codeLinesCapacity * sizeof(u64));
                }
                
                game->next = newGame;
                game = newGame;
            }
            
            strncpy(game->title, line+1, 81);
            game->title[strlen(line) - 2] = '\0';
            game->next = NULL;
            g_numGamesRead += 1;
            break;
            
        case TOKEN_CHEAT: // Add new cheat to game
            if(!game)
                return 0;

            cheatsCheat_t *newCheat = objectPoolAllocate(OBJECTPOOLTYPE_CHEAT);

            if(game->cheats == NULL)
            {
                game->cheats = newCheat;
                cheat = game->cheats;
            }
            else
            {
                cheat->next = newCheat;
                cheat = cheat->next;
            }
            
            strncpy(cheat->title, line, 81);
            cheat->type = CHEATHEADER;
            cheat->next = NULL;
            game->numCheats++;
            break;
            
        case TOKEN_CODE: // Add code to cheat
            if(!game || !cheat)
                return 0;
            
            if(!game->codeLines)
            {
                game->codeLinesCapacity = 1;
                game->codeLines = calloc(game->codeLinesCapacity, sizeof(u64));
            }
            else if(game->codeLinesUsed == game->codeLinesCapacity)
            {
                game->codeLinesCapacity *= 2;
                game->codeLines = realloc(game->codeLines, game->codeLinesCapacity * sizeof(u64));
            }
            
            if(cheat->numCodeLines == 0)
            {
                cheat->codeLinesOffset = game->codeLinesUsed;
            }
            
            u64 *codeLine = game->codeLines + cheat->codeLinesOffset + cheat->numCodeLines;

            int i = 0;
            static char offsets[] = {3, 0, 2, 0, 1, 0, 0, 0, 0, 7, 0, 6, 0, 5, 0, 4};
            do
            {
                char c1 = line[i];
                char c2 = line[i + 1];
                u8 hex = 0;

                if(c1 >= 'a')
                    hex = (0xA + c1 - 'a') << 4;
                else if(c1 >= 'A')
                    hex = (0xA + c1 - 'A') << 4;
                else
                    hex = (c1 - '0') << 4;

                if(c2 >= 'a')
                    hex |= (0xA + c2 - 'a') & 0xF;
                else if(c2 >= 'A')
                    hex |= (0xA + c2 - 'A') & 0xF;
                else
                    hex |= (c2 - '0') & 0xF;

                *((u8 *)codeLine + offsets[i]) = hex;

                if(i != 6)
                    i += 2; // Skip over 2 hex digits
                else
                    i += 3; // Skip over 2 hex digits + space

            } while( i <= 16 );

            cheat->type = CHEATNORMAL;
            cheat->numCodeLines++;
            game->codeLinesUsed++;
            break;
            
        default:
            break;
    }
    
    return 1;
}
