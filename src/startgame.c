#include "startgame.h"
#include "graphics.h"
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <sbv_patches.h>
#include <libcdvd.h>
#include <erl.h>
#include <libpad.h>
#include <tamtypes.h>

#include "menus.h"
#include "cheats.h"
#include "settings.h"
#include "util.h"
#include "objectpool.h"

typedef struct {
    u8  ident[16];  // struct definition for ELF object header
    u16 type;
    u16 machine;
    u32 version;
    u32 entry;
    u32 phoff;
    u32 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
} elf_header_t;

typedef struct {
    u32 type;       // struct definition for ELF program section header
    u32 offset;
    void    *vaddr;
    u32 paddr;
    u32 filesz;
    u32 memsz;
    u32 flags;
    u32 align;
} elf_pheader_t;

#define GS_BGCOLOUR *((vu32*)0x120000e0)

extern u8   _bootstrap_elf_start[];
extern int _bootstrap_elf_size;

static int discPrompt()
{
    const char *items[] = {"Start Game", "Cancel"};
    char *activeGameTitle = cheatsGetActiveGameTitle();
    char promptText[128];

    if(activeGameTitle)
        snprintf(promptText, sizeof(promptText), "Please insert game disc for\n%s", activeGameTitle);
    else
        snprintf(promptText, sizeof(promptText), "Please insert game disc");

    return (displayPromptMenu(items, 2, promptText) == 0);
}

#define ELF_PT_LOAD 1

// Load boostrap into memory. Returns bootstrap entrypoint.
static void* loadBootstrap()
{
    elf_header_t *eh = (elf_header_t *)_bootstrap_elf_start;
    elf_pheader_t *eph = (elf_pheader_t *)(_bootstrap_elf_start + eh->phoff);

    int i;
    for (i = 0; i < eh->phnum; i++)
    {
        if (eph[i].type != ELF_PT_LOAD)
            continue;

        void *pdata = (void *)(_bootstrap_elf_start + eph[i].offset);
        memcpy(eph[i].vaddr, pdata, eph[i].filesz);

        if (eph[i].memsz > eph[i].filesz)
            memset(eph[i].vaddr + eph[i].filesz, 0, eph[i].memsz - eph[i].filesz);
    }

    return (void *)eh->entry;
}

void startgameExecute(const char *path)
{
    static char boot2[100];
    
    if(strcmp(path, "==Disc==") == 0)
    {
        if(!discPrompt())
            return;

        graphicsDrawTextCentered(310, COLOR_YELLOW, "Starting game...");
        graphicsRender();

        // Wait for disc to be ready
        while(sceCdGetDiskType() == 1) {}
        sceCdDiskReady(0);
        
        int syscnfFile = open("cdrom0:\\SYSTEM.CNF;1", O_TEXT | O_RDONLY);
        if(!syscnfFile)
        {
            GS_BGCOLOUR = 0x1010B4; // red
            SleepThread();
        }
        
        char syscnfText[256];
        int syscnfLen = read(syscnfFile, syscnfText, 255);
        close(syscnfFile);
        if(!syscnfLen)
        {
            GS_BGCOLOUR = 0x1010B4;
            SleepThread();
        }
        
        syscnfText[syscnfLen] = '\0';

        int found = 0;
        char *line = strtok(syscnfText, "\n");
        while(line)
        {
            if(!found)
            {
                char *substr = strstr(line, "BOOT2");
                if(substr)
                {
                    substr += strlen("BOOT2");
                    while(*substr == ' ' || (*substr == '='))
                        substr++;
                    
                    strncpy(boot2, substr, 0x30);
                    found = 1;
                }
            }
            
            line = strtok(NULL, "\n");
        }
    }
    else
    {
        strncpy(boot2, path, 100);
    }

    cheatsSaveDatabase();
    settingsSave(NULL, 0);
    cheatsInstallCodesForEngine();
    killMenus();
    killCheats();
    killSettings();
    objectPoolKill();
    
    void *bootstrapEntrypoint = loadBootstrap();
    
    padPortClose(0, 0);
    
    fioExit();
    SifInitRpc(0);
    SifExitRpc();

    FlushCache(0); // data cache
    FlushCache(2); // instruction cache

    char *argv[2] = {boot2, "\0"};

    ExecPS2(bootstrapEntrypoint, 0, 2, argv);
};
