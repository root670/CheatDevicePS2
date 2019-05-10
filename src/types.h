#ifndef TYPES_H
#define TYPES_H

#ifdef __PS2__
    #include <tamtypes.h>

#elif __PS1__
    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;
    typedef unsigned long u64;

    typedef signed char s8;
    typedef signed short s16;
    typedef signed int s32;
    typedef signed long s64;
#endif

#endif // TYPES_H
