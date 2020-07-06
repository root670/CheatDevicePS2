#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "textcheats.h"
#include "cheats.h"
#include "graphics.h"
#include "objectpool.h"
#include "util.h"
#include "hash.h"
#include "libraries/minizip/unzip.h"

#define TOKEN_TITLE       (1 << 0) // "Game"
#define TOKEN_CHEAT       (1 << 1) // Cheat
#define TOKEN_CODE        (1 << 2) // 12345678 12345678
#define TOKEN_CODE_MAPPED (1 << 3) // 12345678 $Name
#define TOKEN_MAP_START   (1 << 4) // [Name]
#define TOKEN_MAP_ENTRY   (1 << 5) // 00: Item or 00=Item

typedef struct loadingContext {
    cheatsGame_t *gamesHead; // First game added
    cheatsGame_t *game; // Current game
    cheatsCheat_t *cheat; // Current cheat
    unsigned int numGamesAdded;
    int lastToken; // Previous line's token type
    char mapName[16]; // name of current value map
    cheatsValueMap_t *valueMap; // Current value map
    hashTable_t *listHashes; // list name -> valueMapIndex
} loadingContext_t;

static loadingContext_t g_ctx;

static void loadingContextInit()
{
    g_ctx.gamesHead = NULL;
    g_ctx.game = NULL;
    g_ctx.cheat = NULL;
    g_ctx.numGamesAdded = 0;
    g_ctx.lastToken = 0;
    g_ctx.valueMap = NULL;
    g_ctx.listHashes = hashNewTable(15);
}

