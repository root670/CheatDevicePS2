#
# Cheat Device for PlayStation 2
# by root670
#

DTL_T10000 = 0

EE_BIN = cheatdevice.elf

# For minizip
EE_CFLAGS += -DUSE_FILE32API

# Helper libraries
OBJS += src/libraries/upng.o
OBJS += src/libraries/ini.o
OBJS += src/libraries/minizip/ioapi.o
OBJS += src/libraries/minizip/zip.o
OBJS += src/libraries/minizip/unzip.o
OBJS += src/libraries/lzari.o

# Main
OBJS += src/main.o
OBJS += src/objectpool.o
OBJS += src/hash.o
OBJS += src/pad.o
OBJS += src/util.o
OBJS += src/startgame.o
OBJS += src/textcheats.o
OBJS += src/cheats.o
OBJS += src/graphics.o
OBJS += src/saves.o
OBJS += src/menus.o
OBJS += src/settings.o

# Save Formats
OBJS += src/saveformats/util.o
OBJS += src/saveformats/cbs.o
OBJS += src/saveformats/psu.o
OBJS += src/saveformats/zip.o

# IRX Modules
IRX_OBJS += resources/usbd_irx.o
IRX_OBJS += resources/usbhdfsd_irx.o
IRX_OBJS += resources/iomanX_irx.o
ifeq ($(DTL_T10000),1)
	IRX_OBJS += resources/sio2man_irx.o
	IRX_OBJS += resources/mcman_irx.o
	IRX_OBJS += resources/mcserv_irx.o
	IRC_OBJS += resources/padman_irx.o
endif

# Graphic resources
OBJS += resources/background_png.o
OBJS += resources/check_png.o
OBJS += resources/hamburgerIcon_png.o
OBJS += resources/gamepad_png.o
OBJS += resources/cube_png.o
OBJS += resources/savemanager_png.o
OBJS += resources/flashdrive_png.o
OBJS += resources/memorycard1_png.o
OBJS += resources/memorycard2_png.o
OBJS += resources/buttonCross_png.o
OBJS += resources/buttonCircle_png.o
OBJS += resources/buttonTriangle_png.o
OBJS += resources/buttonSquare_png.o
OBJS += resources/buttonStart_png.o
OBJS += resources/buttonSelect_png.o
OBJS += resources/buttonL1_png.o
OBJS += resources/buttonL2_png.o
OBJS += resources/buttonL3_png.o
OBJS += resources/buttonR1_png.o
OBJS += resources/buttonR2_png.o
OBJS += resources/buttonR3_png.o

# Engine
OBJS += engine/engine_erl.o

# Bootstrap ELF
OBJS += bootstrap/bootstrap_elf.o

ifeq ($(DTL_T10000),1)
	EE_CFLAGS += -D_DTL_T10000 -g
	EE_LIBS += -lpadx
else
	EE_LIBS += -lpad
endif

# Replace uses of "mass" with "host"
ifeq ($(NO_MASS),1)
	EE_CFLAGS += -D_NO_MASS
endif

GSKIT = $(PS2DEV)/gsKit
EE_LIBS += -lgskit_toolkit -lgskit -ldmakit -lc -lkernel -lmc -lpatches -lerl -lcdvd -lz -lmf
EE_LDFLAGS += -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib -s
EE_INCS += -I$(GSKIT)/include -I$(PS2SDK)/ports/include

EE_OBJS = $(IRX_OBJS) $(OBJS)

all: modules version main

modules:
	@# IRX Modules
	@bin2o $(PS2SDK)/iop/irx/iomanX.irx resources/iomanX_irx.o _iomanX_irx
	@bin2o $(PS2SDK)/iop/irx/usbd.irx resources/usbd_irx.o _usbd_irx
	@bin2o $(PS2SDK)/iop/irx/usbhdfsd.irx resources/usbhdfsd_irx.o _usbhdfsd_irx
ifeq ($(DTL_T10000),1)
	@bin2o $(PS2SDK)/iop/irx/freesio2.irx resources/sio2man_irx.o _sio2man_irx
	@bin2o $(PS2SDK)/iop/irx/mcman.irx resources/mcman_irx.o _mcman_irx
	@bin2o $(PS2SDK)/iop/irx/mcserv.irx resources/mcserv_irx.o _mcserv_irx
	@bin2o $(PS2SDK)/iop/irx/freepad.irx resources/padman_irx.o _padman_irx
endif

	@# Graphics
	@bin2o resources/background.png resources/background_png.o _background_png
	@bin2o resources/check.png resources/check_png.o _check_png
	@bin2o resources/hamburgerIcon.png resources/hamburgerIcon_png.o _hamburgerIcon_png

	@bin2o resources/gamepad.png resources/gamepad_png.o _gamepad_png
	@bin2o resources/cube.png resources/cube_png.o _cube_png
	@bin2o resources/savemanager.png resources/savemanager_png.o _savemanager_png
	@bin2o resources/flashdrive.png resources/flashdrive_png.o _flashdrive_png
	@bin2o resources/memorycard1.png resources/memorycard1_png.o _memorycard1_png
	@bin2o resources/memorycard2.png resources/memorycard2_png.o _memorycard2_png
	@bin2o resources/buttonCross.png resources/buttonCross_png.o _buttonCross_png
	@bin2o resources/buttonCircle.png resources/buttonCircle_png.o _buttonCircle_png
	@bin2o resources/buttonTriangle.png resources/buttonTriangle_png.o _buttonTriangle_png
	@bin2o resources/buttonSquare.png resources/buttonSquare_png.o _buttonSquare_png
	@bin2o resources/buttonStart.png resources/buttonStart_png.o _buttonStart_png
	@bin2o resources/buttonSelect.png resources/buttonSelect_png.o _buttonSelect_png
	@bin2o resources/buttonL1.png resources/buttonL1_png.o _buttonL1_png
	@bin2o resources/buttonL2.png resources/buttonL2_png.o _buttonL2_png
	@bin2o resources/buttonL3.png resources/buttonL3_png.o _buttonL3_png
	@bin2o resources/buttonR1.png resources/buttonR1_png.o _buttonR1_png
	@bin2o resources/buttonR2.png resources/buttonR2_png.o _buttonR2_png
	@bin2o resources/buttonR3.png resources/buttonR3_png.o _buttonR3_png

	@# Engine
	@cd engine && $(MAKE)
	@bin2o engine/engine.erl engine/engine_erl.o _engine_erl

	@# Bootstrap
	@cd bootstrap && $(MAKE)
	@bin2o bootstrap/bootstrap.elf bootstrap/bootstrap_elf.o _bootstrap_elf

version:
	@./version.sh > src/version.h

main: $(EE_BIN)
	rm -rf src/*.o src/libraries/*.o src/libraries/minizip/*.o src/saveformats/*.o
	rm -f resources/*.o
	rm -f bootstrap/*.elf bootstrap/*.o
	rm -f engine/*.erl engine/*.o

release: all
	rm -rf release
	mkdir release
	ps2-packer cheatdevice.elf release/cheatdevice.elf
	zip -q -9 release/CheatDatabase.zip CheatDatabase.txt
	cp CheatDevicePS2.ini LICENSE README.md release
	sed -i 's/CheatDatabase.txt/CheatDatabase.zip/g' release/CheatDevicePS2.ini
	cd release && zip -q -9 CheatDevicePS2-$$(git describe).zip *

clean:
	rm -rf src/*.o src/libraries/*.o src/libraries/minizip/*.o src/saveformats/*.o *.elf
	rm -f resources/*.o
	cd engine && make clean
	cd bootstrap && make clean

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
