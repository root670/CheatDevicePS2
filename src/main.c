#include <stdio.h>
#ifdef __PS2__
    #include <kernel.h>
    #include <sifrpc.h>
    #include <debug.h>
    #include <libpad.h>
#elif __PS1__
    #include <psx.h>
    #include "pad.h"
#endif

#include "types.h"
#include "graphics.h"
#include "menus.h"
#include "cheats.h"
#include "settings.h"
#include "util.h"

int main(int argc, char *argv[])
{
    #ifdef __PS1__
    printf("\n ** Loading main modules **\n");

    PSX_InitEx(PSX_INIT_SAVESTATE | PSX_INIT_CD);
    padInitialize();
    #endif
    initGraphics();
    loadModules();
    initSettings();
    initMenus();
    
    char *readOnlyPath = settingsGetReadOnlyDatabasePath();
    if(readOnlyPath && !cheatsOpenDatabase(readOnlyPath, 1))
    {
        char error[255];
        sprintf(error, "Error loading read-only cheat database \"%s\"!", readOnlyPath);
        displayError(error);
    }

    char *readWritePath = settingsGetReadWriteDatabasePath();
    if(readWritePath && !cheatsOpenDatabase(readWritePath, 0))
    {
        char error[255];
        sprintf(error, "Error loading read/write cheat database \"%s\"!", readWritePath);
        displayError(error);
    }
    
    cheatsLoadGameMenu();
    cheatsLoadHistory();

    /* Main Loop */
    while(1)
    {
        handlePad();
        graphicsRender();
    }

    return 0;
}
