Cheat Device for PS2
====================
Cheat Device is a game enhancer for PlayStation 2 games, similar to Action 
Replay or CodeBreaker. It supports booting retail, disc based games as well as
ELF files (such as OpenPS2Loader, ESR, etc.) stored on a flashdrive or memory
card. Cheat databases are stored in files created with makecdb, which will
convert cheat lists using a this simple format:

```
"Game Name"
Cheat 1
27654321 12345678
Cheat 2
12345678 98765432
```

## Settings File
Settings are stored in an ini file named "CheatDevicePS2.ini", which needs to
be located in the directory Cheat Device is run from. The cheat database path
and additional boot paths are set here. See the included settings file for an
example.

## Features
* Easy to navigate menu system similar to CodeBreaker
* Supports retail discs and loader ELFs
* Fast loading of large cheat lists
* Powered by ps2rd's powerful cheat engine

## Compiling
Compile Cheat Device
```bash
    cd cheatdeviceps2
	make all
```

Compile makecdb
```bash
	cd cheatdeviceps2/makecdb
	make all
```

## License
Cheat Device is not licensed, sponsored, endorsed by Sony Computer 
Entertainment, Inc. This program comes with ABSOLUTELY NO WARRANTY. Cheat 
Device is licensed under GNU GPL-3. See LICENSE for details. The cheat 
engine is from ps2rd by Mathias Lafedlt. The bootstrap is based on EE_CORE 
from OpenPS2Loader.