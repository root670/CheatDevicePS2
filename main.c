#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <debug.h>
#include <libpad.h>
#include "graphics.h"
#include "menus.h"
#include "cheats.h"
#include "settings.h"
#include "util.h"
#include "database.h"

int main(int argc, char *argv[])
{
    loadModules();
    initGraphics();
    initSettings();
    initMenus();
    
    if(!cheatsOpenDatabase(settingsGetDatabasePath()))
    {
        char error[255];
        sprintf(error, "Error loading cheat database \"%s\"!", settingsGetDatabasePath());
        char *items[] = {"OK"};
        displayPromptMenu(items, 1, error);

        // Go back to OSD
        LoadExecPS2("rom:OSDSYS", 0, NULL);
    }
    
    cheatsLoadGameMenu();
    cheatsLoadHistory();
    graphicsClearScreen(128, 128, 128);

    /* Main Loop */
    while(1)
    {
        graphicsDrawBackground();
        
        menuRender();
        cheatsDrawStats();
        handlePad();
        graphicsRender();
    }
    
    killCheats();
    SleepThread();
    return 0;
}
