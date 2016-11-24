Cheat Device for PS2
====================
Cheat Device is a game enhancer for PlayStation 2 games, similar to Action 
Replay, GameShark, and CodeBreaker. It supports booting retail, disc based 
games as well as ELF files such as OpenPS2Loader or ESR.

# Features
* Easy to navigate menu system similar to CodeBreaker
* Supports retail discs and loader ELFs
* Fast loading of large cheat lists
* Save manager for backing up and restoring game saves to/from a flash drive
* Powered by ps2rd's powerful cheat engine

# Important Things
## Storing Cheats
Cheat databases are stored in **CDB** files created with **[cdb-util](https://mega.nz/#!LNYB0DAL!n_2Co6zI8c3fun-Mb2-KtA-nIR1wn1vCP_mu4dQR_wg)**, which will
convert cheat lists following this simple format:

```
"Game Name"
Enable
90000000 88888888
Cheat 1
27654321 12345678
Cheat 2
12345678 98765432
Cheat Section
Cheat 3
11111111 00000000

// Comment line
```

## Enable Codes
9-type enable codes (9xxxxxxx yyyyyyyy) are supported, and **the first cheat 
for every game must contain an enable code**. If an F-type enable code is used 
instead (which is common for older games), it will be silently ignored and a 
hook will be installed automatically by the code engine. Many games have been 
tested with the auto-hook function, but its best if a 9-type enable code is 
used.

## Code format
All cheats must be in "RAW" format; Cheat Device can't decrypt or read any 
other formats (ARMAX, CB1-6, CB7+, AR2, GS, etc.). However, you can use  
tools such as OmniConvert to convert any of these formats to RAW.

## Settings
Settings are stored in an ini file named "CheatDevicePS2.ini", which needs to
be located in the directory Cheat Device is run from. The cheat database path
and additional boot paths are set here. See the included settings file for an
example.

# Planned Features
* Loading of plain-text cheat lists
* Loading of ELF files from the hard drive
* Saving/opening additional game save formats (*.MAX, etc.)
* Console-side cheat manipulation
* Save list of enabled cheats before starting a game. Re-enable cheats next
  time Cheat Device is run.

# Compiling
Note: You must have zlib installed from ps2sdk-ports.
```bash
    cd cheatdeviceps2
	make release
```
If you have Docker installed, it's easiest to compile Cheat Device using my
docker image:
```bash
    ./docker-make
```

# License
Cheat Device is not licensed, sponsored, or endorsed by Sony Computer 
Entertainment, Inc. This program comes with ABSOLUTELY NO WARRANTY. Cheat 
Device is licensed under GNU GPL-3. See LICENSE for details. The cheat 
engine is from ps2rd by Mathias Lafedlt. The bootstrap is based on EE_CORE 
from OpenPS2Loader.
