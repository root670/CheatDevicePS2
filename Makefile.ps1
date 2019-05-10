#
# Cheat Device for PS1
# by Wes Castro
#

PROJECT_NAME=cheatdev

all: clean $(PROJECT_NAME)

# Main object files
OBJS += src/main.o
OBJS += src/objectpool.o
OBJS += src/hash.o
OBJS += src/pad.o
OBJS += src/util.o
OBJS += src/startgame.o
OBJS += src/textcheats.o
OBJS += src/cheats.o
OBJS += src/graphics.o
OBJS += src/menus.o
OBJS += src/settings.o

CC = psx-gcc
CFLAGS = -Wall -Werror -D__PS1__
AR = mipsel-unknown-elf-ar
AS = mipsel-unknown-elf-as
RANLIB = mipsel-unknown-elf-ranlib
LD = mipsel-unknown-elf-ld
OBJCOPY = mipsel-unknown-elf-objcopy

clean:
	rm -f $(PROJECT_NAME).bin $(PROJECT_NAME).cue $(PROJECT_NAME).exe $(PROJECT_NAME).elf $(OBJS)
	rm -rf cd_root

src/engine.h:
	$(CC) -c src/platform/ps1/engine.S
	$(OBJCOPY) -j .text -O binary engine.o engine.bin
	bin2c -o src/engine.h -n engine engine.bin

version:
	./version.sh > src/version.h

$(PROJECT_NAME): version src/engine.h $(OBJS)
	$(CC) $(CFLAGS) -o $(PROJECT_NAME).elf $(OBJS) -lm

release: all
	mkdir cd_root
	elf2exe $(PROJECT_NAME).elf $(PROJECT_NAME).exe
	cp $(PROJECT_NAME).exe cd_root
	systemcnf $(PROJECT_NAME).exe > cd_root/system.cnf
	mkisofs -o $(PROJECT_NAME).hsf -V $(PROJECT_NAME) -sysid PLAYSTATION cd_root
	mkpsxiso $(PROJECT_NAME).hsf $(PROJECT_NAME).bin /usr/local/psxsdk/share/licenses/infousa.dat
	rm -f $(PROJECT_NAME).hsf
