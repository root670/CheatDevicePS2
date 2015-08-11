#
# Cheat Device for PlayStation 2
# by root670
#

EE_BIN = cheatdevice.elf

# Helper libraries
OBJS += libraries/upng.o
OBJS += libraries/ini.o

# Main
OBJS += util.o
OBJS += startgame.o
OBJS += database.o
OBJS += cheats.o
OBJS += graphics.o
OBJS += saves.o
OBJS += menus.o
OBJS += settings.o

# Graphic resources
OBJS += background_png.o
OBJS += check_png.o
OBJS += gamepad_png.o
OBJS += cube_png.o
OBJS += cogs_png.o
OBJS += savemanager_png.o
OBJS += flashdrive_png.o
OBJS += memorycard1_png.o
OBJS += memorycard2_png.o

# Engine
OBJS += engine_erl.o

# Bootstrap ELF
OBJS += bootstrap_elf.o

GSKIT = $(PS2DEV)/gsKit

EE_LIBS += -lpad -lgskit_toolkit -lgskit -ldmakit -lc -lkernel -lmc -lpatches -lerl -lcdvd -lz
EE_LDFLAGS += -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib -L$(GSKIT)/lib -s
EE_INCS += -I$(GSKIT)/include -I$(PS2SDK)/ports/include

IRX_OBJS += usbd_irx.o usb_mass_irx.o iomanX_irx.o

EE_OBJS = $(IRX_OBJS) $(OBJS) main.o

all: modules version main

modules:
	# IRX Modules
	bin2o resources/iomanX.irx iomanX_irx.o _iomanX_irx
	bin2o resources/usbd.irx usbd_irx.o _usbd_irx
	bin2o resources/usb_mass.irx usb_mass_irx.o _usb_mass_irx

	# Graphics
	bin2o resources/background.png background_png.o _background_png
	bin2o resources/check.png check_png.o _check_png
	bin2o resources/gamepad.png gamepad_png.o _gamepad_png
	bin2o resources/cube.png cube_png.o _cube_png
	bin2o resources/cogs.png cogs_png.o _cogs_png
	bin2o resources/savemanager.png savemanager_png.o _savemanager_png
	bin2o resources/flashdrive.png flashdrive_png.o _flashdrive_png
	bin2o resources/memorycard1.png memorycard1_png.o _memorycard1_png
	bin2o resources/memorycard2.png memorycard2_png.o _memorycard2_png
	
	# Engine
	cd engine && $(MAKE)
	bin2o engine/engine.erl engine_erl.o _engine_erl

	# Bootstrap
	cd bootstrap && $(MAKE)
	bin2o bootstrap/bootstrap.elf bootstrap_elf.o _bootstrap_elf

version:
	echo -n '#define GIT_VERSION "' > version.h
	git describe | tr -d '\n' >> version.h
	echo '"' >> version.h

main: $(EE_BIN)
	rm -f version.h
	rm -f *.o
	rm -f libraries/*.o
	rm -f bootstrap/*.elf bootstrap/*.o

release: all
	rm -rf release
	mkdir release
	ps2-packer cheatdevice.elf cheatdevice-packed.elf
	cp cheatdevice-packed.elf cb10.cdb CheatDevicePS2.ini LICENSE README.md release
	rm cheatdevice-packed.elf
	mv release/cheatdevice-packed.elf release/cheatdevice.elf
	cd release && zip -q CheatDevicePS2-$$(git describe).zip *

clean:
	rm -f *.o *.elf
	rm -f libraries/*.o
	rm -f bootstrap/*.elf
	cd engine && make clean

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
