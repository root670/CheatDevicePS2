#
# Cheat Device for PlayStation 2
# by root670
#

EE_BIN = cheatdevice.elf

# Helper libraries
OBJS += libraries/upng.o
OBJS += libraries/miniz.o
OBJS += libraries/ini.o

# Main
OBJS += util.o
OBJS += startgame.o
OBJS += database.o
OBJS += cheats.o
OBJS += graphics.o
OBJS += storage.o
OBJS += menus.o
OBJS += settings.o

# Graphic resources
OBJS += background_png.o
OBJS += check_png.o
OBJS += gamepad_png.o
OBJS += cube_png.o
OBJS += cogs_png.o

# Engine
OBJS += engine_erl.o

# Bootstrap ELF
OBJS += bootstrap_elf.o

GSKIT = $(PS2DEV)/gsKit

EE_LIBS += -lpad -lgskit_toolkit -lgskit -ldmakit -lc -lkernel -lmc -lpatches -lerl -lcdvd
EE_LDFLAGS += -L$(PS2SDK)/ee/lib -L$(GSKIT)/lib -s
EE_INCS += -I$(GSKITSRC)/ee/gs/include -I$(GSKITSRC)/ee/dma/include -I$(GSKITSRC)/ee/toolkit/include

IRX_OBJS += usbd_irx.o usb_mass_irx.o iomanX_irx.o

EE_OBJS = $(IRX_OBJS) $(OBJS) main.o

all: modules main

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
	
	# Engine
	bin2o resources/engine.erl engine_erl.o _engine_erl

	# Bootstrap
	cd bootstrap && $(MAKE)
	bin2o bootstrap/bootstrap.elf bootstrap_elf.o _bootstrap_elf

main: $(EE_BIN)
	rm -f *.o
	rm -f libraries/*.o
	rm -f bootstrap/*.elf bootstrap/*.o

clean:
	rm -f *.o *.elf
	rm -f libraries/*.o
	rm -f bootstrap/*.elf

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
