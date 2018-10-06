#include "textcheats.h"
#include "cheats.h"
#include "graphics.h"
#include "objectpool.h"
#include "util.h"
#include "libraries/minizip/unzip.h"
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

static int readTextCheats(char *text, size_t len);

cheatsGame_t* textCheatsOpen(const char *path, unsigned int *numGamesRead)
{
    FILE *txtFile;
    char *text;
    char *buff;
    size_t txtLen;
    
    if(!path)
        return NULL;
    if(!numGamesRead)
        return NULL;
    
    txtFile = fopen(path, "r");
    if(!txtFile)
        return NULL;

    fseek(txtFile, 0, SEEK_END);
    txtLen = ftell(txtFile);
    fseek(txtFile, 0, SEEK_SET);
    
    buff = malloc(txtLen + 1);
    text = buff;
    fread(text, 1, txtLen, txtFile);
    fclose(txtFile);

    readTextCheats(text, txtLen);
    
    free(buff);
    *numGamesRead = g_numGamesRead;
    
    return gamesHead;
}

// Print game title surrounded by double-quotation marks in buffer. Returns
// number of bytes written.
static inline int copyGameTitleToBuffer(char *buffer, const char *gameTitle)
{
    int length = 0;
    buffer[length++] = '"';
    strcpy(&buffer[length], gameTitle);
    length += strlen(gameTitle);
    buffer[length++] = '"';
    buffer[length++] = '\n';

    return length;
}

// Print cheat title in buffer. Returns number of bytes written.
static inline int copyCheatTitleToBuffer(char *buffer, const char *cheatTitle)
{
    strcpy(buffer, cheatTitle);
    int length = strlen(cheatTitle);
    buffer[length++] = '\n';

    return length;
}

// Print "<addr> <val>\n" in text buffer. Returns number of bytes written.
static inline int copyCodeToBuffer(char *buffer, u32 addr, u32 val)
{
    int i;
    int index = 0;
    const char *hexLUT = "0123456789ABCDEF";

    // Convert address to ASCII
    for(i = 0; i < 8; i++)
    {
        u8 nibble = (addr >> (28 - i*4)) & 0xF;
        buffer[index++] = hexLUT[nibble];
    }
    buffer[index++] = ' ';
    // Convert value to ASCII
    for(i = 0; i < 8; i++)
    {
        u8 nibble = (val >> (28 - i*4)) & 0xF;
        buffer[index++] = hexLUT[nibble];
    }
    buffer[index++] = '\n';

    return 18; // 8 characters for address + space + 8 characters for value + newline
}

int textCheatsSave(const char *path, const cheatsGame_t *games)
{
    if(!path || !games)
        return 0;

    FILE *f = fopen(path, "w");
    if(!f)
        return 0;

    graphicsDrawLoadingBar(50, 350, 0.0);
    graphicsDrawTextCentered(310, COLOR_YELLOW, "Saving cheat database...");
    graphicsRenderNow();

    clock_t start = clock();
    char buf[8192];
    int index = 0;
    float progress = 0;
    const cheatsGame_t *game = games;
    while(game)
    {
        progress += (float)1/cheatsGetNumGames();
        graphicsDrawLoadingBar(50, 350, progress);
        graphicsRenderNow();

        if(index + strlen(game->title) + 3 < sizeof(buf)) // 3 = 2 double-quotes + 1 newline character
        {
            index += copyGameTitleToBuffer(&buf[index], game->title);
        }
        else
        {
            // Flush buffer to file
            fwrite(buf, 1, index, f);
            index = copyGameTitleToBuffer(&buf[0], game->title);
        }

        cheatsCheat_t *cheat = game->cheats;
        while(cheat)
        {
            if(index + strlen(cheat->title) + 1 < sizeof(buf))
            {
                index += copyCheatTitleToBuffer(&buf[index], cheat->title);
            }
            else
            {
                // Flush buffer to file
                fwrite(buf, 1, index, f);
                index = copyCheatTitleToBuffer(&buf[0], cheat->title);
            }

            int i;
            for(i = 0; i < cheat->numCodeLines; i++)
            {
                u32 addr = game->codeLines[cheat->codeLinesOffset + i];
                u32 val = game->codeLines[cheat->codeLinesOffset + i] >> 32;
                if(index + 18 < sizeof(buf))
                {
                    index += copyCodeToBuffer(&buf[index], addr, val);
                }
                else
                {
                    // Flush buffer to file
                    fwrite(buf, 1, index, f);
                    index = copyCodeToBuffer(&buf[0], addr, val);
                }
            }
            cheat = cheat->next;
        }
        buf[index++] = '\n';
        game = game->next;
    }

    // Flush remaining buffer to file
    if(index > 0)
        fwrite(buf, 1, index, f);

    fclose(f);
    clock_t end = clock();
    printf("Saving took %f seconds\n", ((float)end - start) / CLOCKS_PER_SEC);

    return 1;
}

