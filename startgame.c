#include "startgame.h"
#include "graphics.h"
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <sbv_patches.h>
#include <libcdvd.h>
#include <erl.h>
#include <libpad.h>

#include "menus.h"
#include "cheats.h"
#include "settings.h"

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

static void discPrompt()
{
    struct padButtonStatus padStat;
    static u32 old_pad = PAD_CROSS;
    static u32 pad_pressed = 0;
    int state;
    
    graphicsDrawBackground();
    graphicsDrawText(30, 200, "Insert disc and press X to start the game.", WHITE);
    graphicsRenderNow();
    
    while(!(pad_pressed & PAD_CROSS))
    {       
        state = padGetState(0, 0);
        while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
            state = padGetState(0, 0);
    
        padRead(0, 0, &padStat);
    
        pad_pressed = (0xFFFF ^ padStat.btns) & ~old_pad;
        old_pad = 0xFFFF ^ padStat.btns;
    }
    
    graphicsDrawBackground();
    graphicsDrawText(30, 200, "Starting game...", WHITE);
    graphicsRenderNow();
}

#define ELF_PT_LOAD 1

void startgameExecute(char *path)
{
    elf_header_t *eh;
    elf_pheader_t *eph;
    void *pdata;
    int i, syscnf, syscnfLen, found = 0;
    char syscnfText[256];
    static char boot2[100];
    char *line, *substr;

    killMenuMan();
    killCheatMan();
    killSettingsMan();
    
    if(strcmp(path, "==Disc==") == 0)
    {
        discPrompt();
        // wait for disc to be ready
        while(sceCdGetDiskType() == 1);
        sceCdDiskReady(0);
        
        syscnf = open("cdrom0:\\SYSTEM.CNF;1", O_TEXT | O_RDONLY);
        if(!syscnf)
        {
            GS_BGCOLOUR = 0x1010B4; // red
            SleepThread();
        }
        
        syscnfLen = read(syscnf, syscnfText, 255);
        close(syscnf);
        if(!syscnfLen)
        {
            GS_BGCOLOUR = 0x1010B4;
            SleepThread();
        }
        
        syscnfText[syscnfLen] = '\0';
        line = strtok(syscnfText, "\n");
        
        while(line)
        {
            if(!found)
            {
                substr = strstr(line, "BOOT2");
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
    
    eh = (elf_header_t *)_bootstrap_elf_start;
    eph = (elf_pheader_t *)(_bootstrap_elf_start + eh->phoff);

    for (i = 0; i < eh->phnum; i++) {
        if (eph[i].type != ELF_PT_LOAD)
            continue;

        pdata = (void *)(_bootstrap_elf_start + eph[i].offset);
        memcpy(eph[i].vaddr, pdata, eph[i].filesz);

        if (eph[i].memsz > eph[i].filesz)
            memset(eph[i].vaddr + eph[i].filesz, 0, eph[i].memsz - eph[i].filesz);
    }
    
    padPortClose(0, 0);
    padReset();
    
    fioExit();
    SifInitRpc(0);
    SifExitRpc();

    FlushCache(0); // data cache
    FlushCache(2); // instruction cache
    
    char *argv[2] = {boot2, "\0"};

    ExecPS2((void *)eh->entry, 0, 2, argv);
};
