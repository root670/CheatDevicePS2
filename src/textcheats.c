#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "textcheats.h"
#include "cheats.h"
#include "graphics.h"
#include "objectpool.h"
#include "util.h"
#include "libraries/minizip/unzip.h"

#define TOKEN_TITLE     (1 << 0)
#define TOKEN_CHEAT     (1 << 1)
#define TOKEN_CODE      (1 << 2)
#define TOKEN_VALID     TOKEN_TITLE|TOKEN_CHEAT|TOKEN_CODE
#define TOKEN_START     TOKEN_TITLE

typedef struct loadingContext {
    cheatsGame_t *gamesHead;
    cheatsGame_t *game;
    cheatsCheat_t *cheat;
    unsigned int numGamesAdded;
} loadingContext_t;

static loadingContext_t g_ctx;

static void loadingContextInit()
{
    g_ctx.gamesHead = NULL;
    g_ctx.game = NULL;
    g_ctx.cheat = NULL;
    g_ctx.numGamesAdded = 0;
}

static int readTextCheats(char *text, size_t len);

int textCheatsOpen(const char *path, cheatsGame_t **gamesAdded, unsigned int *numGamesAdded)
{
    if(!path || !gamesAdded || !numGamesAdded)
        return 0;
    
    FILE *txtFile = fopen(path, "r");
    if(!txtFile)
        return 1;

    fseek(txtFile, 0, SEEK_END);
    size_t txtLen = ftell(txtFile);
    fseek(txtFile, 0, SEEK_SET);
    
    if(txtLen == 0)
        return 1; // Empty file.

    char *text = malloc(txtLen + 1);
    fread(text, 1, txtLen, txtFile);
    fclose(txtFile);

    // NULL-terminate so cheats can be properly parsed if it doesn't end with a
    // newline character.
    text[txtLen] = '\0';

    loadingContextInit(&g_ctx);
    readTextCheats(text, txtLen);
    free(text);

    *gamesAdded    = g_ctx.gamesHead;
    *numGamesAdded = g_ctx.numGamesAdded;
    
    return 1;
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
    graphicsRender();

    clock_t start = clock();
    char buf[8192];
    int index = 0;
    const cheatsGame_t *game = games;

    int numGamesProcessed = 0;
    while(game)
    {
        int wroteGameTitle = 0;
        if(numGamesProcessed % 500 == 0)
        {
            float progress = (float)numGamesProcessed / cheatsGetNumGames();
            graphicsDrawLoadingBar(50, 350, progress);
            graphicsRender();
        }

        const cheatsCheat_t *cheat = game->cheats;
        while(cheat)
        {
            if(cheat->readOnly)
            {
                // Cheat is from a read-only database, so we don't want to
                // duplicate it in the file being written.
                cheat = cheat->next;
                continue;
            }

            if(!wroteGameTitle)
            {
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
                wroteGameTitle = 1;
            }

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
        game = game->next;
        numGamesProcessed++;
    }

    // Flush remaining buffer to file
    if(index > 0)
        fwrite(buf, 1, index, f);

    fclose(f);
    clock_t end = clock();
    printf("Saving took %f seconds\n", ((float)end - start) / CLOCKS_PER_SEC);

    return 1;
}

int textCheatsOpenZip(const char *path, cheatsGame_t **gamesAdded, unsigned int *numGamesAdded)
{
    if(!path || !gamesAdded || !numGamesAdded)
        return 0;

    unzFile zipFile = unzOpen(path);
    if(!zipFile)
        return 1; // File doesn't exist.

    unz_global_info zipGlobalInfo;
    if(unzGetGlobalInfo(zipFile, &zipGlobalInfo) != UNZ_OK)
    {
        unzClose(zipFile);
        return 0;
    }

    unz_file_info64 zipFileInfo;
    char filename[100];
    if(unzGoToFirstFile2(zipFile, &zipFileInfo, 
                      filename, sizeof(filename), 
                      NULL, 0, 
                      NULL, 0) != UNZ_OK)
    {
        unzClose(zipFile);
        return 0;
    }

    // Read all .txt files in the archive
    cheatsGame_t *gamesHead = NULL;
    unsigned int numGamesAddedTotal = 0;
    int hasNextFile = UNZ_OK;
    while(hasNextFile == UNZ_OK)
    {
        const char *extension = getFileExtension(filename);
        if(!extension || strcasecmp(extension, "txt") != 0)
        {
            // Not a .txt file
            hasNextFile = unzGoToNextFile2(zipFile, &zipFileInfo,
                                        filename, sizeof(filename),
                                        NULL, 0,
                                        NULL, 0);
            continue;
        }

        // Current file is a .txt file, so we'll try to read it
        if(unzOpenCurrentFile(zipFile) != UNZ_OK)
        {
            unzClose(zipFile);
            return 0;
        }

        if(zipFileInfo.uncompressed_size == 0)
            continue; // Empty file.

        // +1 to NULL-terminate later
        char *text = malloc(zipFileInfo.uncompressed_size + 1);
        if(!text)
        {
            unzCloseCurrentFile(zipFile);
            unzClose(zipFile);
            return 0;
        }

        if(unzReadCurrentFile(zipFile, text, zipFileInfo.uncompressed_size) < 0)
        {
            // Zip file is corrupt!
            free(text);
            unzCloseCurrentFile(zipFile);
            unzClose(zipFile);
            return 0;
        }

        // NULL-terminate so cheats can be properly parsed if it doesn't end
        // with a newline character.
        text[zipFileInfo.uncompressed_size] = '\0';

        loadingContextInit(&g_ctx);
        readTextCheats(text, zipFileInfo.uncompressed_size);
        
        free(text);
        unzCloseCurrentFile(zipFile);

        if(!gamesHead)
        {
            gamesHead = g_ctx.gamesHead;
        }
        else
        {
            // Append to end of game list bring built
            cheatsGame_t *game = gamesHead;
            while(game->next)
                game = game->next;
            
            game->next = g_ctx.gamesHead;
        }

        numGamesAddedTotal += g_ctx.numGamesAdded;

        // Get next file, or exit the loop if there are no more files in the
        // archive.
        hasNextFile = unzGoToNextFile2(zipFile, &zipFileInfo,
                            filename, sizeof(filename),
                            NULL, 0,
                            NULL, 0);
    };

    unzClose(zipFile);

    *numGamesAdded = numGamesAddedTotal;
    *gamesAdded    = gamesHead;

    return 1;
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
    if(len < 1)
        return 0;

    int token = getToken(line, len);

    if(token == TOKEN_TITLE)
    {
        // Use existing game or create new one
        char tempName[81];
        strncpy(tempName, line+1, sizeof(tempName));
        tempName[len-2] = '\0'; // Remove trailing '"'

        // Try to find existing game with this title
        cheatsGame_t *thisGame = cheatsFindGame(tempName);

        if(!thisGame)
        {
            // No game with this title exists, so create a new one
            thisGame = objectPoolAllocate(OBJECTPOOLTYPE_GAME);
            if(!g_ctx.game)
            {
                // First game added
                g_ctx.gamesHead = thisGame;
            }
            else
                g_ctx.game->next = thisGame;

            strncpy(thisGame->title, tempName, 81);
            g_ctx.numGamesAdded += 1;
        }
        else
        {
            // Add cheats to end of game's existing cheat list
            cheatsCheat_t *cheat = thisGame->cheats;
            if(cheat)
            {
                while(cheat->next)
                    cheat = cheat->next;
            }

            g_ctx.cheat = cheat;
        }

        if(g_ctx.game && g_ctx.game->codeLinesUsed > 0)
        {
            // Free excess memory from the previous game's code list
            g_ctx.game->codeLinesCapacity = g_ctx.game->codeLinesUsed;
            g_ctx.game->codeLines = realloc(g_ctx.game->codeLines, g_ctx.game->codeLinesCapacity * sizeof(u64));
        }

        g_ctx.game = thisGame;
    }
    else if (token == TOKEN_CHEAT)
    {
        // Add new cheat to game
        if(!g_ctx.game)
            return 0;

        cheatsCheat_t *newCheat = objectPoolAllocate(OBJECTPOOLTYPE_CHEAT);

        if(!g_ctx.game->cheats)
        {
            // Game's first cheat
            g_ctx.game->cheats = newCheat;
        }
        else
            g_ctx.cheat->next = newCheat;

        g_ctx.cheat = newCheat;
        
        strncpy(g_ctx.cheat->title, line, 81);
        g_ctx.cheat->type = CHEAT_HEADER;
        g_ctx.cheat->next = NULL;
        g_ctx.game->numCheats++;
    }
    else if(token == TOKEN_CODE)
    {
        // Add code to cheat
        if(!g_ctx.game || !g_ctx.cheat)
            return 0;
        
        if(!g_ctx.game->codeLines)
        {
            g_ctx.game->codeLinesCapacity = 1;
            g_ctx.game->codeLines = calloc(g_ctx.game->codeLinesCapacity, sizeof(u64));
        }
        else if(g_ctx.game->codeLinesUsed == g_ctx.game->codeLinesCapacity)
        {
            g_ctx.game->codeLinesCapacity *= 2;
            g_ctx.game->codeLines = realloc(g_ctx.game->codeLines, g_ctx.game->codeLinesCapacity * sizeof(u64));
        }
        
        if(g_ctx.cheat->numCodeLines == 0)
            g_ctx.cheat->codeLinesOffset = g_ctx.game->codeLinesUsed;
        
        u64 *codeLine = g_ctx.game->codeLines + g_ctx.cheat->codeLinesOffset + g_ctx.cheat->numCodeLines;

        // Decode pair of 32-bit hexidecimal text values
        static const unsigned char lut[] = {
            0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            // 0x30: [0-9]
            0,1,2,3,4,5,6,7,8,9,
            0,0,0,0,0,0,0,
            // 0x41: [A-F]
            0xa,0xb,0xc,0xd,0xe,0xf,
            0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,
            // 0x61: [a-f]
            0xa,0xb,0xc,0xd,0xe,0xf
        };

        u64 hex = ((u64)lut[(int)line[ 0]] << 28) |
                    ((u64)lut[(int)line[ 1]] << 24) |
                    ((u64)lut[(int)line[ 2]] << 20) |
                    ((u64)lut[(int)line[ 3]] << 16) |
                    ((u64)lut[(int)line[ 4]] << 12) |
                    ((u64)lut[(int)line[ 5]] <<  8) |
                    ((u64)lut[(int)line[ 6]] <<  4) |
                    ((u64)lut[(int)line[ 7]])       |
                    ((u64)lut[(int)line[ 9]] << 60) |
                    ((u64)lut[(int)line[10]] << 56) |
                    ((u64)lut[(int)line[11]] << 52) |
                    ((u64)lut[(int)line[12]] << 48) |
                    ((u64)lut[(int)line[13]] << 44) |
                    ((u64)lut[(int)line[14]] << 40) |
                    ((u64)lut[(int)line[15]] << 36) |
                    ((u64)lut[(int)line[16]] << 32);

        *codeLine = hex;

        g_ctx.cheat->type = CHEAT_NORMAL;
        g_ctx.cheat->numCodeLines++;
        g_ctx.game->codeLinesUsed++;
    }
    
    return 1;
}

static int readTextCheats(char *text, size_t len)
{
    if(!text || len <= 0 )
        return 0;

    char *endPtr = text + len;
    u32 lineNum = 0;

    clock_t start = clock();
    
    while(text < endPtr)
    {
        char *end = strchr(text, '\n');
        if(!end) // Reading the last line
            end = endPtr - 1;
        
        int lineLen = end - text;
        if(lineLen)
        {
            // Remove trailing whitespace
            char *end;
            for(end = text + lineLen; (*end == ' ') || (*end == '\r') || (*end == '\n') || (*end == '\t'); --end)
                *end = '\0';

            // Remove leading whitespace
            while(isspace(*text) && lineLen > 0)
            {
                text++;
                lineLen--;
            }

            parseLine(text, end - text + 1);
        }
        
        text += lineLen + 1;
        lineNum++;
    }

    clock_t end = clock();
    printf("Loading took %f seconds\n", ((float)end - start) / CLOCKS_PER_SEC);

    return 1;
}
