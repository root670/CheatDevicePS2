Cheat Device
============

[![Build Status](https://travis-ci.com/root670/CheatDevicePS2.svg?branch=master)](https://travis-ci.com/root670/CheatDevicePS2)
[![Downloads](https://img.shields.io/github/downloads/root670/CheatDevicePS2/total.svg)](https://github.com/root670/CheatDevicePS2/releases)
[![License](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](https://github.com/root670/CheatDevicePS2/blob/master/LICENSE)

Cheat Device is a game enhancer for PlayStation and PlayStation 2 games similar
to Action Replay, GameShark, and CodeBreaker. It supports booting retail,
disc-based games as well as ELF files such as OpenPS2Loader or ESR on the PS2
version.

See the [wiki](https://github.com/root670/CheatDevicePS2/wiki) to get started
using Cheat Device.

# Features
* Easy to navigate menu system similar to CodeBreaker
* Fast loading of large cheat lists

## PS2 Version
* Supports booting retail discs and loader ELFs
* Save manager for backing up and restoring game saves to/from a flash drive
* Powered by ps2rd's powerful cheat engine

# Compiling
If you have Docker installed, it's easiest to compile Cheat Device using my
Docker image:

## Unix
```bash
./docker-make [args...]
```

## Windows Command Prompt
```cmd
docker-make.bat [args...]
```

## Windows PowerShell
```ps
.\docker-make.bat [args...]
```

See the [compile](https://github.com/root670/CheatDevicePS2/wiki/Compiling) 
page in the wiki for instructions to build without Docker.

# License
Cheat Device is not licensed, sponsored, or endorsed by Sony Computer 
Entertainment, Inc. This program comes with ABSOLUTELY NO WARRANTY. Cheat 
Device is licensed under GNU GPL-3. See LICENSE for details.

The cheat engine in the PS2 version is from ps2rd by Mathias Lafedlt. The
bootstrap is based on EE_CORE from OpenPS2Loader.