cheatsGame_t* textCheatsOpenZip(const char *path, unsigned int *numGamesRead)
{
    if(!path || !numGamesRead)
        return NULL;

    unzFile zipFile = unzOpen(path);
    if(!zipFile)
        return NULL;

    unz_global_info zipGlobalInfo;
    if(unzGetGlobalInfo(zipFile, &zipGlobalInfo) != UNZ_OK)
    {
        unzClose(zipFile);
        return NULL;
    }

    // TODO: Read multiple TXT cheat database files from ZIP.
    if(zipGlobalInfo.number_entry != 1)
    {
        displayError("More than 1 file found in the cheat database ZIP file.\n"
                     "It should only contain a single TXT file with cheats.\n"
                     "Please read the documentation for a list of supported\n"
                     "cheat database formats.");
        unzClose(zipFile);
        return NULL;
    }

    unz_file_info64 zipFileInfo;
    char filename[100];
    if(unzGoToFirstFile2(zipFile, &zipFileInfo, 
                      filename, sizeof(filename), 
                      NULL, 0, 
                      NULL, 0) != UNZ_OK)
    {
        unzClose(zipFile);
        return NULL;
    }

    if(strcmp(getFileExtension(filename), "txt") != 0)
    {
        displayError("TXT file not found in cheat database ZIP file.\n"
                     "It should only contain a single TXT file with cheats.\n"
                     "Please read the documentation for a list of supported\n"
                     "cheat database formats");
        unzClose(zipFile);
        return NULL;
    }

    if(unzOpenCurrentFile(zipFile) != UNZ_OK)
    {
        unzClose(zipFile);
        return NULL;
    }

    char *text = malloc(zipFileInfo.uncompressed_size);
    if(!text)
    {
        unzCloseCurrentFile(zipFile);
        unzClose(zipFile);
        return NULL;
    }

    if(unzReadCurrentFile(zipFile, text, zipFileInfo.uncompressed_size) < 0)
    {
        // Zip file is corrupt!
        free(text);
        unzCloseCurrentFile(zipFile);
        unzClose(zipFile);
        return NULL;
    }

    unzCloseCurrentFile(zipFile);
    unzClose(zipFile);

    readTextCheats(text, zipFileInfo.uncompressed_size);

    free(text);
    *numGamesRead = g_numGamesRead;
    
    return gamesHead;
}

int textCheatsSaveZip(const char *path, const cheatsGame_t *games)
{
    // TODO: Implement saving text cheats in a ZIP file.
    return 1;
}

// Determine token type for line.
static inline int getToken(const char *line, const int len)
{
    const char *c;
    int numDigits = 0, ret;
    
    if (!line || len <= 0)
        return 0;

    if(line[0] == '"' && line[len-1] == '"')
        ret = TOKEN_TITLE;
    
    else if(len == 17 && line[8] == ' ')
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
    
    else if((line[0] == '/' && line[1] == '/') ||
             line[0] == '#')
        ret = 0;
    
    else
        ret = TOKEN_CHEAT;

    return ret;
}

// Parse line and process token.
static int parseLine(const char *line, const int len)
{
    cheatsGame_t *newGame;
    int token = getToken(line, len);

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
                    // Free excess memory from the previous game's code list
                    game->codeLinesCapacity = game->codeLinesUsed;
                    game->codeLines = realloc(game->codeLines, game->codeLinesCapacity * sizeof(u64));
                }
                
                game->next = newGame;
                game = newGame;
            }
            
            strncpy(game->title, line+1, 81);
            game->title[strlen(line) - 2] = '\0'; // Remove trailing '"'
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
            cheat->type = CHEAT_HEADER;
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

            // Decode pair of 32-bit hexidecimal text values
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
                    i += 2; // Skip over 2 hex nibbles
                else
                    i += 3; // Skip over 2 hex nibbles + space

            } while( i <= 16 );

            cheat->type = CHEAT_NORMAL;
            cheat->numCodeLines++;
            game->codeLinesUsed++;
            break;
            
        default:
            break;
    }
    
    return 1;
}

static int readTextCheats(char *text, size_t len)
{
    char *endPtr = text + len;
    u32 lineNum = 0;
    
    while(text < endPtr)
    {
        if((lineNum % 100) == 0)
        {
            float progress = 1.0 - ((endPtr - text)/(float)len);
            graphicsDrawLoadingBar(100, 375, progress);
            graphicsRenderNow();
        }
        char *end = strchr(text, '\n');
        if(!end) // Reading the last line
            end = endPtr - 1;
        
        int lineLen = end - text;
        if(lineLen)
        {
            // Remove trailing whitespace
            char *c;
            for(c = text + lineLen; (*c == ' ') || (*c == '\r') || (*c == '\n') || (*c == '\t'); --c)
                *c = '\0';

            parseLine(text, c - text + 1);
        }
        
        text += lineLen + 1;
        lineNum++;
    }

    return 1;
}
