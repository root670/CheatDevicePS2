#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __PS2__
    #include <kernel.h>
    #include <sifrpc.h>
    #include <sbv_patches.h>
    #include <libcdvd.h>
    #include <erl.h>
    #include <libpad.h>
#elif __PS1__
    #include <psx.h>
    #include <psxbios.h>
#endif // __PS2__

#include "startgame.h"
#include "graphics.h"
#include "types.h"
#include "menus.h"
#include "cheats.h"
#include "settings.h"
#include "util.h"
#include "objectpool.h"

#ifdef __PS2__
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

    extern u8   _bootstrap_elf_start[];
    extern int _bootstrap_elf_size;

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
#endif

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

extern void startgameGetBootPathFromDisc(char *bootPath, int bootPathLen);

void startgameExecute(const char *path)
{
    static char bootPath[100];
    
    if(strcmp(path, "==Disc==") == 0)
    {
        if(!discPrompt())
            return;

        graphicsDrawTextCentered(310, COLOR_YELLOW, "Starting game...");
        graphicsRender();

        startgameGetBootPathFromDisc(bootPath, sizeof(bootPath));
    }
    else
    {
        strncpy(bootPath, path, 100);
    }

    cheatsSaveDatabase();
    settingsSave();
    cheatsInstallCodesForEngine();
    killMenus();
    killCheats();
    killSettings();
    objectPoolKill();
    
    #ifdef __PS2__
        void *bootstrapEntrypoint = loadBootstrap();
        
        padPortClose(0, 0);
        
        fioExit();
        SifInitRpc(0);
        SifExitRpc();

        FlushCache(0); // data cache
        FlushCache(2); // instruction cache

        char *argv[2] = {bootPath, "\0"};

        ExecPS2(bootstrapEntrypoint, 0, 2, argv);

    #elif __PS1__
        printf("Booting \"%s\"\n", bootPath);
        printf("boot : %p (%d bytes)\n", bootPath, strlen(bootPath));
        RemoveVBlankHandler();
        PSX_RestoreBiosState();

        _96_remove();
        _96_init();

        cheatsInstallEngine();

        EnterCriticalSection();
	    LoadExec(bootPath, 0x801FFFF0, 0);
    #endif
};
