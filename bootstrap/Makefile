EE_BIN = bootstrap.elf
EE_OBJS = main.o

#EE_LDFLAGS = -s -Ttext 0x00090000
#EE_LDFLAGS = -s -Ttext 0x01900000
#EE_LDFLAGS = -s -Ttext 0x000F3000
#EE_LIBS += -lkernel -lpatches -lerl

EE_CFLAGS  := -mips3 -ffreestanding -fno-builtin -G0 \
		-fshort-double -mlong64 -mhard-float -mno-abicalls -O2 -EL -Wall \
		$(EE_INCS) $(EE_CFLAGS)

EE_ASFLAGS := -EL -G0 $(EE_ASFLAGS)

LOADADDR  = 0x90000
STACKADDR = 0xA8000
STACKSIZE = 0x04000

LDPARAMS := -Wl,--defsym -Wl,_stack_size=$(STACKSIZE) -Wl,--defsym -Wl,_stack=$(STACKADDR)

EE_LDFLAGS += -Wl,-Ttext -Wl,$(LOADADDR) -s $(LDPARAMS)

all: $(EE_BIN)
	ee-strip -s -d -R .mdebug.eabi64 -R .reginfo -R .comment $(EE_BIN)
	rm -f *.o *.s

clean:
	rm -f *.o *.s *.elf

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
