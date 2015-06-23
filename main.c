#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <debug.h>
#include <libpad.h>
#include "graphics.h"
#include "storage.h"
#include "menus.h"
#include "cheats.h"
#include "settings.h"
#include "util.h"
#include "database.h"

extern u8  _cheatdb_start[];
extern int _cheatdb_size;

int main(int argc, char *argv[])
{
	loadModules();
	initGraphicsMan();
	initStorageMan();
	initSettingsMan();
	initCheatMan();
	initMenuMan();
	
	//if(!cheatsOpenDatabase(settingsGetDatabasePath(), 0))
	if(!cheatsOpenDatabase(settingsGetDatabasePath(), 0))
	{
		char error[255];
		sprintf(error, "Error loading cheat database \"%s\"!", settingsGetDatabasePath());
		graphicsDrawText(50, 300, error, WHITE);
		graphicsRender();
		SleepThread();
	}
	
	cheatsLoadGameMenu();
	graphicsClearScreen(128, 128, 128);

	/* Main Loop */
	while(1)
	{
		graphicsDrawBackground();
		handlePad();
		
		menuRender();
		cheatsDrawStats();
		graphicsRender();
	}
	
	killCheatMan();
	killStorageMan();
	SleepThread();
	return 0;
}