static void loadingContextDestroy()
{
    hashDestroyTable(g_ctx.listHashes);
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

    *gamesAdded    = g_ctx.gamesHead;
    *numGamesAdded = g_ctx.numGamesAdded;

    free(text);
    loadingContextDestroy();
    
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

// Print value map title in buffer. Returns number of bytes written.
static inline int copyValueMapTitleToBuffer(char *buffer, const char *mapTitle)
{
    int length = 0;
    buffer[length++] = '[';
    strcpy(&buffer[length], mapTitle);
    length += strlen(mapTitle);
    buffer[length++] = ']';
    buffer[length++] = '\n';

    return length;
}

// Print value map entry in buffer. Returns number of bytes written
static inline int copyValueMapEntryToBuffer(char *buffer, const char *key, u32 value)
{
    int length = sprintf(buffer, "%X = %s\n", value, key);
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

static inline int copyValueMappedCodeToBuffer(char *buffer, u32 addr, u32 val, const char *mapName, int bytesPerEntry)
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

    // Find first non-zero nibble from the right so we can position the map
    // reference immediately after it.
    int refStart;
    for(refStart = (7 - bytesPerEntry*2); refStart >= 0; refStart--)
    {
        u8 nibble = (val >> (28 - refStart*4)) & 0xF;
        if(nibble)
            break;
    }

    refStart++; // Skip past the non-zero nibble

    // Convert value to ASCII
    for(i = 0; i < refStart; i++)
    {
        u8 nibble = (val >> (28 - i*4)) & 0xF;
        buffer[index++] = hexLUT[nibble];
    }

    // Write map reference
    buffer[index++] = '$';
    strcpy(&buffer[index], mapName);
    index += strlen(mapName);
    buffer[index++] = '\n';

    return index;
}

int textCheatsSave(const char *path, const cheatsGame_t *games, char *error, int errorLen)
{
    if(!path || !games)
        return 0;

    FILE *f = fopen(path, "w");
    if(!f)
    {
        if(error)
            snprintf(error, errorLen, "Failed to open \"%s\" for writing.", path);
        return 0;
    }

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

                int i;
                for(i = 0; i < game->numValueMaps; i++)
                {
                    const cheatsValueMap_t *map = game->valueMaps + i;
                    const char *mapTitle = map->title;
                    if(index + strlen(mapTitle) + 3 < sizeof(buf)) // 3 = 1 open bracket + 1 close bracket + 1 newline character
                    {
                        index += copyValueMapTitleToBuffer(&buf[index], mapTitle);
                    }
                    else
                    {
                        // Flush buffer to file
                        fwrite(buf, 1, index, f);
                        index = copyValueMapTitleToBuffer(&buf[0], mapTitle);
                    }

                    int j;
                    for(j = 0; j < map->numEntries; j++)
                    {
                        const char *key = getNthString(map->keys, j);
                        const u32 value = map->values[j];
                        int valueLength;
                        if(value <= 0xFF)
                            valueLength = 2;
                        else if(value <= 0xFFFF)
                            valueLength = 4;
                        else
                            valueLength = 8;
                        if(index + strlen(key) + valueLength + 4) // 4 = 2 spaces + 1 separating character + 1 newline character
                        {
                            index += copyValueMapEntryToBuffer(&buf[index], key, value);
                        }
                        else
                        {
                            fwrite(buf, 1, index, f);
                            index = copyValueMapEntryToBuffer(&buf[index], key, value);
                        }
                    }
                }
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

            int valueMappedCheat = cheat->type;

            int i;
            for(i = 0; i < cheat->numCodeLines; i++)
            {
                u32 addr = game->codeLines[cheat->codeLinesOffset + i];
                u32 val = game->codeLines[cheat->codeLinesOffset + i] >> 32;
                if(!valueMappedCheat || i != cheat->valueMapLine)
                {
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
                else
                {
                    const cheatsValueMap_t *map = &game->valueMaps[cheat->valueMapIndex];
                    char *mapName = map->title;

                    if(index + 17 + strlen(mapName) < sizeof(buf)) // worst case: 8 char address + 1 space + 6 char value + $ char + mapName + null char
                    {
                        index += copyValueMappedCodeToBuffer(&buf[index], addr, val, mapName, map->bytesPerEntry);
                    }
                    else
                    {
                        fwrite(buf, 1, index, f);
                        index = copyValueMappedCodeToBuffer(&buf[index], addr, val, mapName, map->bytesPerEntry);
                    }
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

int textCheatsSaveZip(const char *path, const cheatsGame_t *games, char *error, int errorLen)
{
    // TODO: Implement saving text cheats in a ZIP file.
    if(error)
        snprintf(error, errorLen, "Saving to zipped TXT is not supported.");
    return 1;
}

#define CONSUME_WHITESPACE(p, end) while(p != end && isspace(*p)) p++;
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

static const unsigned char isCodeDigitLUT[] = {
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,
    // 0x20: space
    1,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,
    // 0x30: [0-9]
    1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,
    // 0x41: [A-F]
    1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,
    // 0x61: [a-f]
    1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,
    0,0
};

// Determine token type for line.
static inline int getToken(const unsigned char *line, const int len)
{
    const unsigned char *c;
    int numTokens = 0;

    if (!line || len <= 0)
        return 0;

    if(UNLIKELY(line[0] == '"' && line[len-1] == '"'))
        return TOKEN_TITLE;

    if(UNLIKELY(line[0] == '[' && line[len-1] == ']'))
        return TOKEN_MAP_START;

    if(UNLIKELY(len >= 11 && len <= 26 && line[9] == '$'))
    {
        c = line;
        while(isCodeDigitLUT[*c++])
            numTokens++;

        if(numTokens == 9)
            return TOKEN_CODE_MAPPED;
    }

    if(UNLIKELY(len >= 13 && len <= 28 && line[11] == '$'))
    {
        c = line;
        while(isCodeDigitLUT[*c++])
            numTokens++;

        if(numTokens == 11)
            return TOKEN_CODE_MAPPED;
    }

    if(UNLIKELY(len > 15 && len <= 30 && line[13] == '$'))
    {
        c = line;
        while(isCodeDigitLUT[*c++])
            numTokens++;

        if(numTokens == 13)
            return TOKEN_CODE_MAPPED;
    }
    
    if(UNLIKELY(len > 15 && len <= 30 && line[15] == '$'))
    {
        c = line;
        while(isCodeDigitLUT[*c++])
            numTokens++;

        if(numTokens == 15)
            return TOKEN_CODE_MAPPED;
    }

    if(UNLIKELY(len > 3 &&
       ((g_ctx.lastToken == TOKEN_MAP_START) || 
        (g_ctx.lastToken == TOKEN_MAP_ENTRY))))
    {
        c = line;
        while(isxdigit(*c))
        {
            c++;
            numTokens++;
        }

        if(numTokens >= 1 && numTokens <= 8)
        {
            CONSUME_WHITESPACE(c, line + len);
            if (*c == ':' || *c == '=' || *c == '-')
                return TOKEN_MAP_ENTRY;
        }
    }

    if(len == 17 && line[8] == ' ')
    {
        c = line;
        while(isCodeDigitLUT[*c++])
            numTokens++;

        if(numTokens == 17)
            return TOKEN_CODE;
    }

    if((line[0] == '/' && line[1] == '/') || line[0] == '#')
        return 0; // Comment

    return TOKEN_CHEAT;
}

static inline int readGameTitleLine(const char *line, int len)
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
        if(!g_ctx.gamesHead)
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
    g_ctx.valueMap = NULL;
    hashDestroyTable(g_ctx.listHashes);
    g_ctx.listHashes = hashNewTable(15);

    return 1;
}

static inline int readCheatTitleLine(const char *line, int len)
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

    return 1;
}

static inline int readCodeLine(const char *line, int len)
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

    if(g_ctx.cheat->type != CHEAT_VALUE_MAPPED)
        g_ctx.cheat->type = CHEAT_NORMAL;
    g_ctx.cheat->numCodeLines++;
    g_ctx.game->codeLinesUsed++;

    return 1;
}

static int readMapStartLine(const char *line, int len)
{
    // All of the following are equivalent:
    // {xx}
    // { xx}
    // {xx }
    // { xx }

    // Skip past starting character ('[')
    const char *c = line + 1;
    const char *end = line + len;

    // Skip whitespace before map name
    CONSUME_WHITESPACE(c, end);

    // Get map name
    int i = 0;
    while((c != end) &&
            (i < sizeof(g_ctx.mapName) - 1) &&
            (!isspace(*c)) &&
            (*c != ']'))
    {
        g_ctx.mapName[i++] = *c++;
    }
        g_ctx.mapName[i] = '\0';

    if(i == 0)
        return 0; // Can't use a zero-length name

    unsigned int hash = hashFunction(g_ctx.mapName, strlen(g_ctx.mapName));

    int mapIndex = hashFindValue(g_ctx.listHashes, hash);
    if(mapIndex >= 0)
    {
        // Use existing list with this name
        g_ctx.valueMap = &g_ctx.game->valueMaps[mapIndex];
    }
    else
    {
        if(g_ctx.game->numValueMaps >= MAX_VALUE_MAPS)
        {
            // Exceeded maximum number of value maps for this game
            printf("Exceeded maximum number of value maps for this game (%d)\n", MAX_VALUE_MAPS);
            return 0;
        }

        // Add new list
        if(!g_ctx.game->valueMaps)
        {
            g_ctx.game->valueMaps = malloc(sizeof(cheatsValueMap_t));
        }
        else
        {
            g_ctx.game->valueMaps = realloc(g_ctx.game->valueMaps, sizeof(cheatsValueMap_t) * (g_ctx.game->numValueMaps + 1));
        }

        g_ctx.valueMap = &(g_ctx.game->valueMaps[g_ctx.game->numValueMaps]);
        memset(g_ctx.valueMap, 0, sizeof(cheatsValueMap_t));

        strncpy(g_ctx.valueMap->title, g_ctx.mapName, sizeof(g_ctx.mapName));

        hashAddValue(g_ctx.listHashes, g_ctx.game->numValueMaps, hash);
        g_ctx.game->numValueMaps++;
    }

    return 1;
}

static inline int readMapEntryLine(const char *line, int len)
{
    // All of the following are equivalent:
    // 00:Value
    // 00: Value
    // 00 : Value
    // 00=Value
    // 00 = Value

    if(!g_ctx.valueMap)
    {
        printf("Value map list not set\n");
        return 0;
    }

    const char *c   = line;
    const char *end = line + len;

    // Get value
    char hex[9];
    int i = 0;
    while(c != end && (i < sizeof(hex)-1) && isxdigit(*c))
        hex[i++] = *c++;

    hex[i] = '\0';
    u32 value = strtol(hex, NULL, 16);

    if(g_ctx.valueMap->bytesPerEntry == 0)
        g_ctx.valueMap->bytesPerEntry = i/2 + i%2; // Round up to nearest even number.

    // Skip whitespace before seperator (':' or '=')
    CONSUME_WHITESPACE(c, end);

    if(*c != ':' && *c != '=' && *c != '-')
    {
        printf("invalid seperator character (%c)\n", *c);
        return 0; // Valid seperator not found
    }

    // Skip seperator
    c++;

    // Skip whitespace after seperator
    CONSUME_WHITESPACE(c, end);

    // Get key
    char name[32];
    i = 0;
    while(i < (sizeof(name) - 1) && c != end)
        name[i++] = *c++;
    name[i] = '\0';

    size_t nameLength = strlen(name) + 1;

    // Add key to list
    if(!g_ctx.valueMap->keys)
        g_ctx.valueMap->keys = malloc(nameLength);
    else
        g_ctx.valueMap->keys = realloc(g_ctx.valueMap->keys, g_ctx.valueMap->keysLength + nameLength);
        
    memcpy(g_ctx.valueMap->keys + g_ctx.valueMap->keysLength, name, nameLength);
    g_ctx.valueMap->keysLength += nameLength;

    // Add value to list
    if(!g_ctx.valueMap->values)
        g_ctx.valueMap->values = malloc(sizeof(value));
    else
        g_ctx.valueMap->values = realloc(g_ctx.valueMap->values, (g_ctx.valueMap->numEntries + 1) * sizeof(value));

    g_ctx.valueMap->values[g_ctx.valueMap->numEntries] = value;
    g_ctx.valueMap->numEntries++;

    return 1;
}

static inline int readMappedCodeLine(const char *line, int len)
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

    // Decode 32-bit hexidecimal text values for address
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

    u64 address = ((u64)lut[(int)line[0]] << 28) |
                  ((u64)lut[(int)line[1]] << 24) |
                  ((u64)lut[(int)line[2]] << 20) |
                  ((u64)lut[(int)line[3]] << 16) |
                  ((u64)lut[(int)line[4]] << 12) |
                  ((u64)lut[(int)line[5]] <<  8) |
                  ((u64)lut[(int)line[6]] <<  4) |
                  ((u64)lut[(int)line[7]]);

    const char *c   = line + 9;
    const char *end = line + len;

    // Get 0-6 digits for value appearing before the map reference
    int i = 0;
    u64 value = 0;
    while(c != end && *c != '$')
    {
        value |= (u64)lut[(int)*c++] << (60 - 4*i);
        i++;
        value |= (u64)lut[(int)*c++] << (60 - 4*i);
        i++;
    }

    *codeLine = address | value;

    // Skip over $
    c++;

    // Get map name
    char mapName[sizeof(g_ctx.mapName)];
    i = 0;
    while(c != end && (i < sizeof(mapName) - 1) && !isspace(*c))
    {
        mapName[i++] = *c++;
    }

    mapName[i] = '\0';

    // Get map associated with this name
    unsigned int hash = hashFunction(mapName, i);
    int mapIndex = hashFindValue(g_ctx.listHashes, hash);
    if(mapIndex < 0)
        return 0; // Map not found

    g_ctx.cheat->valueMapIndex = mapIndex;
    g_ctx.cheat->type = CHEAT_VALUE_MAPPED;
    g_ctx.cheat->valueMapLine = g_ctx.cheat->numCodeLines;
    g_ctx.cheat->numCodeLines++;
    g_ctx.game->codeLinesUsed++;

    return 1;
}

// Parse line and process token.
static inline int parseLine(const char *line, const int len)
{
    if(len < 1)
        return 0;

    int token = getToken(line, len);
    g_ctx.lastToken = token;

    if(token == TOKEN_TITLE)
    {
        return readGameTitleLine(line, len);
    }
    else if (token == TOKEN_CHEAT)
    {
        return readCheatTitleLine(line, len);
    }
    else if(token == TOKEN_CODE)
    {
        return readCodeLine(line, len);
    }
    else if(token == TOKEN_MAP_START)
    {
        return readMapStartLine(line, len);
    }
    else if(token == TOKEN_MAP_ENTRY)
    {
        return readMapEntryLine(line, len);
    }
    else if(token == TOKEN_CODE_MAPPED)
    {
        return readMappedCodeLine(line, len);
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
