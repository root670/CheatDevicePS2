#ifndef PAD_H
#define PAD_H

#ifdef __PS2__
    #include <libpad.h>
#elif __PS1__
    #include <psx.h>
#endif

#include "types.h"

typedef enum delayTime {
    DELAYTIME_FAST = 2,
    DELAYTIME_SLOW = 6
} delayTime_t;

// Initialize pad
void padInitialize();

// Poll controller for button status
void padPoll(delayTime_t delayTime);

// Get buttons pressed momentarily
u32 padPressed();

// Get buttons held down
u32 padHeld();

#endif // PAD_H
