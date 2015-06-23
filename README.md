Cheat Device for PS2
====================
Cheat Device is a game enhancer for PlayStation 2 games, similar to Action 
Replay or CodeBreaker. It supports booting retail, disc based games as well as
ELF files (such as OpenPS2Loader, ESR, etc.) stored on a flashdrive or memory
card. Cheat databases are stored in files created with makecdb, which will
convert cheat lists using this simple format:

```
"Game Name"
Cheat 1
27654321 12345678
Cheat 2
12345678 98765432
Cheat Section
Cheat 3
11111111 00000000
// Comment line
```

Currently only 9-type enable codes are supported, so games that use an F-type
enable code will not work. The first cheat for every game must be it's enable
code (9xxxxxxx yyyyyyyy). Also, all cheats must be in "RAW" format; Cheat
Device can't decrypt or read any other formats (ARMAX, CB1-6, CB7+, AR2, GS,
etc.). However, you can use a tool such as OmniConvert to convert any of these
formats to RAW.

## Settings File
Settings are stored in an ini file named "CheatDevicePS2.ini", which needs to
be located in the directory Cheat Device is run from. The cheat database path
and additional boot paths are set here. See the included settings file for an
example.

## Features
* Easy to navigate menu system similar to CodeBreaker
* Supports retail discs and loader ELFs
* Fast loading of large cheat lists
* Save manager for backing up and restoring game saves to/from a flash drive
* Powered by ps2rd's powerful cheat engine

## Planned Features
* Loading of plain-text cheat lists
* Saving/opening additional game save formats (*.CBS, *.MAX, etc.)
* Console-side cheat manipulation
* Save list of enabled cheats before starting a game. Re-enable cheats next
  time Cheat Device runs.

## Compiling
Compile Cheat Device
```bash
    cd cheatdeviceps2
	make all
```

## License
Cheat Device is not licensed, sponsored, endorsed by Sony Computer 
Entertainment, Inc. This program comes with ABSOLUTELY NO WARRANTY. Cheat 
Device is licensed under GNU GPL-3. See LICENSE for details. The cheat 
engine is from ps2rd by Mathias Lafedlt. The bootstrap is based on EE_CORE 
from OpenPS2Loader.