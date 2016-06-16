#include <tamtypes.h>
#include <kernel.h>
#include <syscallnr.h>
#include <ee_regs.h>
#include <ps2lib_err.h>
#include <ps2_reg_defs.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <string.h>
#include <stdio.h>

/* Debug colors. Now NTSC Safe! At least I believe they are... */
int red = 0x1010B4; /* RED: Opening elf */
int green = 0x10B410; /* GREEN: Reading elf */
int blue = 0xB41010; /* BLUE: Installing elf header */
int yellow = 0x10B4B4; /* YELLOW: ExecPS2 */
int orange = 0x0049E6; /* ORANGE: SifLoadElf */
int pink = 0xB410B4; /* PINK: Launching OSDSYS */
int white = 0xEBEBEB; /* WHITE: Failed to launch OSDSYS */
int purple = 0x801080; /* PURPLE: Launching uLaunchELF */
int black = 0x101010;

#define ELF_MAGIC   0x464c457f
#define ELF_PT_LOAD 1

int g_argc;
char *g_argv[16];
#define GS_BGCOLOUR *((vu32*)0x120000e0)

//#define MAKE_JAL(addr)  (((u32)addr << 4) >> 6) + 0x0C000000;
#define MAKE_JAL(addr)  (u32)(0x0C000000 | (0x03FFFFFF & ((u32)addr >> 2)))
u64 syscallHook;

struct _lf_elf_load_arg {
    union
    {
        u32 epc;
        int result;
    } p;
    u32 gp;
    char path[LF_PATH_MAX];
    char secname[LF_ARG_MAX];
} __attribute__((aligned (16)));

typedef struct {
    u8  ident[16];
    u16 type;
    u16 machine;
    u32 version;
    u32 entry;
    u32 phoff;
    u32 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
} elf_header_t;

typedef struct {
    u32 type;
    u32 offset;
    void *vaddr;
    u32 paddr;
    u32 filesz;
    u32 memsz;
    u32 flags;
    u32 align;
} elf_pheader_t;

/* From NetCheat */
void MyLoadElf(char *elfpath)
{
    ResetEE(0x7f);

    /* Clear user memory */
    int i;
    for (i = 0x00100000; i < 0x02000000; i += 64) {
        __asm__ (
        "\tsq $0, 0(%0) \n"
        "\tsq $0, 16(%0) \n"
        "\tsq $0, 32(%0) \n"
        "\tsq $0, 48(%0) \n"
        :: "r" (i)
        );
    }

    /* Clear scratchpad memory */
    memset((void*)0x70000000, 0, 16 * 1024);

    char *args[1];
    t_ExecData sifelf;

    memset(&sifelf, 0, sizeof(t_ExecData)); 

    elf_header_t boot_header;
    elf_pheader_t boot_pheader;
    int fd = 0;
    fioInit();
    GS_BGCOLOUR = red; /* RED: Opening elf */
    fd = fioOpen(elfpath, O_RDONLY); //Open the elf
    if (fd < 0) { fioClose(fd); return; } //If it doesn't exist, return
    
    GS_BGCOLOUR = blue;
    
    if(read(fd, &boot_header, sizeof(elf_header_t)) != sizeof(elf_header_t)) {
        close(fd);
    }
    
    GS_BGCOLOUR = white;
    
    /* check ELF magic */
    if ((*(u32*)boot_header.ident) != ELF_MAGIC) {
        close(fd);
    }
    
    /* copy loadable program segments to RAM */
    for (i = 0; i < boot_header.phnum; i++) {
        lseek(fd, boot_header.phoff+(i*sizeof(elf_pheader_t)), SEEK_SET);
        read(fd, &boot_pheader, sizeof(elf_pheader_t));

        if (boot_pheader.type != ELF_PT_LOAD)
            continue;

        lseek(fd, boot_pheader.offset, SEEK_SET);
        read(fd, boot_pheader.vaddr, boot_pheader.filesz);

        if (boot_pheader.memsz > boot_pheader.filesz)
            memset(boot_pheader.vaddr + boot_pheader.filesz, 0,
                    boot_pheader.memsz - boot_pheader.filesz);
                    
    }
    
    close(fd);

    // Booting part
    GS_BGCOLOUR = yellow; /* YELLOW: ExecPS2 */
    
    /* IOP reboot routine from ps2rd */
    SifInitRpc(0);

    while (!SifIopReset("rom0:UDNL rom0:EELOADCNF", 0))
        ;
    while (!SifIopSync())
        ;

    /* exit services */
    fioExit();
    SifExitIopHeap();
    SifLoadFileExit();
    SifExitRpc();
    SifExitCmd();

    FlushCache(0);
    FlushCache(2);

    args[0] = elfpath;
    ee_kmode_enter();
    *((u32 *)(0x800002FC)) = MAKE_JAL(syscallHook);
    //*((u32 *)(0x800002fc)) = 0x0C02000e;
    ee_kmode_exit();
    ExecPS2((u32*)boot_header.entry, 0, 1, args);
}

/*
argv[0] = elf path
argv[1] = syscallHook address
*/

int main(int argc, char *argv[])
{
    sscanf(argv[1], "%X", &syscallHook);
    //syscallHook = 0x00080038;
    printf("%X %X\n", syscallHook, (void*)argv[1]);
    MyLoadElf((char *) argv[0]);

    return 0;
}
